#pragma once
#include "mission_drone.hpp"
#include <vector>
#include <tuple>
#include <string>

class AutonomousDrone : public MissionDrone {
public:
    std::string ai_mode;
    std::tuple<float, float, float> home_position;

private:
    std::vector<std::string> obstacle_log;

public:
    AutonomousDrone(const std::string& name, float initial_battery = 100.0f, float max_alt = 100.0f, float speed = 0.0f,
                    const std::string& mission_name = "", const std::vector<std::tuple<float, float, float>>& waypoints = {},
                    const std::string& ai_mode = "manual", std::tuple<float, float, float> home_pos = {0.0f, 0.0f, 0.0f});
    virtual ~AutonomousDrone() = default;

    void set_ai_mode(const std::string& mode);
    void detect_obstacle(std::tuple<float, float, float> position, const std::string& severity);
    std::vector<std::tuple<float, float, float>> auto_replan(const std::vector<std::tuple<float, float, float>>& obstacles);

    std::string get_info() override;

    const std::vector<std::string>& get_obstacle_log() const;
};
