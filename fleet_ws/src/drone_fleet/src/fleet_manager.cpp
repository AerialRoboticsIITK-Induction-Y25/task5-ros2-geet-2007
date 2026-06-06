#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std::chrono_literals;

// ── Tiny manual JSON parser ───────────────────────────────────────────────────

static std::string json_extract(const std::string& json, const std::string& key) {
    // Finds "key": value  (handles string and number values)
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

// ── Per-drone state ───────────────────────────────────────────────────────────

struct DroneState {
    std::string name;
    double      battery{100.0};
    double      altitude{0.0};
    std::string status{"unknown"};
    int         wp_index{0};
    int         wp_total{0};
    std::string mission{"—"};
    bool        critical{false};
    std::string last_update;
};

// ── FleetManagerNode ──────────────────────────────────────────────────────────

class FleetManagerNode : public rclcpp::Node {
public:
    FleetManagerNode() : Node("fleet_manager") {
        const std::vector<std::string> drones = {"Alpha", "Beta", "Gamma"};

        for (const auto& name : drones) {
            fleet_[name] = DroneState{name};
            std::string base = "/drone/" + name;

            // Status
            subs_status_.push_back(
                create_subscription<std_msgs::msg::String>(
                    base + "/status", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        on_status(name, msg->data);
                    }));

            // Alert
            subs_alert_.push_back(
                create_subscription<std_msgs::msg::String>(
                    base + "/alert", 10,
                    [this](const std_msgs::msg::String::SharedPtr msg) {
                        on_alert(msg->data);
                    }));

            // Mission complete
            subs_complete_.push_back(
                create_subscription<std_msgs::msg::String>(
                    base + "/mission_complete", 10,
                    [this](const std_msgs::msg::String::SharedPtr msg) {
                        RCLCPP_INFO(get_logger(), "[MISSION_COMPLETE] %s", msg->data.c_str());
                    }));

            // Telemetry
            subs_telem_.push_back(
                create_subscription<std_msgs::msg::String>(
                    base + "/telemetry", 10,
                    [this, name](const std_msgs::msg::String::SharedPtr msg) {
                        on_telemetry(name, msg->data);
                    }));
        }

        // Fleet report timer — every 5 seconds
        timer_report_ = create_wall_timer(5s, [this]() { print_fleet_report(); });

        // Service — /fleet/status_report
        srv_report_ = create_service<std_srvs::srv::Trigger>(
            "/fleet/status_report",
            [this](const std_srvs::srv::Trigger::Request::SharedPtr,
                         std_srvs::srv::Trigger::Response::SharedPtr res) {
                print_fleet_report();
                res->success = true;
                res->message = "Fleet report triggered.";
            });

        RCLCPP_INFO(get_logger(), "FleetManager started — monitoring Alpha, Beta, Gamma");
    }

private:
    // ── Parse status string: "name:X|battery:Y|..." ──────────────────────────
    void on_status(const std::string& name, const std::string& data) {
        auto& s = fleet_[name];
        auto kv = [&](const std::string& key) -> std::string {
            std::string needle = key + ":";
            size_t pos = data.find(needle);
            if (pos == std::string::npos) return "";
            pos += needle.size();
            size_t end = data.find('|', pos);
            return data.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        };
        try { s.battery  = std::stod(kv("battery")); } catch (...) {}
        try { s.altitude = std::stod(kv("altitude")); } catch (...) {}
        s.status = kv("status");
        // waypoint: "2/5"
        std::string wp = kv("waypoint");
        size_t slash = wp.find('/');
        if (slash != std::string::npos) {
            try { s.wp_index = std::stoi(wp.substr(0, slash)); } catch (...) {}
            try { s.wp_total = std::stoi(wp.substr(slash + 1)); } catch (...) {}
        }
        s.last_update = now_str();
    }

    // ── Parse telemetry JSON ──────────────────────────────────────────────────
    void on_telemetry(const std::string& name, const std::string& json) {
        auto& s = fleet_[name];
        try { s.battery  = std::stod(json_extract(json, "battery")); }  catch (...) {}
        try { s.altitude = std::stod(json_extract(json, "altitude")); } catch (...) {}
        s.status  = json_extract(json, "status");
        s.mission = json_extract(json, "mission");
        std::string crit = json_extract(json, "critical");
        s.critical = (crit == "true");
        s.last_update = now_str();
    }

    // ── Alert handler ─────────────────────────────────────────────────────────
    void on_alert(const std::string& msg) {
        RCLCPP_WARN(get_logger(), "[%s] ⚠  ALERT: %s", now_str().c_str(), msg.c_str());
    }

    // ── Fleet report table ────────────────────────────────────────────────────
    void print_fleet_report() {
        std::ostringstream oss;
        oss << "\n╔══════════════════════════════════════════════════════════════════╗\n";
        oss << "║           FLEET REPORT  —  " << now_str() << "            ║\n";
        oss << "╠══════════╦═══════════╦══════════╦═══════════╦══════════════════╣\n";
        oss << "║  Drone   ║  Battery  ║ Altitude ║  Waypoint ║     Status       ║\n";
        oss << "╠══════════╬═══════════╬══════════╬═══════════╬══════════════════╣\n";
        for (const auto& [name, s] : fleet_) {
            std::ostringstream row;
            row << std::fixed << std::setprecision(1);
            row << "║ " << std::left << std::setw(8) << name
                << " ║ " << std::right << std::setw(7) << s.battery << "%  "
                << " ║ " << std::setw(6) << s.altitude << "m  "
                << " ║   " << s.wp_index << "/" << s.wp_total << "     "
                << " ║ " << std::left << std::setw(16) << s.status << " ║";
            oss << row.str() << "\n";
        }
        oss << "╚══════════╩═══════════╩══════════╩═══════════╩══════════════════╝\n";
        RCLCPP_INFO(get_logger(), "%s", oss.str().c_str());
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

    std::map<std::string, DroneState> fleet_;

    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs_status_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs_alert_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs_complete_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> subs_telem_;

    rclcpp::TimerBase::SharedPtr timer_report_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr srv_report_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FleetManagerNode>());
    rclcpp::shutdown();
    return 0;
}
