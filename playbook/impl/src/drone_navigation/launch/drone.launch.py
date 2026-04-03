"""
Launch file for drone navigation package
"""
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    nodes = [
        Node(
            package='drone_navigation',
            executable='flight_controller',
            name='flight_controller',
            parameters=[{'max_altitude': 100.0, 'max_speed': 15.0}],
            output='screen',
        ),
        Node(
            package='drone_navigation',
            executable='perception_node',
            name='perception_node',
            parameters=[{'confidence_threshold': 0.7}],
            output='screen',
        ),
        Node(
            package='drone_navigation',
            executable='approach_controller',
            name='approach_controller',
            parameters=[{'intercept_distance': 2.0, 'approach_height': 3.0}],
            output='screen',
        ),
        Node(
            package='drone_navigation',
            executable='docking_manager',
            name='docking_manager',
            parameters=[{'docking_speed': 0.5}],
            output='screen',
        ),
        Node(
            package='drone_navigation',
            executable='battery_monitor',
            name='battery_monitor',
            parameters=[{'battery_critical': 15.0, 'battery_warning': 25.0, 'battery_return': 40.0}],
            output='screen',
        ),
        Node(
            package='drone_navigation',
            executable='failsafe_node',
            name='failsafe_node',
            output='screen',
        ),
    ]
    
    return LaunchDescription(nodes)