cmake_minimum_required(VERSION 3.14)
project(drone_fleet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

# ── ROS 2 deps ─────────────────────────────────────────────────────────────────
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# ── Shared drone library (reuses Part-1 classes) ──────────────────────────────
add_library(drone_lib STATIC
    src/vehicle.cpp
    src/drone.cpp
    src/mission_drone.cpp
    src/autonomous_drone.cpp
)
target_include_directories(drone_lib PUBLIC include/drone_fleet)
ament_target_dependencies(drone_lib rclcpp)

# ── drone_node ────────────────────────────────────────────────────────────────
add_executable(drone_node src/drone_node.cpp)
target_include_directories(drone_node PRIVATE include/drone_fleet)
target_link_libraries(drone_node drone_lib)
ament_target_dependencies(drone_node rclcpp std_msgs)

# ── fleet_manager ─────────────────────────────────────────────────────────────
add_executable(fleet_manager src/fleet_manager.cpp)
target_include_directories(fleet_manager PRIVATE include/drone_fleet)
ament_target_dependencies(fleet_manager rclcpp std_msgs std_srvs)

# ── health_monitor ────────────────────────────────────────────────────────────
add_executable(health_monitor src/health_monitor.cpp)
target_include_directories(health_monitor PRIVATE include/drone_fleet)
ament_target_dependencies(health_monitor rclcpp std_msgs)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS drone_node fleet_manager health_monitor
        DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
        DESTINATION share/${PROJECT_NAME})

ament_package()
