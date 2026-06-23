from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='alpha_drone',
            parameters=[{
                'drone_name': 'Alpha',
                'initial_battery': 100.0,
                'mission_name': 'Mission-Alpha'
            }]
        ),
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='beta_drone',
            parameters=[{
                'drone_name': 'Beta',
                'initial_battery': 60.0,
                'mission_name': 'Mission-Beta'
            }]
        ),
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='gamma_drone',
            parameters=[{
                'drone_name': 'Gamma',
                'initial_battery': 35.0,
                'mission_name': 'Mission-Gamma'
            }]
        ),
        Node(
            package='drone_fleet',
            executable='fleet_manager',
            name='fleet_manager'
        ),
        Node(
            package='drone_fleet',
            executable='health_monitor',
            name='health_monitor'
        ),
    ])
