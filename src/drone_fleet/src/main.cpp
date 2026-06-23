#include <iostream>
#include <vector>
#include "drone_fleet/vehicle.hpp"
#include "drone_fleet/drone.hpp"
#include "drone_fleet/mission_drone.hpp"
#include "drone_fleet/autonomous_drone.hpp"
#include "drone_fleet/exceptions.hpp"

int main() {
    // --- Polymorphism demo ---
    std::vector<Waypoint> waypoints = {
        {0.0f, 0.0f, 10.0f},
        {10.0f, 5.0f, 15.0f},
        {20.0f, 10.0f, 20.0f},
        {30.0f, 5.0f, 15.0f},
        {40.0f, 0.0f, 10.0f}
    };
    Waypoint home = {0.0f, 0.0f, 0.0f};

    Drone base_drone("BaseDrone", 80.0f);
    MissionDrone m_drone("Scout", "Recon-1", waypoints, 75.0f);
    AutonomousDrone a_drone("Alpha", "FullAuto", waypoints, home, 90.0f);

    std::vector<Vehicle*> fleet = {&base_drone, &m_drone, &a_drone};

    std::cout << "=== Fleet Info (Polymorphism) ===\n";
    for (auto* v : fleet) {
        std::cout << v->get_info() << "\n";
    }

    // Private members cannot be accessed directly:
    // base_drone.battery_level_ = 50.0f;  // ERROR: battery_level_ is private
    // base_drone.status_ = "flying";       // ERROR: status_ is private
    // Access must go through getters/methods only.

    // --- Exception handling demo ---
    std::cout << "\n=== Exception Handling Demo ===\n";

    // drain_battery
    try {
        base_drone.drain_battery(90.0f);  // drains to ~0
        base_drone.drain_battery(5.0f);   // should throw
    } catch (const BatteryDepletedError& e) {
        std::cout << "Caught BatteryDepletedError: " << e.what() << "\n";
    }

    // take_off altitude error
    try {
        Drone limited("LimitedDrone", 100.0f, 50.0f);
        limited.take_off(200.0f);  // exceeds max
    } catch (const AltitudeError& e) {
        std::cout << "Caught AltitudeError: " << e.what() << "\n";
    }

    // detect_obstacle
    try {
        a_drone.take_off(20.0f);
        a_drone.detect_obstacle({5.0f, 5.0f, 15.0f}, "high");
    } catch (const DroneException& e) {
        std::cout << "Caught DroneException during obstacle: " << e.what() << "\n";
    }

    // --- Full mission on AutonomousDrone ---
    std::cout << "\n=== Full Mission Run ===\n";
    AutonomousDrone mission_drone("Gamma", "Survey-9", waypoints, home, 100.0f);
    try {
        mission_drone.take_off(15.0f);
        while (!mission_drone.mission_complete()) {
            auto wp = mission_drone.next_waypoint();
            std::cout << "Visited waypoint: ("
                      << std::get<0>(wp) << ", "
                      << std::get<1>(wp) << ", "
                      << std::get<2>(wp) << ")\n";
        }
        mission_drone.detect_obstacle({10.0f, 5.0f, 15.0f}, "high");
    } catch (const DroneException& e) {
        std::cout << "Mission exception: " << e.what() << "\n";
    }

    std::cout << "\n" << mission_drone.mission_summary();
    std::cout << "\n" << mission_drone.get_flight_log();

    return 0;
}
