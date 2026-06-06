#pragma once
#include <string>
#include <vector>
#include "drone_exceptions.hpp"

class Vehicle {
public:
    explicit Vehicle(const std::string& name, float battery = 100.0f);
    virtual ~Vehicle() = default;

    // Pure virtual — every subclass must implement
    virtual std::string get_info() const = 0;

    // Battery operations
    void drain_battery(float amount);
    void charge_battery(float amount, int duration_seconds);
    bool is_critical() const;

    // Flight log
    std::string get_flight_log() const;

    // Getters (no public setters for battery/status)
    const std::string& get_name()   const { return name_; }
    float              get_battery() const { return battery_level_; }
    const std::string& get_status() const { return status_; }

protected:
    std::string name_;

    // Status setter with validation + logging (accessible to subclasses)
    void set_status(const std::string& new_status);
    void log(const std::string& entry);

    static std::string timestamp();

private:
    float battery_level_;           // 0.0 – 100.0
    std::string status_;            // "idle" | "flying" | "charging"
    std::vector<std::string> flight_log_;

    static const std::vector<std::string> ALLOWED_STATES;
};
