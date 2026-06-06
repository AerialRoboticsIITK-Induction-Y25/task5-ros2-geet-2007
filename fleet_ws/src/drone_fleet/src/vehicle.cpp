#include "vehicle.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

const std::vector<std::string> Vehicle::ALLOWED_STATES = {"idle", "flying", "charging"};

// ── Helpers ──────────────────────────────────────────────────────────────────

std::string Vehicle::timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Vehicle::log(const std::string& entry) {
    flight_log_.push_back("[" + timestamp() + "] " + entry);
}

// ── Constructor ───────────────────────────────────────────────────────────────

Vehicle::Vehicle(const std::string& name, float battery)
    : name_(name), battery_level_(battery), status_("idle") {
    log("Vehicle created with battery " + std::to_string(battery));
}

// ── Status ────────────────────────────────────────────────────────────────────

void Vehicle::set_status(const std::string& new_status) {
    if (std::find(ALLOWED_STATES.begin(), ALLOWED_STATES.end(), new_status) == ALLOWED_STATES.end()) {
        throw InvalidStateError("Unknown status: " + new_status);
    }
    log("Status changed: " + status_ + " → " + new_status);
    status_ = new_status;
}

// ── Battery ───────────────────────────────────────────────────────────────────

void Vehicle::drain_battery(float amount) {
    if (battery_level_ <= 0.0f) {
        throw BatteryDepletedError("Battery already at 0 for drone '" + name_ + "'");
    }
    battery_level_ = std::max(0.0f, battery_level_ - amount);
    log("Battery drained by " + std::to_string(amount) + " → " + std::to_string(battery_level_));
}

void Vehicle::charge_battery(float amount, int duration_seconds) {
    if (status_ != "charging") {
        throw InvalidStateError("Cannot charge while status is '" + status_ +
                                "'. Set status to 'charging' first.");
    }
    battery_level_ = std::min(100.0f, battery_level_ + amount);
    log("Battery charged by " + std::to_string(amount) + " over " +
        std::to_string(duration_seconds) + "s → " + std::to_string(battery_level_));
}

bool Vehicle::is_critical() const {
    return battery_level_ < 20.0f;
}

// ── Flight log ────────────────────────────────────────────────────────────────

std::string Vehicle::get_flight_log() const {
    std::ostringstream oss;
    oss << "=== Flight Log for " << name_ << " ===\n";
    for (const auto& entry : flight_log_) {
        oss << entry << "\n";
    }
    return oss.str();
}
