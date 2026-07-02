#include "drone.hpp"
#include <iostream>

Drone::Drone(const std::string& name, float initial_battery, float max_alt, float speed)
    : Vehicle(name, initial_battery), altitude(0.0f), max_altitude(max_alt), speed(speed) {}

void Drone::take_off(float target_altitude) {
    if (target_altitude > max_altitude) {
        throw AltitudeError(get_name() + ": Target altitude " + std::to_string(target_altitude) + "m exceeds max altitude " + std::to_string(max_altitude) + "m.");
    }
    if (target_altitude <= 0.0f) {
        throw AltitudeError(get_name() + ": Target altitude must be greater than 0.");
    }
    set_status("flying");
    altitude = target_altitude;
    log_event("Took off to altitude " + std::to_string(altitude) + "m");
}

void Drone::land() {
    set_status("idle");
    altitude = 0.0f;
    log_event("Landed successfully.");
}

void Drone::emergency_stop() {
    log_event("Emergency stop triggered. Draining battery by 30% and landing.");
    try {
        drain_battery(30.0f);
    } catch (const BatteryDepletedError& e) {
        // Battery already 0, ignore further depletion error here to proceed with landing
    }
    land();
}

std::string Drone::get_info() {
    return "Drone name: " + get_name() + " | battery: " + std::to_string(get_battery_level()) + "% | altitude: " + std::to_string(altitude) + "m | status: " + get_status() + " | speed: " + std::to_string(speed) + " m/s";
}

float Drone::get_altitude() const { return altitude; }
float Drone::get_max_altitude() const { return max_altitude; }
float Drone::get_speed() const { return speed; }

void Drone::set_speed(float new_speed) {
    speed = new_speed;
    log_event("Speed set to " + std::to_string(speed) + " m/s");
}

void Drone::set_status(const std::string& new_status) {
    set_status_internal(new_status);
}
