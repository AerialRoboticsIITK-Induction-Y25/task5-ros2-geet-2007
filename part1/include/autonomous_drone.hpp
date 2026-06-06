#pragma once
#include "mission_drone.hpp"
#include <deque>

class AutonomousDrone : public MissionDrone {
public:
    AutonomousDrone(const std::string& name,
                    const std::string& mission_name,
                    const std::vector<Waypoint>& waypoints,
                    const Waypoint& home_position,
                    float battery = 100.0f,
                    float max_alt = 120.0f,
                    float speed   = 5.0f);

    void set_ai_mode(const std::string& mode);
    void detect_obstacle(Waypoint position, const std::string& severity);
    std::vector<Waypoint> auto_replan(const std::vector<Waypoint>& obstacles);

    const std::string& get_ai_mode()       const { return ai_mode_; }
    const Waypoint&    get_home_position() const { return home_position_; }

    std::string get_info() const override;

private:
    std::string              ai_mode_;       // "manual" | "auto" | "return_home"
    Waypoint                 home_position_;
    std::vector<std::string> obstacle_log_;  // private — timestamped entries

    static float distance3d(const Waypoint& a, const Waypoint& b);
};
