#pragma once
#include <string>
#include <vector>
#include "drone_exceptions.hpp"

class Vehicle {
private:
    std::string name;
    float battery_level;
    std::string status;
    std::vector<std::string> flight_log;

protected:
    void log_event(const std::string& event);
    void set_status_internal(const std::string& new_status);

public:
    Vehicle(const std::string& name, float initial_battery = 100.0f);
    virtual ~Vehicle() = default;

    virtual std::string get_info() = 0;

    void drain_battery(float amount);
    void charge_battery(float amount, int duration_seconds);
    bool is_critical() const;
    std::string get_flight_log() const;

    // Getters
    std::string get_name() const;
    float get_battery_level() const;
    std::string get_status() const;
};
