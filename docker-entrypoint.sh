#!/bin/bash
set -e
source /opt/ros/humble/setup.bash
source /fleet_ws/install/setup.bash
exec "$@"
