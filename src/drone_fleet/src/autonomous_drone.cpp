#include "drone_fleet/autonomous_drone.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cmath>

AutonomousDrone::AutonomousDrone(const std::string& name, const std::string& mission_name,
                                 const std::vector<Waypoint>& waypoints,
                                 const Waypoint& home_position, float battery)
    : MissionDrone(name, mission_name, waypoints, battery),
      ai_mode_("manual"), home_position_(home_position) {}

std::string AutonomousDrone::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

float AutonomousDrone::distance(const Waypoint& a, const Waypoint& b) {
    float dx = std::get<0>(a) - std::get<0>(b);
    float dy = std::get<1>(a) - std::get<1>(b);
    float dz = std::get<2>(a) - std::get<2>(b);
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

void AutonomousDrone::set_ai_mode(const std::string& mode) {
    if (mode != "manual" && mode != "auto" && mode != "return_home") {
        throw InvalidStateError("Invalid AI mode: " + mode);
    }
    ai_mode_ = mode;
    if (mode == "return_home") {
        waypoints_.insert(waypoints_.begin() + current_waypoint_index_, home_position_);
        add_log("[" + current_timestamp() + "] Return home inserted as next waypoint");
    }
}

void AutonomousDrone::detect_obstacle(const Waypoint& position, const std::string& severity) {
    std::ostringstream entry;
    entry << std::fixed << std::setprecision(1);
    entry << "[" << current_timestamp() << "] Obstacle at ("
          << std::get<0>(position) << ", " << std::get<1>(position) << ", "
          << std::get<2>(position) << ") severity=" << severity;
    obstacle_log_.push_back(entry.str());
    add_log(entry.str());
    if (severity == "high") {
        emergency_stop();
    }
}

std::vector<Waypoint> AutonomousDrone::auto_replan(const std::vector<Waypoint>& obstacles) {
    std::vector<Waypoint> new_plan;
    for (const auto& wp : waypoints_) {
        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (distance(wp, obs) < 5.0f) {
                blocked = true;
                break;
            }
        }
        if (!blocked) {
            new_plan.push_back(wp);
        }
    }
    return new_plan;
}

std::string AutonomousDrone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[AutonomousDrone] " << get_name()
        << " | Mission: " << mission_name_
        << " | AI Mode: " << ai_mode_
        << " | Battery: " << get_battery()
        << "% | Status: " << get_status()
        << " | Altitude: " << altitude_
        << "m | Waypoint: " << current_waypoint_index_
        << "/" << waypoints_.size()
        << " | Obstacles logged: " << obstacle_log_.size();
    return oss.str();
}
