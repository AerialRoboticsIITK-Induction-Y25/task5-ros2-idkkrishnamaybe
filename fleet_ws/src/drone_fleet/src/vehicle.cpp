#include "drone_fleet/vehicle.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

static std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

Vehicle::Vehicle(const std::string& name, float initial_battery)
    : name(name), battery_level(initial_battery), status("idle") {
    if (battery_level < 0.0f) battery_level = 0.0f;
    if (battery_level > 100.0f) battery_level = 100.0f;
    log_event("Vehicle " + name + " initialized with battery level " + std::to_string(battery_level) + "%");
}

void Vehicle::log_event(const std::string& event) {
    flight_log.push_back("[" + get_timestamp() + "] " + event);
}

void Vehicle::set_status_internal(const std::string& new_status) {
    if (new_status != "idle" && new_status != "flying" && new_status != "charging") {
        throw InvalidStateError(name + ": Invalid status state: " + new_status);
    }
    std::string old_status = status;
    status = new_status;
    log_event("Status changed from " + old_status + " to " + new_status);
}

void Vehicle::drain_battery(float amount) {
    if (battery_level <= 0.0f) {
        throw BatteryDepletedError(name + ": Battery is already fully depleted.");
    }
    battery_level -= amount;
    if (battery_level < 0.0f) {
        battery_level = 0.0f;
    }
    log_event("Battery drained by " + std::to_string(amount) + "%. Level: " + std::to_string(battery_level) + "%");
}

void Vehicle::charge_battery(float amount, int duration_seconds) {
    if (status != "charging") {
        throw InvalidStateError(name + ": Cannot charge unless in charging state. Current: " + status);
    }
    battery_level += amount;
    if (battery_level > 100.0f) {
        battery_level = 100.0f;
    }
    log_event("Charged battery by " + std::to_string(amount) + "% over " + std::to_string(duration_seconds) + " seconds. Level: " + std::to_string(battery_level) + "%");
}

bool Vehicle::is_critical() const {
    return battery_level <= 25.0f;
}

std::string Vehicle::get_flight_log() const {
    std::string log_str;
    for (const auto& log : flight_log) {
        log_str += log + "\n";
    }
    return log_str;
}

std::string Vehicle::get_name() const { return name; }
float Vehicle::get_battery_level() const { return battery_level; }
std::string Vehicle::get_status() const { return status; }
