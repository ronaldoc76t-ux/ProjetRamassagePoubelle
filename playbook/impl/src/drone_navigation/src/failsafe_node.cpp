/**
 * @file failsafe_node.cpp
 * @brief Failsafe node for emergency procedures
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <sensor_msgs/msg/battery_state.hpp>

class FailsafeNode : public rclcpp::Node {
public:
    FailsafeNode() : Node("failsafe_node") {
        this->declare_parameter("max_altitude_loss", 50.0);  // m
        this->declare_parameter("gps_timeout", 5.0);  // s
        this->declare_parameter("rc_timeout", 2.0);  // s

        status_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/drone/status", 10,
            std::bind(&FailsafeNode::status_callback, this, std::placeholders::_1));
            
        battery_sub_ = this->create_subscription<sensor_msgs::msg::BatteryState>(
            "/drone/battery_state", 10,
            std::bind(&FailsafeNode::battery_callback, this, std::placeholders::_1));

        emergency_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/emergency", 10);
        return_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/drone/return_pose", 10);

        RCLCPP_INFO(this->get_logger(), "FailsafeNode initialized");
    }

private:
    void status_callback(const std_msgs::msg::String::SharedPtr msg) {
        status_ = msg->data;
        
        if (status_ == "EMERGENCY") {
            trigger_emergency("Status Emergency");
        }
    }

    void battery_callback(const sensor_msgs::msg::BatteryState::SharedPtr msg) {
        if (msg->percentage <= 15.0) {
            trigger_emergency("Battery Critical");
        }
    }

    void trigger_emergency(const std::string& reason) {
        RCLCPP_ERROR(this->get_logger(), "EMERGENCY TRIGGERED: %s", reason.c_str());
        
        std_msgs::msg::String emergency;
        emergency.data = reason;
        emergency_pub_->publish(emergency);
        
        // Send return to base command
        geometry_msgs::msg::PoseStamped return_pose;
        return_pose.header.stamp = this->now();
        return_pose.header.frame_id = "map";
        return_pose.pose.position.x = 0.0;
        return_pose.pose.position.y = 0.0;
        return_pose.pose.position.z = 10.0;
        return_pub_->publish(return_pose);
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
    rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr battery_sub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr emergency_pub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr return_pub_;
    
    std::string status_;
    double max_altitude_loss_, gps_timeout_, rc_timeout_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FailsafeNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}