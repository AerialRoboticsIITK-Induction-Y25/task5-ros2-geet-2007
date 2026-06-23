#!/bin/bash
docker run -it --rm \
    --net=host \
    -e ROS_DOMAIN_ID=42 \
    -v "$(pwd)/src:/fleet_ws/src" \
    drone_fleet_img \
    ros2 launch drone_fleet fleet.launch.py
