#include "drone_fleet/mission_drone.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

MissionDrone::MissionDrone(const std::string& name, const std::string& mission_name,
                           const std::vector<Waypoint>& waypoints, float battery)
    : Drone(name, battery), mission_name_(mission_name),
      waypoints_(waypoints), current_waypoint_index_(0) {}

std::string MissionDrone::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

Waypoint MissionDrone::next_waypoint() {
    if (mission_complete()) {
        throw InvalidStateError("Mission already complete for " + name_);
    }
    drain_battery(1.5f);
    Waypoint wp = waypoints_[current_waypoint_index_];
    visited_waypoints_.push_back({wp, current_timestamp()});
    current_waypoint_index_++;
    return wp;
}

void MissionDrone::skip_waypoint(const std::string& reason) {
    if (mission_complete()) return;
    Waypoint wp = waypoints_[current_waypoint_index_];
    add_log("Skipped waypoint " + std::to_string(current_waypoint_index_) +
            " at [" + current_timestamp() + "] Reason: " + reason);
    visited_waypoints_.push_back({wp, "SKIPPED: " + reason});
    current_waypoint_index_++;
}

bool MissionDrone::mission_complete() const {
    return current_waypoint_index_ >= static_cast<int>(waypoints_.size());
}

std::string MissionDrone::mission_summary() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Mission Summary: " << mission_name_ << " ===\n";
    oss << "Drone: " << name_ << "\n";
    oss << "Waypoints visited: " << visited_waypoints_.size()
        << "/" << waypoints_.size() << "\n";
    for (size_t i = 0; i < visited_waypoints_.size(); i++) {
        const auto& [wp, ts] = visited_waypoints_[i];
        oss << "  [" << i << "] ("
            << std::get<0>(wp) << ", " << std::get<1>(wp) << ", " << std::get<2>(wp)
            << ") @ " << ts << "\n";
    }
    oss << "Battery remaining: " << get_battery() << "%\n";
    return oss.str();
}

std::string MissionDrone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[MissionDrone] " << get_name()
        << " | Mission: " << mission_name_
        << " | Battery: " << get_battery()
        << "% | Status: " << get_status()
        << " | Altitude: " << altitude_
        << "m | Waypoint: " << current_waypoint_index_
        << "/" << waypoints_.size();
    return oss.str();
}
