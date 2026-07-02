cmake_minimum_required(VERSION 3.10)
project(drone_oop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/vehicle.cpp
    src/drone.cpp
    src/mission_drone.cpp
    src/autonomous_drone.cpp
)

add_executable(drone_oop ${SOURCES})
