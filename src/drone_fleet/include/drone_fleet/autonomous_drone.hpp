#pragma once
#include "drone_fleet/mission_drone.hpp"

class AutonomousDrone : public MissionDrone {
public:
    AutonomousDrone(const std::string& name, const std::string& mission_name,
                    const std::vector<Waypoint>& waypoints,
                    const Waypoint& home_position,
                    float battery = 100.0f);
    virtual ~AutonomousDrone() = default;

    void set_ai_mode(const std::string& mode);
    void detect_obstacle(const Waypoint& position, const std::string& severity);
    std::vector<Waypoint> auto_replan(const std::vector<Waypoint>& obstacles);

    std::string get_ai_mode() const { return ai_mode_; }
    std::string get_info() const override;

private:
    std::string ai_mode_;
    Waypoint home_position_;
    std::vector<std::string> obstacle_log_;

    static std::string current_timestamp();
    static float distance(const Waypoint& a, const Waypoint& b);
};
