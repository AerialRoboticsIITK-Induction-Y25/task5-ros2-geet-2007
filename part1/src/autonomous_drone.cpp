#include "autonomous_drone.hpp"
#include <sstream>
#include <cmath>
#include <iomanip>

AutonomousDrone::AutonomousDrone(const std::string& name,
                                  const std::string& mission_name,
                                  const std::vector<Waypoint>& waypoints,
                                  const Waypoint& home_position,
                                  float battery, float max_alt, float speed)
    : MissionDrone(name, mission_name, waypoints, battery, max_alt, speed),
      ai_mode_("manual"),
      home_position_(home_position) {}

float AutonomousDrone::distance3d(const Waypoint& a, const Waypoint& b) {
    float dx = std::get<0>(a) - std::get<0>(b);
    float dy = std::get<1>(a) - std::get<1>(b);
    float dz = std::get<2>(a) - std::get<2>(b);
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

void AutonomousDrone::set_ai_mode(const std::string& mode) {
    if (mode != "manual" && mode != "auto" && mode != "return_home") {
        throw InvalidStateError("Unknown AI mode: " + mode);
    }
    ai_mode_ = mode;
    log("AI mode set to: " + mode);
    if (mode == "return_home") {
        // Insert home position as the NEXT waypoint
        waypoints_.insert(waypoints_.begin() + current_waypoint_index_, home_position_);
        log("Home position inserted as next waypoint");
    }
}

void AutonomousDrone::detect_obstacle(Waypoint position, const std::string& severity) {
    std::string entry = "[" + timestamp() + "] OBSTACLE detected at ("
        + std::to_string(std::get<0>(position)) + ","
        + std::to_string(std::get<1>(position)) + ","
        + std::to_string(std::get<2>(position)) + ") severity=" + severity;
    obstacle_log_.push_back(entry);
    log("Obstacle detected: severity=" + severity);

    if (severity == "high") {
        log("High-severity obstacle — triggering emergency stop");
        emergency_stop();
    }
}

std::vector<Waypoint> AutonomousDrone::auto_replan(const std::vector<Waypoint>& obstacles) {
    std::vector<Waypoint> safe_waypoints;
    for (const auto& wp : waypoints_) {
        bool blocked = false;
        for (const auto& obs : obstacles) {
            if (distance3d(wp, obs) < 5.0f) {
                blocked = true;
                log("Waypoint (" + std::to_string(std::get<0>(wp)) + "," +
                    std::to_string(std::get<1>(wp)) + "," +
                    std::to_string(std::get<2>(wp)) + ") removed — within 5 units of obstacle");
                break;
            }
        }
        if (!blocked) safe_waypoints.push_back(wp);
    }
    waypoints_ = safe_waypoints;
    // Reset index if out of bounds after replanning
    if (current_waypoint_index_ > static_cast<int>(waypoints_.size())) {
        current_waypoint_index_ = static_cast<int>(waypoints_.size());
    }
    log("Replanned: " + std::to_string(safe_waypoints.size()) + " safe waypoints remain");
    return safe_waypoints;
}

std::string AutonomousDrone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[AutonomousDrone] Name=" << get_name()
        << " Mission=" << get_mission_name()
        << " AIMode=" << ai_mode_
        << " Waypoint=" << get_current_waypoint_index() << "/" << get_total_waypoints()
        << " Battery=" << get_battery()
        << " Status=" << get_status()
        << " Altitude=" << get_altitude()
        << " Home=(" << std::get<0>(home_position_) << ","
                     << std::get<1>(home_position_) << ","
                     << std::get<2>(home_position_) << ")"
        << " Critical=" << (is_critical() ? "YES" : "no");
    return oss.str();
}
