# CMake最低版本
cmake_minimum_required(VERSION 3.8)

#项目名称
project(arm_agent_lite)

#检查编译器是否支持c++17
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 寻找依赖包
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(arm_agent_interfaces REQUIRED)
find_package(rclcpp_action REQUIRED)
# 设置 C++ 标准为 C++17 (或者 20)
set(CMAKE_CXX_STANDARD 17)

# --- 节点 1: UI ---
add_executable(text_ui_node src/text_ui_node.cpp)
ament_target_dependencies(text_ui_node rclcpp std_msgs)

# --- 节点 2: NLU ---
add_executable(nlu_node src/nlu_node.cpp)
ament_target_dependencies(nlu_node rclcpp std_msgs)

# --- 节点 3: 视觉 (Service Server) ---
add_executable(vision_node src/vision_node.cpp)
ament_target_dependencies(vision_node rclcpp arm_agent_interfaces)

# --- 节点 4: 机械臂控制 (Action Server) ---
add_executable(arm_controller_node src/arm_controller_node.cpp)
ament_target_dependencies(arm_controller_node rclcpp rclcpp_action arm_agent_interfaces)

# 安装规则 (为了让 ros2 run 找到它们)
install(TARGETS
  text_ui_node
  nlu_node
  vision_node
  arm_controller_node
  DESTINATION lib/${PROJECT_NAME}
)

# 安装 launch 文件和 config 文件 (预留给你以后的)
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
