cmake_minimum_required(VERSION 3.8)
project(drone_fleet)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

include_directories(include)

# Shared drone library
add_library(drone_lib
  src/vehicle.cpp
  src/drone.cpp
  src/mission_drone.cpp
  src/autonomous_drone.cpp
)
ament_target_dependencies(drone_lib rclcpp)

# Part 1: standalone OOP demo
add_executable(drone_demo src/main.cpp)
target_link_libraries(drone_demo drone_lib)

# ROS 2 nodes
add_executable(drone_node src/drone_node.cpp)
target_link_libraries(drone_node drone_lib)
ament_target_dependencies(drone_node rclcpp std_msgs)

add_executable(fleet_manager src/fleet_manager.cpp)
ament_target_dependencies(fleet_manager rclcpp std_msgs std_srvs)

add_executable(health_monitor src/health_monitor.cpp)
ament_target_dependencies(health_monitor rclcpp std_msgs)

install(TARGETS
  drone_demo
  drone_node
  fleet_manager
  health_monitor
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
