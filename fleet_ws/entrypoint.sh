#!/bin/bash
set -e

# Source ROS 2 Humble
source /opt/ros/humble/setup.bash

# Source the built workspace
source /fleet_ws/install/setup.bash

# Execute whatever command was passed (or the default CMD)
exec "$@"
