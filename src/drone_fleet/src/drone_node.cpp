#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include "drone_fleet/mission_drone.hpp"
#include "drone_fleet/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

class DroneNode : public rclcpp::Node {
public:
    DroneNode() : Node("drone_node"), publish_count_(0) {
        this->declare_parameter("drone_name", std::string("Alpha"));
        this->declare_parameter("initial_battery", 100.0);
        this->declare_parameter("mission_name", std::string("DefaultMission"));

        std::string name = this->get_parameter("drone_name").as_string();
        double battery = this->get_parameter("initial_battery").as_double();
        std::string mission = this->get_parameter("mission_name").as_string();

        std::vector<Waypoint> wps = {
            {0.0f, 0.0f, 10.0f},
            {10.0f, 5.0f, 15.0f},
            {20.0f, 10.0f, 20.0f},
            {30.0f, 5.0f, 15.0f},
            {40.0f, 0.0f, 10.0f}
        };

        drone_ = std::make_unique<MissionDrone>(name, mission, wps, static_cast<float>(battery));

        try { drone_->take_off(15.0f); } catch (...) {}

        status_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/drone/" + name + "/status", 10);
        alert_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/drone/" + name + "/alert", 10);
        complete_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/drone/" + name + "/mission_complete", 10);
        telemetry_pub_ = this->create_publisher<std_msgs::msg::String>(
            "/drone/" + name + "/telemetry", 10);

        status_timer_ = this->create_wall_timer(
            std::chrono::seconds(1),
            std::bind(&DroneNode::publish_status, this));
        telemetry_timer_ = this->create_wall_timer(
            std::chrono::seconds(2),
            std::bind(&DroneNode::publish_telemetry, this));
    }

private:
    std::unique_ptr<MissionDrone> drone_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr complete_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;
    rclcpp::TimerBase::SharedPtr status_timer_;
    rclcpp::TimerBase::SharedPtr telemetry_timer_;
    int publish_count_;

    void publish_status() {
        try {
            drone_->drain_battery(0.5f);
        } catch (const BatteryDepletedError&) {}

        publish_count_++;
        if (publish_count_ % 3 == 0 && !drone_->mission_complete()) {
            try { drone_->next_waypoint(); } catch (...) {}
        }

        if (drone_->mission_complete()) {
            auto msg = std_msgs::msg::String();
            msg.data = drone_->get_name() + " mission complete";
            complete_pub_->publish(msg);
            // restart mission
            std::vector<Waypoint> wps = {
                {0.0f, 0.0f, 10.0f}, {10.0f, 5.0f, 15.0f},
                {20.0f, 10.0f, 20.0f}, {30.0f, 5.0f, 15.0f}, {40.0f, 0.0f, 10.0f}
            };
            drone_ = std::make_unique<MissionDrone>(
                drone_->get_name(), drone_->get_mission_name(), wps, drone_->get_battery());
            try { drone_->take_off(15.0f); } catch (...) {}
        }

        if (drone_->is_critical()) {
            auto alert = std_msgs::msg::String();
            alert.data = "CRITICAL BATTERY: " + drone_->get_name() +
                         " at " + std::to_string(drone_->get_battery()) + "%";
            alert_pub_->publish(alert);
            try { drone_->land(); } catch (...) {}
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "name:" << drone_->get_name()
            << "|battery:" << drone_->get_battery()
            << "|altitude:" << drone_->get_altitude()
            << "|status:" << drone_->get_status()
            << "|waypoint:" << drone_->get_waypoint_index()
            << "/" << drone_->get_waypoint_count()
            << "|speed:3.2";

        auto msg = std_msgs::msg::String();
        msg.data = oss.str();
        status_pub_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "%s", msg.data.c_str());
    }

    void publish_telemetry() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ts;
        ts << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");

        std::ostringstream json;
        json << std::fixed << std::setprecision(2);
        json << "{"
             << "\"name\":\"" << drone_->get_name() << "\","
             << "\"battery\":" << drone_->get_battery() << ","
             << "\"altitude\":" << drone_->get_altitude() << ","
             << "\"status\":\"" << drone_->get_status() << "\","
             << "\"waypoint\":" << drone_->get_waypoint_index() << ","
             << "\"total_waypoints\":" << drone_->get_waypoint_count() << ","
             << "\"mission\":\"" << drone_->get_mission_name() << "\","
             << "\"timestamp\":\"" << ts.str() << "\""
             << "}";

        auto msg = std_msgs::msg::String();
        msg.data = json.str();
        telemetry_pub_->publish(msg);
    }
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DroneNode>());
    rclcpp::shutdown();
    return 0;
}
