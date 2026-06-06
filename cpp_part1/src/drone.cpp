#include "drone.hpp"
#include <sstream>
#include <iomanip>

Drone::Drone(const std::string& name, float battery, float max_alt, float speed)
    : Vehicle(name, battery), altitude_(0.0f), max_altitude_(max_alt), speed_(speed) {}

void Drone::take_off(float target_altitude) {
    if (target_altitude > max_altitude_) {
        throw AltitudeError("Target altitude " + std::to_string(target_altitude) +
                            " exceeds max " + std::to_string(max_altitude_) +
                            " for drone '" + get_name() + "'");
    }
    set_status("flying");
    altitude_ = target_altitude;
    log("Took off to altitude " + std::to_string(target_altitude));
}

void Drone::land() {
    altitude_ = 0.0f;
    set_status("idle");
    log("Landed successfully");
}

void Drone::emergency_stop() {
    log("EMERGENCY STOP triggered — draining 30 battery units as penalty");
    drain_battery(30.0f);
    land();
}

std::string Drone::get_info() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "[Drone] Name=" << get_name()
        << " Battery=" << get_battery()
        << " Status=" << get_status()
        << " Altitude=" << altitude_
        << " MaxAlt=" << max_altitude_
        << " Speed=" << speed_
        << " Critical=" << (is_critical() ? "YES" : "no");
    return oss.str();
}
