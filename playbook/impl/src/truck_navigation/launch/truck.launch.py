"""
Launch file for truck navigation package
"""
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    # Get package share directory
    pkg_dir = get_package_share_directory('truck_navigation')
    config_dir = os.path.join(pkg_dir, 'config')
    
    # Parameters
    params_file = os.path.join(config_dir, 'params.yaml')
    
    # Nodes
    nodes = [
        # Localization node
        Node(
            package='truck_navigation',
            executable='localization_node',
            name='localization_node',
            parameters=[params_file],
            output='screen',
            emulate_tty=True,
        ),
        
        # Navigation node
        Node(
            package='truck_navigation',
            executable='nav_node',
            name='nav_node',
            parameters=[params_file],
            output='screen',
            emulate_tty=True,
        ),
        
        # Trajectory predictor
        Node(
            package='truck_navigation',
            executable='trajectory_predictor',
            name='trajectory_predictor',
            parameters=[params_file],
            output='screen',
            emulate_tty=True,
        ),
        
        # Telemetry publisher
        Node(
            package='truck_navigation',
            executable='telemetry_publisher',
            name='telemetry_publisher',
            parameters=[params_file],
            output='screen',
            emulate_tty=True,
        ),
    ]
    
    return LaunchDescription(nodes)