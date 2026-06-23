#include "drone_fleet/drone.hpp"
#include <sstream>
#include <iomanip>

Drone::Drone(const std::string& name, float battery, float max_alt, float speed)
    : Vehicle(name, battery), altitude_(0.0f), max_altitude_(max_alt), speed_(speed) {}

void Drone::take_off(float target_altitude) {
    if (target_altitude > max_altitude_) {
        throw AltitudeError("Target altitude " + std::to_string(target_altitude) +
                            " exceeds max altitude " + std::to_string(max_altitude_));
    }
    set_status("flying");
    altitude_ = target_altitude;
    add_log("Took off to altitude: " + std::to_string(altitude_));
}

void Drone::land() {
    set_status("idle");
    altitude_ = 0.0f;
    add_log("Landed successfully");
}

void Drone::emergency_stop() {
    drain_battery(30.0f);
    land();
    add_log("EMERGENCY STOP triggered — battery drained by 30");
}

std::string Drone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[Drone] " << get_name()
        << " | Battery: " << get_battery()
        << "% | Status: " << get_status()
        << " | Altitude: " << altitude_
        << "m | Speed: " << speed_ << "m/s";
    return oss.str();
}
