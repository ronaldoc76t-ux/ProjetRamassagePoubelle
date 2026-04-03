cmake_minimum_required(VERSION 3.10)
project(openclaw_simulation_tests)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_gtest REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${rclcpp_INCLUDE_DIRS}
)

# Test executable
ament_add_gtest(simulation_test
    test/simulation_test.cpp
)

# Link dependencies
ament_target_dependencies(simulation_test
    rclcpp
    nav_msgs
    sensor_msgs
    geometry_msgs
)

# Install targets
ament_install_programs(
    DESTINATION bin
)