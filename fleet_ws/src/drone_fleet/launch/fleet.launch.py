from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        # ── Alpha — starts full battery ─────────────────────────────────────
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='drone_alpha',
            output='screen',
            parameters=[{
                'drone_name':      'Alpha',
                'initial_battery': 100.0,
                'mission_name':    'Alpha-Patrol',
            }]
        ),

        # ── Beta — starts at 60% ────────────────────────────────────────────
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='drone_beta',
            output='screen',
            parameters=[{
                'drone_name':      'Beta',
                'initial_battery': 60.0,
                'mission_name':    'Beta-Scout',
            }]
        ),

        # ── Gamma — starts nearly critical (35%) ───────────────────────────
        Node(
            package='drone_fleet',
            executable='drone_node',
            name='drone_gamma',
            output='screen',
            parameters=[{
                'drone_name':      'Gamma',
                'initial_battery': 35.0,
                'mission_name':    'Gamma-Survey',
            }]
        ),

        # ── Fleet Manager ───────────────────────────────────────────────────
        Node(
            package='drone_fleet',
            executable='fleet_manager',
            name='fleet_manager',
            output='screen',
        ),

        # ── Health Monitor ──────────────────────────────────────────────────
        Node(
            package='drone_fleet',
            executable='health_monitor',
            name='health_monitor',
            output='screen',
        ),
    ])
