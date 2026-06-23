#include "drone_fleet/vehicle.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <set>

static const std::set<std::string> VALID_STATES = {"idle", "flying", "charging"};

Vehicle::Vehicle(const std::string& name, float battery)
    : name_(name), battery_level_(battery), status_("idle") {}

std::string Vehicle::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Vehicle::set_status(const std::string& new_status) {
    if (VALID_STATES.find(new_status) == VALID_STATES.end()) {
        throw InvalidStateError("Invalid status: " + new_status);
    }
    status_ = new_status;
    add_log("[" + current_timestamp() + "] Status changed to: " + new_status);
}

void Vehicle::add_log(const std::string& entry) {
    flight_log_.push_back(entry);
}

void Vehicle::drain_battery(float amount) {
    if (battery_level_ <= 0.0f) {
        throw BatteryDepletedError("Battery already depleted for " + name_);
    }
    battery_level_ -= amount;
    if (battery_level_ < 0.0f) battery_level_ = 0.0f;
}

void Vehicle::charge_battery(float amount, int duration_seconds) {
    if (status_ != "charging") {
        throw InvalidStateError(name_ + " must be in charging state to charge battery");
    }
    battery_level_ += amount;
    if (battery_level_ > 100.0f) battery_level_ = 100.0f;
    add_log("[" + current_timestamp() + "] Charged for " +
            std::to_string(duration_seconds) + "s, battery now: " +
            std::to_string(battery_level_));
}

bool Vehicle::is_critical() const {
    return battery_level_ < 20.0f;
}

std::string Vehicle::get_flight_log() const {
    std::ostringstream oss;
    oss << "=== Flight Log for " << name_ << " ===\n";
    for (const auto& entry : flight_log_) {
        oss << entry << "\n";
    }
    return oss.str();
}
