#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>


#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class HealthMonitor : public rclcpp::Node {
public:
    HealthMonitor() : Node("health_monitor") {
        std::vector<std::string> drone_names = {"Alpha", "Beta", "Gamma"};

        for (const auto& name : drone_names) {
            telemetry_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/telemetry", 10,
                [this, name](const std_msgs::msg::String::SharedPtr msg) {
                    this->handle_telemetry(name, msg->data);
                }
            ));
            
            drone_batteries_[name] = 100.0f;
        }

        warning_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
        summary_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

        diagnostics_timer_ = this->create_wall_timer(10s, std::bind(&HealthMonitor::print_diagnostics_and_publish, this));

        RCLCPP_INFO(this->get_logger(), "Health Monitor started.");
    }

private:
    struct BatterySample {
        double time;
        float battery;
    };

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

    void handle_telemetry(const std::string& name, const std::string& json) {
        std::string battery_str = get_json_value(json, "battery");
        if (battery_str.empty()) return;

        float battery = std::stof(battery_str);
        double current_time = this->now().seconds();

        drone_batteries_[name] = battery;

        auto& deque = drone_samples_[name];
        deque.push_back({current_time, battery});
        if (deque.size() > 10) {
            deque.pop_front();
        }

        if (deque.size() >= 2) {
            double dt = deque.back().time - deque.front().time;
            float db = deque.front().battery - deque.back().battery;
            float rate = (dt > 0.0) ? (db / dt) : 0.0f;
            if (rate < 0.0f) rate = 0.0f;

            if (rate > 1.5f) {
                std::string warning_str = "WARNING: High battery drain rate on " + name + ": " + std::to_string(rate) + " %/s";
                auto msg = std_msgs::msg::String();
                msg.data = warning_str;
                warning_pub_->publish(msg);
                RCLCPP_WARN(this->get_logger(), "%s", warning_str.c_str());
            }
        }
    }

    void print_diagnostics_and_publish() {
        std::cout << "\n==================== HEALTH DIAGNOSTICS ====================\n";
        std::cout << " +------------+------------+-----------------+------------------+\n";
        std::cout << " | Drone      | Drain Rate | Est. to Critical| Est. to Depletion|\n";
        std::cout << " +------------+------------+-----------------+------------------+\n";

        std::stringstream json_summary;
        json_summary << "{";

        std::vector<std::string> drone_names = {"Alpha", "Beta", "Gamma"};
        bool first_json = true;

        for (const auto& name : drone_names) {
            float rate = 0.0f;
            auto& deque = drone_samples_[name];
            if (deque.size() >= 2) {
                double dt = deque.back().time - deque.front().time;
                float db = deque.front().battery - deque.back().battery;
                rate = (dt > 0.0) ? (db / dt) : 0.0f;
                if (rate < 0.0f) rate = 0.0f;
            }

            float battery = drone_batteries_[name];
            float to_critical = -1.0f;
            float to_depletion = -1.0f;

            std::string to_crit_str = "Infinite";
            std::string to_dep_str = "Infinite";

            if (rate > 0.0f) {
                if (battery > 25.0f) {
                    to_critical = (battery - 25.0f) / rate;
                    to_crit_str = std::to_string(static_cast<int>(to_critical)) + "s";
                } else {
                    to_critical = 0.0f;
                    to_crit_str = "0s (Critical)";
                }
                to_depletion = battery / rate;
                to_dep_str = std::to_string(static_cast<int>(to_depletion)) + "s";
            }

            std::printf(" | %-10s | %5.2f%%/s  | %-15s | %-16s |\n",
                        name.c_str(), rate, to_crit_str.c_str(), to_dep_str.c_str());

            if (!first_json) json_summary << ",";
            first_json = false;
            json_summary << "\"" << name << "\":{"
                         << "\"drain_rate\":" << rate << ","
                         << "\"time_to_critical\":" << to_critical << ","
                         << "\"time_to_depletion\":" << to_depletion
                         << "}";
        }
        json_summary << "}";

        std::cout << " +------------+------------+-----------------+------------------+\n";
        std::cout << "============================================================\n" << std::endl;

        auto msg = std_msgs::msg::String();
        msg.data = json_summary.str();
        summary_pub_->publish(msg);
    }

    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr warning_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;

    std::map<std::string, std::deque<BatterySample>> drone_samples_;
    std::map<std::string, float> drone_batteries_;

    rclcpp::TimerBase::SharedPtr diagnostics_timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HealthMonitor>());
    rclcpp::shutdown();
    return 0;
}
