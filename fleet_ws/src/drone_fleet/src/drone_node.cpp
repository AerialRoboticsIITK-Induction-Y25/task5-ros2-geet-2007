#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "mission_drone.hpp"

using namespace std::chrono_literals;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string now_iso() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

// Minimal JSON builder — no external library
static std::string build_telemetry_json(const MissionDrone& d) {
    std::ostringstream j;
    j << std::fixed << std::setprecision(2);
    j << "{"
      << "\"name\":\"" << d.get_name() << "\","
      << "\"battery\":" << d.get_battery() << ","
      << "\"altitude\":" << d.get_altitude() << ","
      << "\"status\":\"" << d.get_status() << "\","
      << "\"waypoint_index\":" << d.get_current_waypoint_index() << ","
      << "\"total_waypoints\":" << d.get_total_waypoints() << ","
      << "\"mission\":\"" << d.get_mission_name() << "\","
      << "\"critical\":" << (d.is_critical() ? "true" : "false") << ","
      << "\"timestamp\":\"" << now_iso() << "\""
      << "}";
    return j.str();
}

// ── DroneNode ─────────────────────────────────────────────────────────────────

class DroneNode : public rclcpp::Node {
public:
    DroneNode() : Node("drone_node"), publish_count_(0) {
        // ── Declare & read parameters ─────────────────────────────────────────
        this->declare_parameter<std::string>("drone_name",      "Alpha");
        this->declare_parameter<double>     ("initial_battery", 100.0);
        this->declare_parameter<std::string>("mission_name",    "DefaultMission");

        drone_name_    = this->get_parameter("drone_name").as_string();
        auto battery   = static_cast<float>(this->get_parameter("initial_battery").as_double());
        auto mission   = this->get_parameter("mission_name").as_string();

        RCLCPP_INFO(get_logger(), "Starting DroneNode: %s | battery=%.1f | mission=%s",
                    drone_name_.c_str(), battery, mission.c_str());

        // ── Create MissionDrone with 5 waypoints ──────────────────────────────
        std::vector<Waypoint> waypoints = {
            {10.0f, 0.0f, 15.0f},
            {20.0f, 5.0f, 20.0f},
            {30.0f, 0.0f, 25.0f},
            {40.0f, 5.0f, 20.0f},
            {50.0f, 0.0f, 15.0f}
        };
        drone_ = std::make_unique<MissionDrone>(drone_name_, mission, waypoints, battery);

        // ── Topics ────────────────────────────────────────────────────────────
        std::string base = "/drone/" + drone_name_;
        pub_status_   = create_publisher<std_msgs::msg::String>(base + "/status",    10);
        pub_alert_    = create_publisher<std_msgs::msg::String>(base + "/alert",     10);
        pub_complete_ = create_publisher<std_msgs::msg::String>(base + "/mission_complete", 10);
        pub_telem_    = create_publisher<std_msgs::msg::String>(base + "/telemetry", 10);

        // ── Timers ────────────────────────────────────────────────────────────
        timer_status_ = create_wall_timer(1s,  [this]() { publish_status(); });
        timer_telem_  = create_wall_timer(2s,  [this]() { publish_telemetry(); });

        // Take off to initial altitude
        try { drone_->take_off(15.0f); }
        catch (const DroneException& e) {
            RCLCPP_WARN(get_logger(), "Take-off error: %s", e.what());
        }
    }

private:
    void publish_status() {
        ++publish_count_;

        // Drain battery every publish
        try { drone_->drain_battery(0.5f); }
        catch (const BatteryDepletedError&) {
            RCLCPP_ERROR(get_logger(), "[%s] Battery completely depleted!", drone_name_.c_str());
            return;
        }

        // Advance waypoint every 3 publishes
        if (publish_count_ % 3 == 0 && !drone_->mission_complete()) {
            try { drone_->next_waypoint(); }
            catch (const DroneException& e) {
                RCLCPP_WARN(get_logger(), "Waypoint advance error: %s", e.what());
            }
        }

        // Check critical battery
        if (drone_->is_critical()) {
            auto alert_msg = std_msgs::msg::String{};
            alert_msg.data = "[ALERT] " + drone_name_ + " battery critical: " +
                             std::to_string(drone_->get_battery());
            pub_alert_->publish(alert_msg);
            RCLCPP_WARN(get_logger(), "CRITICAL BATTERY: %.1f%%", drone_->get_battery());
            try { drone_->land(); } catch (...) {}
        }

        // Check mission complete
        if (drone_->mission_complete()) {
            auto msg = std_msgs::msg::String{};
            msg.data = "[MISSION_COMPLETE] " + drone_name_ + " finished " +
                       drone_->get_mission_name();
            pub_complete_->publish(msg);
            RCLCPP_INFO(get_logger(), "Mission complete! Restarting...");
            restart_mission();
        }

        // Build status string
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "name:"    << drone_->get_name()
            << "|battery:" << drone_->get_battery()
            << "|altitude:" << drone_->get_altitude()
            << "|status:"  << drone_->get_status()
            << "|waypoint:" << drone_->get_current_waypoint_index()
            << "/"         << drone_->get_total_waypoints()
            << "|speed:3.2";   // speed not publicly accessible; use nominal

        auto msg = std_msgs::msg::String{};
        msg.data = oss.str();
        pub_status_->publish(msg);
    }

    void publish_telemetry() {
        auto msg = std_msgs::msg::String{};
        msg.data = build_telemetry_json(*drone_);
        pub_telem_->publish(msg);
    }

    void restart_mission() {
        // Re-create drone preserving battery level
        float bat = drone_->get_battery();
        std::vector<Waypoint> waypoints = {
            {10.0f, 0.0f, 15.0f}, {20.0f, 5.0f, 20.0f},
            {30.0f, 0.0f, 25.0f}, {40.0f, 5.0f, 20.0f}, {50.0f, 0.0f, 15.0f}
        };
        drone_ = std::make_unique<MissionDrone>(drone_name_,
                     this->get_parameter("mission_name").as_string(),
                     waypoints, bat);
        try { drone_->take_off(15.0f); } catch (...) {}
    }

    std::string drone_name_;
    std::unique_ptr<MissionDrone> drone_;
    int publish_count_;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_status_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_alert_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_complete_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_telem_;

    rclcpp::TimerBase::SharedPtr timer_status_;
    rclcpp::TimerBase::SharedPtr timer_telem_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DroneNode>());
    rclcpp::shutdown();
    return 0;
}
