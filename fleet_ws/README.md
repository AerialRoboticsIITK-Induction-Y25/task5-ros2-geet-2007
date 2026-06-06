# Virtual Drone Fleet Manager — ARIITK Task 5

## Project Structure

```
fleet_ws/
├── Dockerfile
├── entrypoint.sh
├── run.sh                         ← one-command launcher
└── src/
    └── drone_fleet/
        ├── package.xml
        ├── CMakeLists.txt
        ├── include/drone_fleet/
        │   ├── drone_exceptions.hpp   ← custom exception hierarchy
        │   ├── vehicle.hpp            ← abstract base class
        │   ├── drone.hpp
        │   ├── mission_drone.hpp
        │   └── autonomous_drone.hpp
        ├── src/
        │   ├── vehicle.cpp
        │   ├── drone.cpp
        │   ├── mission_drone.cpp
        │   ├── autonomous_drone.cpp
        │   ├── drone_node.cpp         ← ROS 2 drone publisher node
        │   ├── fleet_manager.cpp      ← ROS 2 fleet monitor + service
        │   └── health_monitor.cpp     ← ROS 2 health diagnostics node
        └── launch/
            └── fleet.launch.py

cpp_part1/                         ← standalone Part 1 OOP demo
├── CMakeLists.txt
├── include/  (same headers)
└── src/
    ├── vehicle.cpp
    ├── drone.cpp
    ├── mission_drone.cpp
    ├── autonomous_drone.cpp
    └── main.cpp
```

## Part 1 — Build & Run (standalone C++)

```bash
cd cpp_part1
mkdir build && cd build
cmake .. && make
./drone_demo
```

## Part 2 + 3 — Docker (recommended)

```bash
git clone <your_repo>
cd fleet_ws
chmod +x run.sh
./run.sh
```

The container will:
- Build the ROS 2 workspace with `colcon build`
- Launch all 5 nodes via `fleet.launch.py`
- Print fleet reports every **5 seconds**
- Print health diagnostics every **10 seconds**
- **Gamma** hits critical battery (~20%) within ~30 seconds

## Manual build (without Docker)

```bash
cd fleet_ws
source /opt/ros/humble/setup.bash
colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
ros2 launch drone_fleet fleet.launch.py
```

## ROS 2 Topics

| Topic | Type | Publisher |
|-------|------|-----------|
| `/drone/<name>/status` | std_msgs/String | drone_node |
| `/drone/<name>/telemetry` | std_msgs/String (JSON) | drone_node |
| `/drone/<name>/alert` | std_msgs/String | drone_node |
| `/drone/<name>/mission_complete` | std_msgs/String | drone_node |
| `/fleet/health_warning` | std_msgs/String (JSON) | health_monitor |
| `/fleet/health_summary` | std_msgs/String (JSON) | health_monitor |

## ROS 2 Services

| Service | Type |
|---------|------|
| `/fleet/status_report` | std_srvs/Trigger |

Call it manually:
```bash
ros2 service call /fleet/status_report std_srvs/srv/Trigger
```

## Class Hierarchy

```
Vehicle  (abstract)
└── Drone
    └── MissionDrone
        └── AutonomousDrone
```

## Exception Hierarchy

```
DroneException  (base, inherits std::runtime_error)
├── BatteryDepletedError
├── InvalidStateError
└── AltitudeError
```
