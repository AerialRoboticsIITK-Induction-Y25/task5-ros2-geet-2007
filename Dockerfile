# Stage 1: builder
FROM osrf/ros:humble-desktop AS builder

WORKDIR /fleet_ws

COPY src/ src/

RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    ros-humble-launch-ros \
    && rm -rf /var/lib/apt/lists/*

RUN /bin/bash -c "source /opt/ros/humble/setup.bash && \
    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release"

# Stage 2: runtime
FROM osrf/ros:humble-desktop

WORKDIR /fleet_ws

RUN apt-get update && apt-get install -y \
    ros-humble-launch-ros \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /fleet_ws/install ./install

RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "source /fleet_ws/install/setup.bash" >> /root/.bashrc

COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["ros2", "launch", "drone_fleet", "fleet.launch.py"]
