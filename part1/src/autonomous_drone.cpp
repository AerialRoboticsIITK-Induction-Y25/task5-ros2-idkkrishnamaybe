#include "autonomous_drone.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>

static std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static float calc_distance(std::tuple<float, float, float> p1, std::tuple<float, float, float> p2) {
    float dx = std::get<0>(p1) - std::get<0>(p2);
    float dy = std::get<1>(p1) - std::get<1>(p2);
    float dz = std::get<2>(p1) - std::get<2>(p2);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

AutonomousDrone::AutonomousDrone(const std::string& name, float initial_battery, float max_alt, float speed,
                                 const std::string& mission_name, const std::vector<std::tuple<float, float, float>>& waypoints,
                                 const std::string& ai_mode, std::tuple<float, float, float> home_pos)
    : MissionDrone(name, initial_battery, max_alt, speed, mission_name, waypoints), ai_mode(ai_mode), home_position(home_pos) {}

void AutonomousDrone::set_ai_mode(const std::string& mode) {
    if (mode != "manual" && mode != "auto" && mode != "return_home") {
        throw InvalidStateError(get_name() + ": Invalid AI mode: " + mode);
    }
    std::string old_mode = ai_mode;
    ai_mode = mode;
    log_event("AI Mode changed from " + old_mode + " to " + mode);
    if (mode == "return_home") {
        waypoints.insert(waypoints.begin() + current_waypoint_index, home_position);
        log_event("Inserted home position as next waypoint: (" + std::to_string(std::get<0>(home_position)) + ", " 
                  + std::to_string(std::get<1>(home_position)) + ", " + std::to_string(std::get<2>(home_position)) + ")");
    }
}

void AutonomousDrone::detect_obstacle(std::tuple<float, float, float> position, const std::string& severity) {
    std::string ts = get_timestamp();
    std::string log_msg = "[" + ts + "] Obstacle at (" + std::to_string(std::get<0>(position)) + ", " 
                          + std::to_string(std::get<1>(position)) + ", " + std::to_string(std::get<2>(position)) + ") | Severity: " + severity;
    obstacle_log.push_back(log_msg);
    log_event("Obstacle detected with severity: " + severity);
    if (severity == "high") {
        emergency_stop();
    }
}

std::vector<std::tuple<float, float, float>> AutonomousDrone::auto_replan(const std::vector<std::tuple<float, float, float>>& obstacles) {
    std::vector<std::tuple<float, float, float>> replanned;
    log_event("Triggered auto-replan to avoid obstacles.");
    for (int i = current_waypoint_index; i < static_cast<int>(waypoints.size()); ++i) {
        auto wp = waypoints[i];
        for (const auto& obs : obstacles) {
            if (calc_distance(wp, obs) < 5.0f) {
                std::get<2>(wp) += 6.0f;
                log_event("Waypoint " + std::to_string(i) + " updated to altitude " + std::to_string(std::get<2>(wp)) + "m to avoid obstacle.");
            }
        }
        waypoints[i] = wp;
        replanned.push_back(wp);
    }
    return replanned;
}

std::string AutonomousDrone::get_info() {
    return MissionDrone::get_info() + " | AI Mode: " + ai_mode + " | Obstacles: " + std::to_string(obstacle_log.size());
}

const std::vector<std::string>& AutonomousDrone::get_obstacle_log() const {
    return obstacle_log;
}
