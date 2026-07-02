#include <iostream>
#include <vector>
#include <tuple>
#include "drone_exceptions.hpp"
#include "vehicle.hpp"
#include "drone.hpp"
#include "mission_drone.hpp"
#include "autonomous_drone.hpp"

int main() {
    std::cout << "--- Part 1: OOPs in C++ ---\n\n";

    // 1. Show polymorphism by storing objects of Drone, MissionDrone, AutonomousDrone in std::vector<Vehicle*>
    std::vector<Vehicle*> fleet;

    std::vector<std::tuple<float, float, float>> alpha_wps = {
        {10.0f, 20.0f, 15.0f},
        {15.0f, 25.0f, 20.0f},
        {20.0f, 30.0f, 25.0f}
    };

    std::vector<std::tuple<float, float, float>> beta_wps = {
        {50.0f, 50.0f, 30.0f},
        {60.0f, 60.0f, 40.0f},
        {70.0f, 70.0f, 50.0f},
        {80.0f, 80.0f, 60.0f}
    };

    Drone* drone = new Drone("BasicDrone", 90.0f, 100.0f, 5.0f);
    MissionDrone* mission_drone = new MissionDrone("Alpha", 80.0f, 150.0f, 10.0f, "Survey", alpha_wps);
    AutonomousDrone* auto_drone = new AutonomousDrone("Beta", 95.0f, 200.0f, 12.0f, "Delivery", beta_wps, "auto", {0.0f, 0.0f, 0.0f});

    fleet.push_back(drone);
    fleet.push_back(mission_drone);
    fleet.push_back(auto_drone);

    std::cout << "Polymorphic Info Retrieval:\n";
    for (Vehicle* v : fleet) {
        std::cout << v->get_info() << "\n";
    }
    std::cout << "\n";

    // 2. Show that private members cannot be accessed directly (commented out)
    // Uncommenting the lines below will result in compilation errors:
    // float bat = drone->battery_level; // Error: 'float Vehicle::battery_level' is private within this context
    // std::string stat = drone->status; // Error: 'std::string Vehicle::status' is private within this context
    // float spd = drone->speed; // Error: 'float Drone::speed' is private within this context
    std::cout << "Attempted private member access is restricted. (See commented code in main.cpp)\n\n";

    // 3. Exception Handling Demonstration
    std::cout << "--- Exception Handling Demonstrations ---\n";
    
    // Test 1: AltitudeError
    try {
        std::cout << "Attempting to take off to 250m on a drone with max altitude 100m...\n";
        drone->take_off(250.0f);
    } catch (const AltitudeError& e) {
        std::cerr << "Caught AltitudeError: " << e.what() << "\n";
    } catch (const DroneException& e) {
        std::cerr << "Caught DroneException: " << e.what() << "\n";
    }
    std::cout << "\n";

    // Test 2: InvalidStateError (Charging without setting state to charging first)
    try {
        std::cout << "Attempting to charge battery while state is idle/flying...\n";
        drone->charge_battery(10.0f, 5);
    } catch (const InvalidStateError& e) {
        std::cerr << "Caught InvalidStateError: " << e.what() << "\n";
    }
    std::cout << "\n";

    // Test 3: BatteryDepletedError
    try {
        std::cout << "Draining battery to 0% and then draining again...\n";
        drone->drain_battery(95.0f); // drains to 0
        drone->drain_battery(5.0f);  // throws
    } catch (const BatteryDepletedError& e) {
        std::cerr << "Caught BatteryDepletedError: " << e.what() << "\n";
    }
    std::cout << "\n";

    // 4. Run a full mission on AutonomousDrone
    std::cout << "--- Running Full Mission on AutonomousDrone (Beta) ---\n";
    try {
        std::cout << "Taking off to 15m...\n";
        auto_drone->take_off(15.0f);

        std::cout << "Starting waypoint traversal...\n";
        while (!auto_drone->mission_complete()) {
            auto_drone->next_waypoint();
        }

        // Simulate obstacle detection
        std::cout << "\nDetecting high-severity obstacle near Beta...\n";
        auto_drone->detect_obstacle({75.0f, 75.0f, 55.0f}, "high");

        std::cout << "\n" << auto_drone->mission_summary() << "\n";

        std::cout << "\nFlight Log:\n" << auto_drone->get_flight_log() << "\n";
    } catch (const DroneException& e) {
        std::cerr << "Exception during mission execution: " << e.what() << "\n";
    }

    // Clean up
    for (Vehicle* v : fleet) {
        delete v;
    }

    return 0;
}
