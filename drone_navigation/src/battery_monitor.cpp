/**
 * @file battery_monitor.cpp
 * @brief Battery monitoring node
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <std_msgs/msg/string.hpp>

class BatteryMonitor : public rclcpp::Node {
public:
    BatteryMonitor() : Node("battery_monitor"), battery_percent_(100.0) {
        this->declare_parameter("battery_critical", 15.0);
        this->declare_parameter("battery_warning", 25.0);
        this->declare_parameter("battery_return", 40.0);
        this->get_parameter("battery_critical", critical_);
        this->get_parameter("battery_warning", warning_);
        this->get_parameter("battery_return", return_);

        battery_sub_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
            "/drone/battery_state", 10,
            std::bind(&BatteryMonitor::battery_callback, this, std::placeholders::_1));
            
        status_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/battery_alert", 10);

        RCLCPP_INFO(this->get_logger(), "BatteryMonitor initialized");
    }

private:
    void battery_callback(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
        battery_percent_ = msg->percentage;
        
        if (battery_percent_ <= critical_) {
            publish_alert("CRITICAL", "Immediate return required");
        } else if (battery_percent_ <= warning_) {
            publish_alert("WARNING", "Return recommended");
        } else if (battery_percent_ <= return_) {
            publish_alert("LOW", "Return to base");
        }
    }

    void publish_alert(const std::string& level, const std::string& message) {
        std_msgs::msg::String alert;
        alert.data = level + ": " + message + " (" + std::to_string(battery_percent_) + "%)";
        status_pub_->publish(alert);
    }

    rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    double battery_percent_;
    double critical_, warning_, return_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<BatteryMonitor>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}