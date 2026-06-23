#pragma once
#include "drone_fleet/drone.hpp"
#include <tuple>
#include <vector>

using Waypoint = std::tuple<float, float, float>;
using VisitedWaypoint = std::pair<Waypoint, std::string>;

class MissionDrone : public Drone {
public:
    MissionDrone(const std::string& name, const std::string& mission_name,
                 const std::vector<Waypoint>& waypoints,
                 float battery = 100.0f);
    virtual ~MissionDrone() = default;

    Waypoint next_waypoint();
    void skip_waypoint(const std::string& reason);
    bool mission_complete() const;
    std::string mission_summary() const;

    int get_waypoint_index() const { return current_waypoint_index_; }
    int get_waypoint_count() const { return static_cast<int>(waypoints_.size()); }
    std::string get_mission_name() const { return mission_name_; }

    std::string get_info() const override;

protected:
    std::string mission_name_;
    std::vector<Waypoint> waypoints_;
    int current_waypoint_index_;

private:
    std::vector<VisitedWaypoint> visited_waypoints_;
    static std::string current_timestamp();
};
