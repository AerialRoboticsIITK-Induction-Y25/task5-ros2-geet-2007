#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <deque>
#include <map>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

static std::string parse_field(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        return json.substr(pos, end - pos);
    } else {
        size_t end = json.find_first_of(",}", pos);
        return json.substr(pos, end - pos);
    }
}

class HealthMonitor : public rclcpp::Node {
public:
    HealthMonitor() : Node("health_monitor") {
        for (const auto& name : {"Alpha", "Beta", "Gamma"}) {
            std::string n(name);
            battery_samples_[n] = std::deque<double>();
            telemetry_subs_.push_back(
                this->create_subscription<std_msgs::msg::String>(
                    "/drone/" + n + "/telemetry", 10,
                    [this, n](const std_msgs::msg::String::SharedPtr msg) {
                        handle_telemetry(n, msg->data);
                    }));
        }

        warning_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
        summary_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

        diag_timer_ = this->create_wall_timer(
            std::chrono::seconds(10),
            std::bind(&HealthMonitor::print_diagnostics, this));
    }

private:
    std::map<std::string, std::deque<double>> battery_samples_;
    std::map<std::string, double> last_battery_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr warning_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;
    rclcpp::TimerBase::SharedPtr diag_timer_;

    void handle_telemetry(const std::string& name, const std::string& json) {
        double battery = 0.0;
        try { battery = std::stod(parse_field(json, "battery")); } catch (...) { return; }

        auto& samples = battery_samples_[name];
        if (last_battery_.count(name)) {
            double drain = last_battery_[name] - battery;
            samples.push_back(drain);
            if (samples.size() > 10) samples.pop_front();

            double avg_drain = 0.0;
            for (auto d : samples) avg_drain += d;
            avg_drain /= samples.size();

            if (avg_drain > 1.5) {
                auto msg = std_msgs::msg::String();
                msg.data = "WARNING: " + name + " drain rate=" +
                           std::to_string(avg_drain) + "%/sample";
                warning_pub_->publish(msg);
                RCLCPP_WARN(this->get_logger(), "%s", msg.data.c_str());
            }
        }
        last_battery_[name] = battery;
    }

    void print_diagnostics() {
        std::ostringstream oss, json;
        oss << std::fixed << std::setprecision(2);
        oss << "\n╔══════════════════════════════════════════════════════════════╗\n";
        oss << "║                  HEALTH DIAGNOSTICS                         ║\n";
        oss << "╠═══════╦════════════╦═══════════════════╦════════════════════╣\n";
        oss << "║ Drone ║ Drain Rate ║ Time to Critical  ║ Time to Depletion  ║\n";
        oss << "╠═══════╬════════════╬═══════════════════╬════════════════════╣\n";

        json << "{\"drones\":[";
        bool first = true;

        for (const auto& [name, samples] : battery_samples_) {
            double avg_drain = 0.0;
            if (!samples.empty()) {
                for (auto d : samples) avg_drain += d;
                avg_drain /= samples.size();
            }

            double battery = last_battery_.count(name) ? last_battery_[name] : 100.0;
            double time_to_critical = avg_drain > 0 ? (battery - 20.0) / avg_drain * 2 : -1;
            double time_to_depletion = avg_drain > 0 ? battery / avg_drain * 2 : -1;

            std::string ttc = time_to_critical > 0 ? std::to_string((int)time_to_critical) + "s" : "N/A";
            std::string ttd = time_to_depletion > 0 ? std::to_string((int)time_to_depletion) + "s" : "N/A";

            oss << "║ " << std::setw(5) << std::left << name
                << " ║ " << std::setw(10) << avg_drain
                << " ║ " << std::setw(17) << ttc
                << " ║ " << std::setw(18) << ttd << " ║\n";

            if (!first) json << ",";
            json << "{\"name\":\"" << name << "\",\"drain_rate\":" << avg_drain
                 << ",\"time_to_critical\":\"" << ttc << "\",\"time_to_depletion\":\"" << ttd << "\"}";
            first = false;
        }

        oss << "╚═══════╩════════════╩═══════════════════╩════════════════════╝\n";
        json << "]}";

        RCLCPP_INFO(this->get_logger(), "%s", oss.str().c_str());

        auto msg = std_msgs::msg::String();
        msg.data = json.str();
        summary_pub_->publish(msg);
    }
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HealthMonitor>());
    rclcpp::shutdown();
    return 0;
}
