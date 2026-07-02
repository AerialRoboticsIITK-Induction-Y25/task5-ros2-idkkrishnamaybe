cmake_minimum_required(VERSION 3.10)
project(drone_oop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
    main.cpp
    vehicle.cpp
    drone.cpp
    mission_drone.cpp
    autonomous_drone.cpp
)

add_executable(drone_oop ${SOURCES})
