#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <cstdio>


#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "drone_fleet/mission_drone.hpp"
#include "drone_fleet/exceptions.hpp"

using namespace std::chrono_literals;

class DroneNode : public rclcpp::Node {
public:
    DroneNode() : Node("drone_node") {
        this->declare_parameter<std::string>("drone_name", "Alpha");
        this->declare_parameter<double>("initial_battery", 100.0);
        this->declare_parameter<std::string>("mission_name", "Survey");

        std::string drone_name = this->get_parameter("drone_name").as_string();
        double initial_battery = this->get_parameter("initial_battery").as_double();
        std::string mission_name = this->get_parameter("mission_name").as_string();

        std::vector<std::tuple<float, float, float>> wps = {
            {10.0f, 10.0f, 10.0f},
            {20.0f, 20.0f, 15.0f},
            {30.0f, 30.0f, 20.0f},
            {40.0f, 40.0f, 25.0f},
            {50.0f, 50.0f, 30.0f}
        };

        drone_ = std::make_unique<MissionDrone>(drone_name, static_cast<float>(initial_battery), 100.0f, 3.2f, mission_name, wps);
        
        try {
            drone_->take_off(10.0f);
        } catch (const DroneException& e) {
            RCLCPP_ERROR(this->get_logger(), "Failed to take off: %s", e.what());
        }

        status_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name + "/status", 10);
        alert_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name + "/alert", 10);
        complete_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name + "/mission_complete", 10);
        telemetry_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name + "/telemetry", 10);

        status_timer_ = this->create_wall_timer(1s, std::bind(&DroneNode::publish_status, this));
        telemetry_timer_ = this->create_wall_timer(2s, std::bind(&DroneNode::publish_telemetry, this));

        RCLCPP_INFO(this->get_logger(), "Drone node '%s' started.", drone_name.c_str());
    }

private:
    void publish_status() {
        std::string drone_name = drone_->get_name();
        
        try {
            drone_->drain_battery(0.5f);
        } catch (const BatteryDepletedError& e) {
            RCLCPP_DEBUG(this->get_logger(), "Battery already depleted: %s", e.what());
        }

        if (drone_->is_critical() && !alerted_critical_) {
            alerted_critical_ = true;
            std::string alert_msg = "CRITICAL: Battery level is " + std::to_string(drone_->get_battery_level()) + "%";
            auto msg = std_msgs::msg::String();
            msg.data = alert_msg;
            alert_pub_->publish(msg);
            drone_->land();
            RCLCPP_WARN(this->get_logger(), "Alert: Battery critical! Landing drone.");
        }

        publish_count_++;
        if (publish_count_ >= 3) {
            publish_count_ = 0;
            
            if (drone_->get_status() == "flying") {
                if (drone_->mission_complete()) {
                    auto msg = std_msgs::msg::String();
                    msg.data = "Mission Complete for drone " + drone_name;
                    complete_pub_->publish(msg);
                    RCLCPP_INFO(this->get_logger(), "Mission Complete! Restarting mission.");
                    drone_->current_waypoint_index = 0;
                } else {
                    try {
                        drone_->next_waypoint();
                    } catch (const DroneException& e) {
                        RCLCPP_ERROR(this->get_logger(), "Error traversing waypoint: %s", e.what());
                    }
                }
            }
        }

        char status_buf[256];
        snprintf(status_buf, sizeof(status_buf),
                 "name:%s|battery:%.1f|altitude:%.1f|status:%s|waypoint:%d/%zu|speed:%.1f",
                 drone_name.c_str(),
                 drone_->get_battery_level(),
                 drone_->get_altitude(),
                 drone_->get_status().c_str(),
                 drone_->current_waypoint_index,
                 drone_->waypoints.size(),
                 drone_->get_speed());

        auto status_msg = std_msgs::msg::String();
        status_msg.data = std::string(status_buf);
        status_pub_->publish(status_msg);
    }

    void publish_telemetry() {
        std::string drone_name = drone_->get_name();
        
        char json_buf[512];
        snprintf(json_buf, sizeof(json_buf),
                 "{\"name\":\"%s\",\"battery\":%.1f,\"altitude\":%.1f,\"status\":\"%s\",\"waypoint_index\":%d,\"total_waypoints\":%zu,\"speed\":%.1f}",
                 drone_name.c_str(),
                 drone_->get_battery_level(),
                 drone_->get_altitude(),
                 drone_->get_status().c_str(),
                 drone_->current_waypoint_index,
                 drone_->waypoints.size(),
                 drone_->get_speed());

        auto telemetry_msg = std_msgs::msg::String();
        telemetry_msg.data = std::string(json_buf);
        telemetry_pub_->publish(telemetry_msg);
    }

    std::unique_ptr<MissionDrone> drone_;
    bool alerted_critical_ = false;
    int publish_count_ = 0;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr complete_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;

    rclcpp::TimerBase::SharedPtr status_timer_;
    rclcpp::TimerBase::SharedPtr telemetry_timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DroneNode>());
    rclcpp::shutdown();
    return 0;
}
