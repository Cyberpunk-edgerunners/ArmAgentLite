#include <arm_agent_interfaces/srv/find_object.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>

class VisionNode : public rclcpp::Node {
public:
  VisionNode() : Node("vision_node") {
    // 创建 Service 服务端
    // 话题名: "/find_object"
    // 回调函数使用 C++11 的 std::bind 绑定类成员函数
    service_ = this->create_service<arm_agent_interfaces::srv::FindObject>(
        "/find_object",
        std::bind(&VisionNode::handleFindObject, this, std::placeholders::_1,
                  std::placeholders::_2));
    RCLCPP_INFO(this->get_logger(),
                "视觉节点已启动，等待坐标查询请求(Service)...");
  }

private:
  // Service 回调函数：处理请求(request)，并填充响应(response)
  void handleFindObject(
      const std::shared_ptr<arm_agent_interfaces::srv::FindObject::Request>
          request,
      std::shared_ptr<arm_agent_interfaces::srv::FindObject::Response>
          response) {
    RCLCPP_INFO(this->get_logger(), "收到视觉查询请求: 寻找 [%s]",
                request->object_name.c_str());

    // 模拟瞬间的视觉算法耗时
    if (request->object_name == "苹果" || request->object_name == "apple") {
      response->success = true;
      response->x = 0.5;
      response->y = 0.2;
      response->z = 0.3;
      RCLCPP_INFO(this->get_logger(),
                  "算法执行完毕：找到苹果！坐标 (%.2f, %.2f, %.2f)",
                  response->x, response->y, response->z);
    } else {
      response->success = false;
      RCLCPP_WARN(this->get_logger(), "视野中没有找到: %s",
                  request->object_name.c_str());
    }
  }

  rclcpp::Service<arm_agent_interfaces::srv::FindObject>::SharedPtr service_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VisionNode>());
  rclcpp::shutdown();
  return 0;
}
