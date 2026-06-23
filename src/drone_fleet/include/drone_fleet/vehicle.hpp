#pragma once
#include <string>
#include <vector>
#include "drone_fleet/exceptions.hpp"

class Vehicle {
public:
    explicit Vehicle(const std::string& name, float battery = 100.0f);
    virtual ~Vehicle() = default;

    virtual std::string get_info() const = 0;

    void drain_battery(float amount);
    void charge_battery(float amount, int duration_seconds);
    bool is_critical() const;
    std::string get_flight_log() const;

    std::string get_name() const { return name_; }
    float get_battery() const { return battery_level_; }
    std::string get_status() const { return status_; }

protected:
    void set_status(const std::string& new_status);
    void add_log(const std::string& entry);

    std::string name_;

private:
    float battery_level_;
    std::string status_;
    std::vector<std::string> flight_log_;

    static std::string current_timestamp();
};
