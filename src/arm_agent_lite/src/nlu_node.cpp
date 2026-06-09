#include "arm_agent_lite/nlu_node.hpp"

#include <functional>
#include <regex>

namespace arm_agent_lite {

NluNode::NluNode(const rclcpp::NodeOptions& options) : Node("nlu_node", options), is_busy_(false) {
  //初始化订阅者(监听前端发送的指令)
  text_sub_ = this->create_subscription<std_msgs::msg::String>(
      "user_command", 10, std::bind(&NluNode::text_command_callback, this, std::placeholders::_1));

  //初始化Service客户端
  vision_client_ = this->create_client<FindObjectSrv>("vision/find_object");

  //初始化Action客户端
  arm_action_client_ = rclcpp_action::create_client<GraspAction>(this, "arm/grasp");

  RCLCPP_INFO(this->get_logger(), "🧠 NLU大脑中枢已启动，等待指令输入...");
}

// 阶段一：收到用户文本，启动业务流
void NluNode::text_command_callback(const std_msgs::msg::String::SharedPtr msg) {
  if (is_busy_) {
    RCLCPP_WARN(this->get_logger(), "机器人正在忙碌中，忽略指令：%s", msg->data.c_str());
    return;
  }

  std::string command = msg->data;
  RCLCPP_INFO(this->get_logger(), "💬 收到指令：[%s]", command.c_str());

  // 极简 NLU 逻辑(后续实际接入大模型API)
  //这里用正则粗匹配“抓取XXX”
  std::smatch match;
  std::regex e("抓取(.*)");
  if (std::regex_search(command, match, e) && match.size() > 1) {
    std::string object_name = match.str(1);
    //去除空格
    object_name.erase(0, object_name.find_first_not_of("\t\r\n"));
    object_name.erase(object_name.find_first_not_of("\t\r\n") + 1);

    RCLCPP_INFO(this->get_logger(), "🔍 解析出目标物体: [%s]，正在请求视觉节点计算坐标...", object_name.c_str());
    is_busy_ = true;  //锁定状态
    send_vision_request(object_name);
  } else {
    RCLCPP_ERROR(this->get_logger(), "❌ 无法理解该指令，支持的格式如：'抓取 苹果'");
  }
}

// 阶段二：异步请求Vision Service,不阻塞主线程
void NluNode::send_vision_request(const std::string& object_name) {
  //等待服务上线
  if (!vision_client_->wait_for_service(std::chrono::seconds(2))) {
    RCLCPP_ERROR(this->get_logger(), "🚨 视觉服务掉线，请检查 Vision 节点是否启动！");
    is_busy_ = false;
    return;
  }

  auto request = std::make_shared<FindObjectSrv::Request>();
  // request->object_name 取决于.srv 里的定义名称
  request->object_name = object_name;

  //使用异步调用 + Lambda 回调，绝不使用 .get() 阻塞
  using ServiceResponseFuture = rclcpp::Client<FindObjectSrv>::SharedFuture;
  auto response_recrived_callback = [this](ServiceResponseFuture future) {
    auto response = future.get();  //此时数据已就绪，不会阻塞
    if (response->success) {
      RCLCPP_INFO(this->get_logger(), "✅ 视觉解算成功！坐标: (X:%.2f, Y:%.2f, Z:%.2f)", response->x, response->y,
                  response->z);
      //拿到坐标，立刻下发Action给机械臂
      this->send_grasp_goal(response->x, response->y, response->z);
    } else {
      RCLCPP_ERROR(this->get_logger(), "❌ 视觉节点未找到物体！");
      this->is_busy_ = false;  //解锁状态
    }
  };

  vision_client_->async_send_request(request, response_recrived_callback);
}

//阶段三：异步下发 Action给小脑(MoveIt2/底层控制器)
void NluNode::send_grasp_goal(double x, double y, double z) {
  if (!arm_action_client_->wait_for_action_server(std::chrono::seconds(2))) {
    RCLCPP_ERROR(this->get_logger(), "🚨 机械臂动作服务器未响应！");
    is_busy_ = false;
    return;
  }
  auto goal_msg = GraspAction::Goal();
  // 这里的 x, y, z 取决于你 .action 文件 Goal 部分的定义
  goal_msg.target_x = x;
  goal_msg.target_y = y;
  goal_msg.target_z = z;

  RCLCPP_INFO(this->get_logger(), "🤖 正在向底层总线下发抓取动作...");

  //配置 Action 的三个异步回调插槽
  auto send_goal_options = rclcpp_action::Client<GraspAction>::SendGoalOptions();
  send_goal_options.goal_response_callback = std::bind(&NluNode::goal_response_callback, this, std::placeholders::_1);
  send_goal_options.feedback_callback =
      std::bind(&NluNode::feedback_callback, this, std::placeholders::_1, std::placeholders::_2);
  send_goal_options.result_callback = std::bind(&NluNode::result_callback, this, std::placeholders::_1);

  arm_action_client_->async_send_goal(goal_msg, send_goal_options);
}

// Action 回调1：服务器是否接受了目标
void NluNode::goal_response_callback(const GoalHandleGrasp::SharedPtr& goal_handle) {
  if (!goal_handle) {
    RCLCPP_ERROR(this->get_logger(), "❌ 机械臂拒绝了抓取请求 (可能是坐标超限或处于急停状态)");
    is_busy_ = false;
  } else {
    RCLCPP_INFO(this->get_logger(), "✅ 机械臂已接受请求，开始运动...");
  }
}

// Action 回调2：高频接收执行进度
void NluNode::feedback_callback(GoalHandleGrasp::SharedPtr,
                                const std::shared_ptr<const GraspAction::Feedback> feedback) {
  // 这里的 current_state 取决于你 .action 文件 Feedback 部分的定义
  RCLCPP_INFO(this->get_logger(), "📈 执行状态：[%s],距目标还行：%.2f 米", feedback->current_state.c_str(),
              feedback->distance_left);
}

// Action 回调3：最终执行结果
void NluNode::result_callback(const GoalHandleGrasp::WrappedResult& result) {
  switch (result.code) {
    case rclcpp_action::ResultCode::SUCCEEDED:
      RCLCPP_INFO(this->get_logger(), "🎉 抓取任务圆满完成！");
      break;
    case rclcpp_action::ResultCode::ABORTED:
      RCLCPP_ERROR(this->get_logger(), "❌ 抓取任务中止 (可能发生了碰撞)");
      break;
    case rclcpp_action::ResultCode::CANCELED:
      RCLCPP_ERROR(this->get_logger(), "🛑 抓取任务被强制取消");
      break;
    default:
      RCLCPP_ERROR(this->get_logger(), "❓ 未知状态");
      break;
  }
  // 流程结束，释放系统锁，准备迎接下一次语音指令！
  is_busy_ = false;
}

}  // namespace arm_agent_lite

// 注册为组件（便于未来使用 ComposableNode 提升性能）
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(arm_agent_lite::NluNode)

int main(int argc, char** argv) {
  // 1. 初始化 ROS2 环境
  rclcpp::init(argc, argv);

  // 2. 实例化 NLU 大脑节点
  auto node = std::make_shared<arm_agent_lite::NluNode>();

  // 3. 使用多线程执行器！
  // 节点既要收文本，又要发 Action，还要等 Service 响应。
  // 如果用单线程，很容易死锁。MultiThreadedExecutor 可以让不同回调在不同 CPU 核心上并发运行。
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);

  // 4. 让节点开始转起来，阻塞在这里，直到按 Ctrl+C
  executor.spin();

  // 5. 退出清理
  rclcpp::shutdown();
  return 0;
}