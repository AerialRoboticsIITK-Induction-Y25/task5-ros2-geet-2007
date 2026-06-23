#pragma once
#include "drone_fleet/vehicle.hpp"

class Drone : public Vehicle {
public:
    Drone(const std::string& name, float battery = 100.0f,
          float max_alt = 100.0f, float speed = 5.0f);
    virtual ~Drone() = default;

    void take_off(float target_altitude);
    void land();
    void emergency_stop();

    float get_altitude() const { return altitude_; }
    float get_speed() const { return speed_; }
    float get_max_altitude() const { return max_altitude_; }

    std::string get_info() const override;

protected:
    float altitude_;
    float max_altitude_;

private:
    float speed_;
};
