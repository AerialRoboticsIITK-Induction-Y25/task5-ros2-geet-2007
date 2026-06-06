#include <iostream>
#include <vector>
#include <memory>
#include "vehicle.hpp"
#include "drone.hpp"
#include "mission_drone.hpp"
#include "autonomous_drone.hpp"

int main() {
    std::cout << "========================================\n";
    std::cout << "   DRONE FLEET OOP DEMO\n";
    std::cout << "========================================\n\n";

    // ── 1. Create one object of each class ───────────────────────────────────
    auto* v  = new Drone("VehicleDemo", 80.0f);
    auto* d  = new Drone("BasicDrone", 90.0f);

    std::vector<Waypoint> md_waypoints = {
        {10,10,15}, {20,20,20}, {30,10,15}
    };
    auto* md = new MissionDrone("MissionHawk", "Patrol-A", md_waypoints, 85.0f);

    std::vector<Waypoint> auto_waypoints = {
        {5,5,10}, {15,5,20}, {25,5,30}, {35,5,20}, {45,5,10}
    };
    Waypoint home = {0.0f, 0.0f, 0.0f};
    auto* ad = new AutonomousDrone("AutonomousEagle", "Scout-B",
                                   auto_waypoints, home, 100.0f, 150.0f, 8.0f);

    // ── 2. Polymorphism via std::vector<Vehicle*> ─────────────────────────────
    std::vector<Vehicle*> fleet = {v, d, md, ad};
    std::cout << "--- Polymorphic get_info() ---\n";
    for (const auto& drone : fleet) {
        std::cout << drone->get_info() << "\n";
    }
    std::cout << "\n";

    // ── 3. Private members cannot be accessed directly ────────────────────────
    // The following lines would cause compile errors if uncommented:
    // v->battery_level_ = 50.0f;   // ERROR: 'battery_level_' is private
    // v->status_ = "flying";        // ERROR: 'status_' is private
    // v->flight_log_.clear();       // ERROR: 'flight_log_' is private
    std::cout << "// Private members are inaccessible (see comments in main.cpp)\n\n";

    // ── 4. drain_battery() with exception handling ────────────────────────────
    std::cout << "--- drain_battery() demo ---\n";
    try {
        d->drain_battery(200.0f); // drain to 0
        d->drain_battery(1.0f);   // already at 0 — should throw
    } catch (const BatteryDepletedError& e) {
        std::cout << "[BatteryDepletedError caught] " << e.what() << "\n";
    } catch (const DroneException& e) {
        std::cout << "[DroneException caught] " << e.what() << "\n";
    }

    // ── 5. take_off() with AltitudeError ─────────────────────────────────────
    std::cout << "\n--- take_off() demo ---\n";
    try {
        md->take_off(500.0f); // exceeds max_altitude of 120
    } catch (const AltitudeError& e) {
        std::cout << "[AltitudeError caught] " << e.what() << "\n";
    }
    // Valid take-off
    try {
        md->take_off(30.0f);
        std::cout << "MissionHawk took off to 30m. " << md->get_info() << "\n";
    } catch (const DroneException& e) {
        std::cout << "[Error] " << e.what() << "\n";
    }

    // ── 6. charge_battery() with InvalidStateError ───────────────────────────
    std::cout << "\n--- charge_battery() demo ---\n";
    try {
        d->charge_battery(20.0f, 60); // status is "idle", not "charging"
    } catch (const InvalidStateError& e) {
        std::cout << "[InvalidStateError caught] " << e.what() << "\n";
    }

    // ── 7. detect_obstacle() with auto exception chain ───────────────────────
    std::cout << "\n--- detect_obstacle() demo ---\n";
    try {
        ad->take_off(50.0f);
        ad->detect_obstacle({15, 5, 20}, "low");
        std::cout << "Low obstacle logged.\n";
        ad->detect_obstacle({25, 5, 30}, "high"); // triggers emergency_stop
    } catch (const BatteryDepletedError& e) {
        std::cout << "[BatteryDepletedError] " << e.what() << "\n";
    } catch (const DroneException& e) {
        std::cout << "[DroneException after high obstacle] " << e.what() << "\n";
    }

    // ── 8. Full AutonomousDrone mission run ───────────────────────────────────
    std::cout << "\n--- Full AutonomousDrone Mission ---\n";

    // Fresh drone for clean mission
    std::vector<Waypoint> mission_wps = {
        {10,0,20}, {20,0,30}, {30,0,25}, {40,0,15}, {50,0,10}
    };
    AutonomousDrone eagle("EagleFinal", "FullMission", mission_wps, home, 100.0f, 150.0f, 8.0f);

    try {
        eagle.take_off(20.0f);
        eagle.set_ai_mode("auto");

        while (!eagle.mission_complete()) {
            Waypoint wp = eagle.next_waypoint();
            std::cout << "  Reached waypoint ("
                      << std::get<0>(wp) << ","
                      << std::get<1>(wp) << ","
                      << std::get<2>(wp) << ")  battery=" << eagle.get_battery() << "\n";
        }

        // Simulate a high-severity obstacle mid-air
        std::cout << "\n  Simulating high-severity obstacle...\n";
        eagle.take_off(20.0f); // take off again after emergency land
        eagle.detect_obstacle({25, 0, 25}, "high");

    } catch (const BatteryDepletedError& e) {
        std::cout << "[BatteryDepletedError during mission] " << e.what() << "\n";
    } catch (const DroneException& e) {
        std::cout << "[DroneException during mission] " << e.what() << "\n";
    }

    std::cout << "\n" << eagle.mission_summary();
    std::cout << "\n" << eagle.get_info() << "\n";

    // ── 9. auto_replan() demo ─────────────────────────────────────────────────
    std::cout << "\n--- auto_replan() demo ---\n";
    std::vector<Waypoint> new_wps = {
        {5,5,10}, {10,10,20}, {100,100,30}, {200,200,40}, {300,300,50}
    };
    AutonomousDrone planner("Planner", "ReplanTest", new_wps, home, 100.0f);
    std::vector<Waypoint> obstacles = {{10,10,20}, {200,200,40}}; // these two should be removed
    auto safe = planner.auto_replan(obstacles);
    std::cout << "Safe waypoints after replanning: " << safe.size() << " (expected 3)\n";

    // ── Cleanup ───────────────────────────────────────────────────────────────
    for (auto* ptr : fleet) delete ptr;

    std::cout << "\n========================================\n";
    std::cout << "   ALL DEMOS COMPLETE\n";
    std::cout << "========================================\n";
    return 0;
}
