#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <chrono>
#include <deque>
#include <iomanip>
#include <map>
#include <sstream>
#include <ctime>

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

// ── JSON extract (same pattern as fleet_manager) ──────────────────────────────

static std::string jex(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    if (json[pos] == '"') {
        size_t end = json.find('"', pos + 1);
        return json.substr(pos + 1, end - pos - 1);
    }
    size_t end = json.find_first_of(",}", pos);
    return json.substr(pos, end - pos);
}

// ── Battery sample for circular buffer ───────────────────────────────────────

struct BatterySample {
    double battery;
    Clock::time_point time;
};

// ── Per-drone telemetry state ─────────────────────────────────────────────────

struct DroneHealth {
    std::deque<BatterySample> samples;  // circular buffer of last 10
    static constexpr size_t MAX_SAMPLES = 10;

    void add_sample(double battery) {
        samples.push_back({battery, Clock::now()});
        if (samples.size() > MAX_SAMPLES)
            samples.pop_front();
    }

    // Returns drain per second (positive = draining)
    double drain_rate() const {
        if (samples.size() < 2) return 0.0;
        const auto& oldest = samples.front();
        const auto& newest = samples.back();
        double delta_bat  = oldest.battery - newest.battery;
        double delta_time = std::chrono::duration<double>(newest.time - oldest.time).count();
        if (delta_time < 0.001) return 0.0;
        return delta_bat / delta_time;
    }

    // Seconds until battery hits 20% (critical threshold)
    double time_to_critical(double current_battery) const {
        double rate = drain_rate();
        if (rate <= 0) return -1.0; // charging or no data
        double diff = current_battery - 20.0;
        return diff <= 0 ? 0.0 : diff / rate;
    }

    // Seconds until battery hits 0%
    double time_to_depletion(double current_battery) const {
        double rate = drain_rate();
        if (rate <= 0) return -1.0;
        return current_battery / rate;
    }

    double last_battery{100.0};
};

// ── HealthMonitorNode ─────────────────────────────────────────────────────────

class HealthMonitorNode : public rclcpp::Node {
public:
    HealthMonitorNode() : Node("health_monitor") {
        pub_warning_ = create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
        pub_summary_ = create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

        const std::vector<std::string> drones = {"Alpha", "Beta", "Gamma"};
        for (const auto& name : drones) {
            health_[name] = DroneHealth{};
            subs_.push_back(
                create_subscription<std_msgs::msg::String>(
                    "/drone/" + name + "/telemetry", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        on_telemetry(name, msg->data);
                    }));
        }

        timer_diag_ = create_wall_timer(10s, [this]() { print_diagnostics(); });
        RCLCPP_INFO(get_logger(), "HealthMonitor started");
    }

private:
    void on_telemetry(const std::string& name, const std::string& json) {
        double bat = 100.0;
        try { bat = std::stod(jex(json, "battery")); } catch (...) {}

        auto& h = health_[name];
        h.last_battery = bat;
        h.add_sample(bat);

        // Warn if drain rate > 1.5 per second
        double rate = h.drain_rate();
        if (rate > 1.5) {
            std::ostringstream w;
            w << std::fixed << std::setprecision(3);
            w << "{\"warning\":\"HIGH_DRAIN\",\"drone\":\"" << name
              << "\",\"drain_rate_per_sec\":" << rate
              << ",\"battery\":" << bat << "}";
            auto msg = std_msgs::msg::String{};
            msg.data = w.str();
            pub_warning_->publish(msg);
            RCLCPP_WARN(get_logger(),
                        "⚠  HIGH DRAIN on %s: %.3f/sec (battery=%.1f%%)",
                        name.c_str(), rate, bat);
        }
    }

    void print_diagnostics() {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "\n┌─────────────────────────────────────────────────────────────────┐\n";
        oss << "│          HEALTH DIAGNOSTICS — " << now_str() << "                │\n";
        oss << "├───────────┬──────────────┬────────────────┬───────────────────┤\n";
        oss << "│  Drone    │  Drain/sec   │ Time→Critical  │  Time→Depletion   │\n";
        oss << "├───────────┼──────────────┼────────────────┼───────────────────┤\n";

        // Also build JSON summary
        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{\"timestamp\":\"" << now_str() << "\",\"drones\":[";
        bool first = true;

        for (auto& [name, h] : health_) {
            double rate = h.drain_rate();
            double ttc  = h.time_to_critical(h.last_battery);
            double ttd  = h.time_to_depletion(h.last_battery);

            auto fmt_time = [](double t) -> std::string {
                if (t < 0) return "N/A";
                std::ostringstream s;
                s << std::fixed << std::setprecision(1) << t << "s";
                return s.str();
            };

            oss << "│ " << std::left << std::setw(9) << name
                << " │ " << std::right << std::setw(10) << rate << "   "
                << " │ " << std::setw(12) << fmt_time(ttc) << "   "
                << " │ " << std::setw(15) << fmt_time(ttd) << "   │\n";

            if (!first) json << ",";
            first = false;
            json << "{\"name\":\"" << name
                 << "\",\"drain_rate\":" << rate
                 << ",\"time_to_critical\":" << (ttc < 0 ? -1 : ttc)
                 << ",\"time_to_depletion\":" << (ttd < 0 ? -1 : ttd)
                 << ",\"battery\":" << h.last_battery << "}";
        }

        oss << "└───────────┴──────────────┴────────────────┴───────────────────┘\n";
        json << "]}";

        RCLCPP_INFO(get_logger(), "%s", oss.str().c_str());

        auto summary_msg = std_msgs::msg::String{};
        summary_msg.data = json.str();
        pub_summary_->publish(summary_msg);
    }

    static std::string now_str() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&t, &tm);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
        return buf;
    }

    std::map<std::string, DroneHealth> health_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_warning_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_summary_;
    rclcpp::TimerBase::SharedPtr timer_diag_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HealthMonitorNode>());
    rclcpp::shutdown();
    return 0;
}
