#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>


#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class FleetManager : public rclcpp::Node {
public:
    FleetManager() : Node("fleet_manager") {
        drones_["Alpha"] = {"Alpha"};
        drones_["Beta"] = {"Beta"};
        drones_["Gamma"] = {"Gamma"};

        std::vector<std::string> drone_names = {"Alpha", "Beta", "Gamma"};

        for (const auto& name : drone_names) {
            status_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/status", 10,
                [this, name](const std_msgs::msg::String::SharedPtr msg) {
                    RCLCPP_DEBUG(this->get_logger(), "Status from %s: %s", name.c_str(), msg->data.c_str());
                }
            ));

            alert_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/alert", 10,
                [this, name](const std_msgs::msg::String::SharedPtr msg) {
                    std::string ts = get_timestamp();
                    std::cout << "[" << ts << "] WARNING: Alert from " << name << ": " << msg->data << std::endl;
                }
            ));

            complete_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/mission_complete", 10,
                [this, name](const std_msgs::msg::String::SharedPtr msg) {
                    std::string ts = get_timestamp();
                    std::cout << "[" << ts << "] INFO: " << name << " completed mission: " << msg->data << std::endl;
                }
            ));

            telemetry_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/telemetry", 10,
                [this, name](const std_msgs::msg::String::SharedPtr msg) {
                    this->parse_telemetry(name, msg->data);
                }
            ));
        }

        report_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/fleet/status_report",
            std::bind(&FleetManager::handle_status_report, this, std::placeholders::_1, std::placeholders::_2)
        );

        report_timer_ = this->create_wall_timer(5s, std::bind(&FleetManager::print_fleet_report, this));

        RCLCPP_INFO(this->get_logger(), "Fleet Manager started.");
    }

private:
    struct DroneStatus {
        std::string name;
        float battery = 0.0f;
        float altitude = 0.0f;
        std::string status = "unknown";
        int waypoint_index = 0;
        int total_waypoints = 0;
        float speed = 0.0f;
        bool received = false;
    };

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::string get_json_value(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return "";
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length()) return "";
        
        if (json[pos] == '"') {
            pos++;
            size_t end_pos = json.find("\"", pos);
            if (end_pos == std::string::npos) return "";
            return json.substr(pos, end_pos - pos);
        } else {
            size_t end_pos = pos;
            while (end_pos < json.length() && 
                   json[end_pos] != ',' && 
                   json[end_pos] != '}' && 
                   json[end_pos] != ' ' && 
                   json[end_pos] != '\r' && 
                   json[end_pos] != '\n') {
                end_pos++;
            }
            return json.substr(pos, end_pos - pos);
        }
    }

    void parse_telemetry(const std::string& name, const std::string& json) {
        std::string battery_str = get_json_value(json, "battery");
        std::string altitude_str = get_json_value(json, "altitude");
        std::string status_str = get_json_value(json, "status");
        std::string wp_str = get_json_value(json, "waypoint_index");
        std::string total_wp_str = get_json_value(json, "total_waypoints");
        std::string speed_str = get_json_value(json, "speed");

        auto& d = drones_[name];
        if (!battery_str.empty()) d.battery = std::stof(battery_str);
        if (!altitude_str.empty()) d.altitude = std::stof(altitude_str);
        if (!status_str.empty()) d.status = status_str;
        if (!wp_str.empty()) d.waypoint_index = std::stoi(wp_str);
        if (!total_wp_str.empty()) d.total_waypoints = std::stoi(total_wp_str);
        if (!speed_str.empty()) d.speed = std::stof(speed_str);
        d.received = true;
    }

    void print_fleet_report() {
        std::cout << "\n====================== FLEET REPORT ======================\n";
        std::cout << " Timestamp: " << get_timestamp() << "\n";
        std::cout << " +------------+---------+----------+----------+----------+\n";
        std::cout << " | Drone      | Battery | Altitude | Waypoint | Status   |\n";
        std::cout << " +------------+---------+----------+----------+----------+\n";
        for (const auto& pair : drones_) {
            const auto& d = pair.second;
            if (d.received) {
                std::printf(" | %-10s | %5.1f%%  | %6.1fm  | %2d/%-2d    | %-8s |\n",
                            d.name.c_str(), d.battery, d.altitude, d.waypoint_index, d.total_waypoints, d.status.c_str());
            } else {
                std::printf(" | %-10s |  [No Telemetry Received]                |\n", d.name.c_str());
            }
        }
        std::cout << " +------------+---------+----------+----------+----------+\n";
        std::cout << "==========================================================\n" << std::endl;
    }

    void handle_status_report(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                              std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        (void)request;
        print_fleet_report();
        response->success = true;
        response->message = "Fleet report printed successfully at " + get_timestamp();
    }

    std::map<std::string, DroneStatus> drones_;

    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> status_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> alert_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> complete_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;

    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr report_service_;
    rclcpp::TimerBase::SharedPtr report_timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FleetManager>());
    rclcpp::shutdown();
    return 0;
}
