#include "drone_fleet/mission_drone.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

MissionDrone::MissionDrone(const std::string& name, float initial_battery, float max_alt, float speed, 
                           const std::string& mission_name, const std::vector<std::tuple<float, float, float>>& waypoints)
    : Drone(name, initial_battery, max_alt, speed), mission_name(mission_name), waypoints(waypoints), current_waypoint_index(0) {}

std::tuple<float, float, float> MissionDrone::next_waypoint() {
    if (mission_complete()) {
        throw InvalidStateError(get_name() + ": Mission already complete.");
    }
    std::tuple<float, float, float> wp = waypoints[current_waypoint_index];
    visited_waypoints.push_back({wp, get_timestamp()});
    
    if (get_status() != "flying") {
        set_status("flying");
    }
    
    float wp_alt = std::get<2>(wp);
    if (wp_alt > max_altitude) {
        throw AltitudeError(get_name() + ": Waypoint altitude " + std::to_string(wp_alt) + "m exceeds max altitude " + std::to_string(max_altitude) + "m.");
    }
    altitude = wp_alt;

    current_waypoint_index++;
    drain_battery(1.5f);
    
    log_event("Moved to waypoint " + std::to_string(current_waypoint_index) + "/" + std::to_string(waypoints.size()) 
              + ": (" + std::to_string(std::get<0>(wp)) + ", " + std::to_string(std::get<1>(wp)) + ", " + std::to_string(std::get<2>(wp)) + ")");
    
    return wp;
}

void MissionDrone::skip_waypoint(const std::string& reason) {
    if (mission_complete()) {
        throw InvalidStateError(get_name() + ": Mission already complete.");
    }
    std::tuple<float, float, float> wp = waypoints[current_waypoint_index];
    log_event("Skipped waypoint " + std::to_string(current_waypoint_index + 1) + ": (" 
              + std::to_string(std::get<0>(wp)) + ", " + std::to_string(std::get<1>(wp)) + ", " + std::to_string(std::get<2>(wp)) 
              + ") Reason: " + reason);
    current_waypoint_index++;
}

bool MissionDrone::mission_complete() const {
    return current_waypoint_index >= static_cast<int>(waypoints.size());
}

std::string MissionDrone::mission_summary() const {
    std::string summary = "Mission Summary for: " + mission_name + "\n";
    summary += "Visited Waypoints:\n";
    for (size_t i = 0; i < visited_waypoints.size(); ++i) {
        auto wp = visited_waypoints[i].first;
        std::string ts = visited_waypoints[i].second;
        summary += "- Waypoint " + std::to_string(i+1) + ": (" 
            + std::to_string(std::get<0>(wp)) + ", " 
            + std::to_string(std::get<1>(wp)) + ", " 
            + std::to_string(std::get<2>(wp)) + ") at " + ts + "\n";
    }
    summary += "Status: " + (mission_complete() ? std::string("Completed") : ("Incomplete (" + std::to_string(current_waypoint_index) + "/" + std::to_string(waypoints.size()) + ")"));
    return summary;
}

std::string MissionDrone::get_info() {
    return Drone::get_info() + " | mission: " + mission_name + " | waypoint: " + std::to_string(current_waypoint_index) + "/" + std::to_string(waypoints.size());
}

const std::vector<std::pair<std::tuple<float, float, float>, std::string>>& MissionDrone::get_visited_waypoints() const {
    return visited_waypoints;
}
