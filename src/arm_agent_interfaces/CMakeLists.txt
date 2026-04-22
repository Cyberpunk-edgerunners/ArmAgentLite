cmake_minimum_required(VERSION 3.8)
project(arm_agent_interfaces)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
# 寻找生成接口必须的核心工具
find_package(rosidl_default_generators REQUIRED)
find_package(action_msgs REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

# 文本文件编译成 c++和python文件
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/FindObject.srv"
  "action/Grasp.action"
  DEPENDENCIES action_msgs
)

ament_package()
