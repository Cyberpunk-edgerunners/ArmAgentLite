#ifndef ARM_AGENT_LITE__NLU_NODE_HPP_
#define ARM_AGENT_LITE__NLU_NODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"

//引入自定义接口
#include "arm_agent_interfaces/action/grasp.hpp"
#include "arm_agent_interfaces/srv/find_object.hpp"

namespace arm_agent_lite {

class NluNode : public rclcpp::Node {
public:
  //定义方便使用的类型别名
  using FindObjectSrv = arm_agent_interfaces::srv::FindObject;
  using GraspAction = arm_agent_interfaces::action::Grasp;
  using GoalHandleGrasp = rclcpp_action::ClientGoalHandle<GraspAction>;

  explicit NluNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
  ~NluNode() override = default;

private:
  // Topic订阅者(接收用户文本)
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr text_sub_;
  void text_command_callback(const std_msgs::msg::String::SharedPtr msg);

  // Service客户端(向视觉节点查询坐标)
  rclcpp::Client<FindObjectSrv>::SharedPtr vision_client_;
  void send_vision_request(const std::string& object_name);
  // 视觉回调(c++11 Lanbda将在cpp中实现，这里无需声明独立函数)

  // Action客户端(向机械臂下发抓取指令)
  rclcpp_action::Client<GraspAction>::SharedPtr arm_action_client_;
  void send_grasp_goal(double x, double y, double z);

  // Action的三个状态回调函数
  void goal_response_callback(const GoalHandleGrasp::SharedPtr& goal_handle);
  void feedback_callback(GoalHandleGrasp::SharedPtr,
                         const std::shared_ptr<const GraspAction::Feedback> feedback);
  void result_callback(const GoalHandleGrasp::WrappedResult& result);

  //内部状态锁，防止上一个动作没做完又接新动作
  bool is_busy_;
};

}  // namespace arm_agent_lite

#endif