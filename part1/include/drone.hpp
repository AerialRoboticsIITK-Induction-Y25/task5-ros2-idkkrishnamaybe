#pragma once
#include "vehicle.hpp"

class Drone : public Vehicle {
protected:
    float altitude;
    float max_altitude;

private:
    float speed;

public:
    Drone(const std::string& name, float initial_battery = 100.0f, float max_alt = 100.0f, float speed = 0.0f);
    virtual ~Drone() = default;

    void take_off(float target_altitude);
    void land();
    void emergency_stop();

    std::string get_info() override;

    // Getters and setters
    float get_altitude() const;
    float get_max_altitude() const;
    float get_speed() const;
    void set_speed(float new_speed);

    void set_status(const std::string& new_status);
};
