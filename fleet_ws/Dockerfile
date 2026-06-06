FROM osrf/ros:humble-desktop AS builder
SHELL ["/bin/bash", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends python3-colcon-common-extensions ros-humble-std-srvs && rm -rf /var/lib/apt/lists/*
WORKDIR /fleet_ws
COPY fleet_ws/src ./src
RUN source /opt/ros/humble/setup.bash && colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release

FROM osrf/ros:humble-desktop AS runtime
SHELL ["/bin/bash", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends ros-humble-std-srvs && rm -rf /var/lib/apt/lists/*
WORKDIR /fleet_ws
COPY --from=builder /fleet_ws/install ./install
COPY fleet_ws/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
CMD ["ros2", "launch", "drone_fleet", "fleet.launch.py"]
