#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class NluNode : public rclcpp::Node {
public:
  NluNode() : Node("nlu_node") {
    // 1. 创建订阅者，使用 Lambda 表达式 [this](...) {...} 作为回调函数
    subscriber_ = this->create_subscription<std_msgs::msg::String>(
        "/command_text", 10,
        [this](const std_msgs::msg::String::SharedPtr msg) {
          this->processCommand(msg->data);
        });
    RCLCPP_INFO(this->get_logger(),
                "NLU Node 已启动，正在监听 /command_text...");
  }

private:
  // 2. 模拟自然语言解析（暂用简单的字符串匹配替代复杂的字典）
  void processCommand(const std::string &text) {
    RCLCPP_INFO(this->get_logger(), "收到原始文本: '%s'", text.c_str());

    // 使用 C++ string 的 find 方法查找关键字
    if (text.find("抓") != std::string::npos) {
      RCLCPP_INFO(this->get_logger(), "[解析结果]: 执行【抓取】动作");
    } else if (text.find("放") != std::string::npos) {
      RCLCPP_INFO(this->get_logger(), "[解析结果]: 执行【放置】动作");
    } else if (text.find("原点") != std::string::npos ||
               text.find("home") != std::string::npos) {
      RCLCPP_INFO(this->get_logger(), "[解析结果]: 执行【回原点】动作");
    } else {
      RCLCPP_WARN(this->get_logger(),
                  "[解析警告]: 未知指令，我听不懂你在说什么。");
    }
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscriber_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NluNode>());
  rclcpp::shutdown();
  return 0;
}
