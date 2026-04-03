cmake_minimum_required(VERSION 3.8)
project(drone_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Messages
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/DetectedObject.msg"
  "msg/DetectedObjects.msg"
  "msg/DockStatus.msg"
  "msg/DockCommand.msg"
  DEPENDENCIES geometry_msgs std_msgs
)

# Executables
add_executable(flight_controller src/flight_controller.cpp)
ament_target_dependencies(flight_controller rclcpp geometry_msgs sensor_msgs std_msgs)

add_executable(perception_node src/perception_node.cpp)
ament_target_dependencies(perception_node rclcpp sensor_msgs std_msgs cv_bridge image_transport)

add_executable(approach_controller src/approach_controller.cpp)
ament_target_dependencies(approach_controller rclcpp geometry_msgs nav_msgs std_msgs)

add_executable(docking_manager src/docking_manager.cpp)
ament_target_dependencies(docking_manager rclcpp geometry_msgs std_msgs)

add_executable(battery_monitor src/battery_monitor.cpp)
ament_target_dependencies(battery_monitor rclcpp sensor_msgs std_msgs)

add_executable(failsafe_node src/failsafe_node.cpp)
ament_target_dependencies(failsafe_node rclcpp std_msgs geometry_msgs)

# Install
install(TARGETS
  flight_controller
  perception_node
  approach_controller
  docking_manager
  battery_monitor
  failsafe_node
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
  
  ament_add_gtest(test_drone_nodes test/test_drone_nodes.cpp)
  ament_target_dependencies(test_drone_nodes rclcpp geometry_msgs)
endif()

ament_package()