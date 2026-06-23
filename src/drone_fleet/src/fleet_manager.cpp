#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <map>
#include <sstream>
#include <iomanip>

struct DroneState {
    std::string name;
    double battery{0.0};
    double altitude{0.0};
    std::string status{"unknown"};
    int waypoint{0};
    int total_waypoints{0};
};

static std::string parse_field(const std::string& json, const std::string& key) {
    // Simple manual JSON field extractor
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

class FleetManager : public rclcpp::Node {
public:
    FleetManager() : Node("fleet_manager") {
        for (const auto& name : {"Alpha", "Beta", "Gamma"}) {
            states_[name] = DroneState{};
            states_[name].name = name;

            std::string n(name);
            status_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/status", 10,
                [this, n](const std_msgs::msg::String::SharedPtr msg) {
                    parse_status(n, msg->data);
                }));
            alert_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/alert", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) {
                    RCLCPP_WARN(this->get_logger(), "[ALERT] %s", msg->data.c_str());
                }));
            complete_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/mission_complete", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) {
                    RCLCPP_INFO(this->get_logger(), "[MISSION COMPLETE] %s", msg->data.c_str());
                }));
            telemetry_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + n + "/telemetry", 10,
                [this, n](const std_msgs::msg::String::SharedPtr msg) {
                    parse_telemetry(n, msg->data);
                }));
        }

        report_timer_ = this->create_wall_timer(
            std::chrono::seconds(5),
            std::bind(&FleetManager::print_report, this));

        service_ = this->create_service<std_srvs::srv::Trigger>(
            "/fleet/status_report",
            [this](const std_srvs::srv::Trigger::Request::SharedPtr,
                   std_srvs::srv::Trigger::Response::SharedPtr response) {
                print_report();
                response->success = true;
                response->message = "Report printed to console";
            });
    }

private:
    std::map<std::string, DroneState> states_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> status_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> alert_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> complete_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;
    rclcpp::TimerBase::SharedPtr report_timer_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr service_;

    void parse_status(const std::string& name, const std::string& data) {
        // format: name:Alpha|battery:87.3|altitude:15.2|status:flying|waypoint:2/5|speed:3.2
        auto& s = states_[name];
        auto get_val = [&](const std::string& key) -> std::string {
            size_t pos = data.find(key + ":");
            if (pos == std::string::npos) return "";
            pos += key.size() + 1;
            size_t end = data.find('|', pos);
            return data.substr(pos, end == std::string::npos ? end : end - pos);
        };
        try {
            s.battery = std::stod(get_val("battery"));
            s.altitude = std::stod(get_val("altitude"));
            s.status = get_val("status");
            std::string wp = get_val("waypoint");
            size_t slash = wp.find('/');
            if (slash != std::string::npos) {
                s.waypoint = std::stoi(wp.substr(0, slash));
                s.total_waypoints = std::stoi(wp.substr(slash + 1));
            }
        } catch (...) {}
    }

    void parse_telemetry(const std::string& name, const std::string& json) {
        auto& s = states_[name];
        try {
            s.battery = std::stod(parse_field(json, "battery"));
            s.altitude = std::stod(parse_field(json, "altitude"));
            s.status = parse_field(json, "status");
            s.waypoint = std::stoi(parse_field(json, "waypoint"));
            s.total_waypoints = std::stoi(parse_field(json, "total_waypoints"));
        } catch (...) {}
    }

    void print_report() {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "\n╔══════════════════════════════════════════════════════╗\n";
        oss << "║              FLEET STATUS REPORT                    ║\n";
        oss << "╠═══════╦══════════╦══════════╦══════════╦════════════╣\n";
        oss << "║ Drone ║ Battery  ║ Altitude ║ Waypoint ║  Status    ║\n";
        oss << "╠═══════╬══════════╬══════════╬══════════╬════════════╣\n";
        for (const auto& [name, s] : states_) {
            oss << "║ " << std::setw(5) << std::left << name
                << " ║ " << std::setw(8) << s.battery
                << " ║ " << std::setw(8) << s.altitude
                << " ║ " << std::setw(3) << s.waypoint << "/" << std::setw(4) << s.total_waypoints
                << " ║ " << std::setw(10) << s.status << " ║\n";
        }
        oss << "╚═══════╩══════════╩══════════╩══════════╩════════════╝\n";
        RCLCPP_INFO(this->get_logger(), "%s", oss.str().c_str());
    }
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FleetManager>());
    rclcpp::shutdown();
    return 0;
}
