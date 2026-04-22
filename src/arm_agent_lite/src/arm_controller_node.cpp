#include <arm_agent_interfaces/action/grasp.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <thread>

class ArmControllerNode : public rclcpp::Node {
public:
  // C++ 类型别名，让代码看起来更整洁
  using Grasp = arm_agent_interfaces::action::Grasp;
  using GoalHandleGrasp = rclcpp_action::ServerGoalHandle<Grasp>;

  ArmControllerNode() : Node("arm_controller_node") {
    using namespace std::placeholders;

    // 创建 Action 服务端，必须绑定 3 个核心回调函数
    action_server_ = rclcpp_action::create_server<Grasp>(
        this, "/grasp_action",
        std::bind(&ArmControllerNode::handleGoal, this, _1,
                  _2), // 1. 处理目标请求
        std::bind(&ArmControllerNode::handleCancel, this,
                  _1), // 2. 处理取消请求
        std::bind(&ArmControllerNode::handleAccepted, this,
                  _1) // 3. 目标被接受后开始执行
    );
    RCLCPP_INFO(this->get_logger(),
                "机械臂控制节点已启动，等待抓取指令(Action)...");
  }

private:
  rclcpp_action::Server<Grasp>::SharedPtr action_server_;

  // ==========================================
  // 回调 1: 决定是否接受这个 Goal
  // ==========================================
  rclcpp_action::GoalResponse
  handleGoal(const rclcpp_action::GoalUUID &uuid,
             std::shared_ptr<const Grasp::Goal> goal) {
    (void)uuid;
    RCLCPP_INFO(this->get_logger(),
                "收到抓取请求！目标坐标: (%.2f, %.2f, %.2f)", goal->target_x,
                goal->target_y, goal->target_z);
    // 这里可以加入安全检查：如果坐标在机器人体外，直接 REJECT
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  // ==========================================
  // 回调 2: 处理紧急取消请求
  // ==========================================
  rclcpp_action::CancelResponse
  handleCancel(const std::shared_ptr<GoalHandleGrasp> goal_handle) {
    RCLCPP_WARN(this->get_logger(), "收到紧急停止请求！准备打断当前动作...");
    (void)goal_handle;
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  // ==========================================
  // 回调 3: 接受请求后，真正开始执行 (极其关键！)
  // ==========================================
  void handleAccepted(const std::shared_ptr<GoalHandleGrasp> goal_handle) {
    using namespace std::placeholders;
    // 工业级铁律：绝不能在这里写 while 循环！否则卡死整个节点！
    // 必须开辟一个新线程去执行真正的硬件动作
    std::thread{std::bind(&ArmControllerNode::executeTask, this, _1),
                goal_handle}
        .detach();
  }

  // ==========================================
  // 独立线程：执行硬件长耗时动作的真实逻辑
  // ==========================================
  void executeTask(const std::shared_ptr<GoalHandleGrasp> goal_handle) {
    RCLCPP_INFO(this->get_logger(), ">> 机械臂开始移动...");
    rclcpp::Rate loop_rate(1); // 设定执行频率：1Hz (每秒1次)

    auto feedback = std::make_shared<Grasp::Feedback>();
    auto result = std::make_shared<Grasp::Result>();

    // 模拟一个耗时 5 秒的运动过程
    for (int i = 5; i > 0; --i) {
      // 每次循环前，检查是否收到了取消指令
      if (goal_handle->is_canceling()) {
        result->success = false;
        result->message = "任务被人类强行终止";
        goal_handle->canceled(result);
        RCLCPP_WARN(this->get_logger(), ">> 动作已取消！机械臂急停！");
        return;
      }

      // 更新并发送实时进度 (Feedback)
      feedback->current_state = "正在靠近目标...";
      feedback->distance_left = i * 0.1; // 模拟还剩多少米
      goal_handle->publish_feedback(feedback);
      RCLCPP_INFO(this->get_logger(), "发布反馈: 距离目标 %.2f 米",
                  feedback->distance_left);

      loop_rate.sleep(); // 线程休眠 1 秒，模拟硬件运动时间
    }

    // 循环结束，任务完成
    if (rclcpp::ok()) {
      result->success = true;
      result->message = "抓取圆满完成";
      goal_handle->succeed(result);
      RCLCPP_INFO(this->get_logger(), ">> 机械臂动作结束：抓取成功！");
    }
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ArmControllerNode>());
  rclcpp::shutdown();
  return 0;
}
