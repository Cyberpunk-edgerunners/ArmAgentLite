#include <iostream>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <thread>

// 1.继承 rclcpp：：Node
class TextUiNode : public rclcpp::Node {
public:
  //构造函数
  TextUiNode() : Node("text_ui_node") {
    // 2.创建发布者(话题名：/command_text, 队列长度：10)
    publisher_ =
        this->create_publisher<std_msgs::msg::String>("/command_text", 10);

    RCLCPP_INFO(this->get_logger(), "Text UI Node 已启动！请输入指令");

    // 3.启动一个独立的后台线程接收键盘输入，防止阻塞ROS2主循环
    input_thread_ = std::thread([this]() { this->readInputLoop(); });
  }
  //析构函数：节点销毁时安全退出线程
  ~TextUiNode() {
    if (input_thread_.joinable()) {
      //工业级通常会有更优雅的退出机制，这里先简化处理
      input_thread_.detach();
    }
  }

private:
  void readInputLoop() {
    std::string input_str;
    //不断读取终端输入
    while (rclcpp::ok()) {
      std::getline(std::cin, input_str);
      if (!input_str.empty()) {
        // 4.构造消息并发送
        auto msg = std_msgs::msg::String();
        msg.data = input_str;
        publisher_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "已发送指令：%s", input_str.c_str());
      }
    }
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::thread input_thread_; // c++11 线程对象
};

// 程序的入口main函数
int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  //使用智能指针创建节点实例
  auto node = std::make_shared<TextUiNode>();
  //挂起节点，使其持续运行并处理事件
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}