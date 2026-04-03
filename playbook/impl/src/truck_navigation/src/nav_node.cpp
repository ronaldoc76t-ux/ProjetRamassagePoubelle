/**
 * @file nav_node.cpp
 * @brief Truck navigation node using Nav2
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <memory>
#include <string>

using NavigateToPose = nav2_msgs::action::NavigateToPose;
using GoalHandleNavigate = rclcpp_action::ClientGoalHandle<NavigateToPose>;

class NavNode : public rclcpp::Node {
public:
    NavNode() : Node("nav_node"), navigation_action_client_(nullptr) {
        RCLCPP_INFO(this->get_logger(), "Initializing NavNode");

        // Parameters
        this->declare_parameter("max_speed", 5.0);
        this->declare_parameter("min_speed", 1.0);
        this->declare_parameter("acceleration_limit", 0.5);
        this->declare_parameter("deceleration_limit", 1.0);
        
        this->get_parameter("max_speed", max_speed_);
        this->get_parameter("min_speed", min_speed_);

        // Subscriptions
        goal_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/truck/goal_pose", 10,
            std::bind(&NavNode::goal_callback, this, std::placeholders::_1));
        
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/truck/odom", 10,
            std::bind(&NavNode::odom_callback, this, std::placeholders::_1));

        // Publishers
        cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/truck/cmd_vel", 10);
        status_pub_ = this->create_publisher<std_msgs::msg::String>("/truck/status", 10);
        
        // Services
        start_srv_ = this->create_service<std_srvs::srv::SetBool>(
            "/truck/start_navigation",
            std::bind(&NavNode::start_navigation_cb, this, std::placeholders::_1, std::placeholders::_2));
            
        stop_srv_ = this->create_service<std_srvs::srv::Empty>(
            "/truck/stop_navigation",
            std::bind(&NavNode::stop_navigation_cb, this, std::placeholders::_1, std::placeholders::_2));

        // Navigation action client (optional - for Nav2 integration)
        // navigation_action_client_ = rclcpp_action::create_client<NavigateToPose>(this, "navigate_to_pose");

        // TF buffer
        tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_, *this);

        // Timer for control loop
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50),  // 20Hz control
            std::bind(&NavNode::control_loop, this));

        RCLCPP_INFO(this->get_logger(), "NavNode initialized successfully");
    }

private:
    void goal_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(), "New goal received: (%.2f, %.2f)", 
                    msg->pose.position.x, msg->pose.position.y);
        
        current_goal_ = *msg;
        has_goal_ = true;
        status_ = "NAVIGATING";
        publish_status();
    }

    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        current_pose_ = msg->pose.pose;
        current_velocity_ = msg->twist.twist;
    }

    void control_loop() {
        if (!has_goal_ || status_ != "NAVIGATING") {
            return;
        }

        // Simple P-controller towards goal
        geometry_msgs::msg::Twist cmd;
        
        double dx = current_goal_.pose.position.x - current_pose_.position.x;
        double dy = current_goal_.pose.position.y - current_pose_.position.y;
        double distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance < 0.5) {
            // Goal reached
            status_ = "IDLE";
            has_goal_ = false;
            cmd.linear.x = 0.0;
            cmd.angular.z = 0.0;
            RCLCPP_INFO(this->get_logger(), "Goal reached!");
        } else {
            // Calculate heading to goal
            double target_heading = std::atan2(dy, dx);
            
            // Current heading from quaternion
            double current_heading = std::atan2(
                2.0 * (current_pose_.orientation.w * current_pose_.orientation.z),
                1.0 - 2.0 * (current_pose_.orientation.y * current_pose_.orientation.y));
            
            // Angular velocity (simple P controller)
            double heading_error = target_heading - current_heading;
            // Normalize to -PI to PI
            while (heading_error > M_PI) heading_error -= 2 * M_PI;
            while (heading_error < -M_PI) heading_error += 2 * M_PI;
            
            cmd.angular.z = 2.0 * heading_error;  // P gain = 2.0
            cmd.linear.x = std::min(max_speed_, distance);  // Linear velocity
        }
        
        cmd_pub_->publish(cmd);
        publish_status();
    }

    void publish_status() {
        std_msgs::msg::String status_msg;
        status_msg.data = status_;
        status_pub_->publish(status_msg);
    }

    bool start_navigation_cb(const std_srvs::srv::SetBool::Request::SharedPtr,
                            std_srvs::srv::SetBool::Response::SharedPtr res) {
        status_ = "NAVIGATING";
        publish_status();
        res->success = true;
        res->message = "Navigation started";
        return true;
    }

    bool stop_navigation_cb(const std_srvs::srv::Empty::Request::SharedPtr,
                            std_srvs::srv::Empty::Response::SharedPtr) {
        status_ = "IDLE";
        has_goal_ = false;
        
        // Stop the truck
        geometry_msgs::msg::Twist cmd;
        cmd.linear.x = 0.0;
        cmd.angular.z = 0.0;
        cmd_pub_->publish(cmd);
        
        publish_status();
        return true;
    }

    // Subscriptions
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_sub_;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    
    // Publishers
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    
    // Services
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr start_srv_;
    rclcpp::Service<std_srvs::srv::Empty>::SharedPtr stop_srv_;
    
    // Timer
    rclcpp::TimerBase::SharedPtr control_timer_;
    
    // Action client (optional)
    std::shared_ptr<rclcpp_action::Client<NavigateToPose>> navigation_action_client_;
    
    // TF
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::unique_ptr<tf2_ros::TransformListener> tf_listener_;
    
    // State
    std::string status_ = "IDLE";
    bool has_goal_ = false;
    geometry_msgs::msg::PoseStamped current_goal_;
    geometry_msgs::msg::Pose current_pose_;
    geometry_msgs::msg::Twist current_velocity_;
    
    // Parameters
    double max_speed_;
    double min_speed_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<NavNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}