#include "mission_drone.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

MissionDrone::MissionDrone(const std::string& name,
                           const std::string& mission_name,
                           const std::vector<Waypoint>& waypoints,
                           float battery, float max_alt, float speed)
    : Drone(name, battery, max_alt, speed),
      mission_name_(mission_name),
      waypoints_(waypoints),
      current_waypoint_index_(0) {}

Waypoint MissionDrone::next_waypoint() {
    if (mission_complete()) {
        throw std::out_of_range("Mission already complete for '" + get_name() + "'");
    }
    Waypoint wp = waypoints_[current_waypoint_index_];
    visited_waypoints_.push_back({wp, timestamp()});
    log("Reached waypoint " + std::to_string(current_waypoint_index_) +
        " (" + std::to_string(std::get<0>(wp)) + "," +
               std::to_string(std::get<1>(wp)) + "," +
               std::to_string(std::get<2>(wp)) + ")");
    drain_battery(1.5f);
    ++current_waypoint_index_;
    return wp;
}

void MissionDrone::skip_waypoint(const std::string& reason) {
    if (mission_complete()) return;
    log("Skipped waypoint " + std::to_string(current_waypoint_index_) +
        " — reason: " + reason);
    ++current_waypoint_index_;
}

bool MissionDrone::mission_complete() const {
    return current_waypoint_index_ >= static_cast<int>(waypoints_.size());
}

std::string MissionDrone::mission_summary() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Mission Summary: " << mission_name_ << " ===\n"
        << "Drone      : " << get_name() << "\n"
        << "Waypoints  : " << waypoints_.size() << " total, "
        << visited_waypoints_.size() << " visited\n"
        << "Complete   : " << (mission_complete() ? "YES" : "NO") << "\n"
        << "Battery    : " << get_battery() << "%\n"
        << "Visited log:\n";
    for (size_t i = 0; i < visited_waypoints_.size(); ++i) {
        const auto& [wp, ts] = visited_waypoints_[i];
        oss << "  [" << i << "] (" << std::get<0>(wp) << ","
            << std::get<1>(wp) << "," << std::get<2>(wp) << ")  @" << ts << "\n";
    }
    return oss.str();
}

std::string MissionDrone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[MissionDrone] Name=" << get_name()
        << " Mission=" << mission_name_
        << " Waypoint=" << current_waypoint_index_ << "/" << waypoints_.size()
        << " Battery=" << get_battery()
        << " Status=" << get_status()
        << " Altitude=" << get_altitude()
        << " Complete=" << (mission_complete() ? "YES" : "no");
    return oss.str();
}
