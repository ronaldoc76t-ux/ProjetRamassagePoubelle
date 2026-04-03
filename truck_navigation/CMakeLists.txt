cmake_minimum_required(VERSION 3.8)
project(truck_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Messages
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/PredictedTrajectory.msg"
  "srv/StartNavigation.srv"
  "srv/StopNavigation.srv"
  "srv/PauseNavigation.srv"
  DEPENDENCIES geometry_msgs
)

# Executables
add_executable(nav_node src/nav_node.cpp)
ament_target_dependencies(nav_node
  rclcpp nav2_msgs nav_msgs geometry_msgs sensor_msgs std_msgs tf2_ros)

add_executable(trajectory_predictor src/trajectory_predictor.cpp)
ament_target_dependencies(trajectory_predictor
  rclcpp geometry_msgs nav_msgs std_msgs)

add_executable(telemetry_publisher src/telemetry_publisher.cpp)
ament_target_dependencies(telemetry_publisher
  rclcpp sensor_msgs std_msgs)

add_executable(localization_node src/localization_node.cpp)
ament_target_dependencies(localization_node
  rclcpp sensor_msgs geometry_msgs tf2_ros)

# Install
install(TARGETS
  nav_node
  trajectory_predictor
  telemetry_publisher
  localization_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY
  launch/
  config/
  DESTINATION share/${PROJECT_NAME})

# Tests
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_gtest REQUIRED)
  
  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(test_nav_node test/test_nav_node.cpp)
  ament_target_dependencies(test_nav_node rclcpp nav_msgs geometry_msgs)
  
  ament_add_gtest(test_trajectory_predictor test/test_trajectory_predictor.cpp)
  ament_target_dependencies(test_trajectory_predictor rclcpp geometry_msgs)
endif()

ament_package()