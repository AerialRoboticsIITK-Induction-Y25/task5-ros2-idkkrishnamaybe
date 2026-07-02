#pragma once
#include "drone.hpp"
#include <vector>
#include <tuple>
#include <string>
#include <utility>

class MissionDrone : public Drone {
public:
    std::string mission_name;
    std::vector<std::tuple<float, float, float>> waypoints;
    int current_waypoint_index;

private:
    std::vector<std::pair<std::tuple<float, float, float>, std::string>> visited_waypoints;

public:
    MissionDrone(const std::string& name, float initial_battery = 100.0f, float max_alt = 100.0f, float speed = 0.0f, 
                 const std::string& mission_name = "", const std::vector<std::tuple<float, float, float>>& waypoints = {});
    virtual ~MissionDrone() = default;

    std::tuple<float, float, float> next_waypoint();
    void skip_waypoint(const std::string& reason);
    bool mission_complete() const;
    std::string mission_summary() const;

    std::string get_info() override;

    const std::vector<std::pair<std::tuple<float, float, float>, std::string>>& get_visited_waypoints() const;
};
