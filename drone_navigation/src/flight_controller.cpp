/**
 * @file flight_controller.cpp
 * @brief Flight controller node for drone
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/battery_state.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/bool.hpp>

enum class FlightState {
    IDLE, TAKEOFF, CRUISE, APPROACH, DOCKED, RETURN, EMERGENCY
};

class FlightController : public rclcpp::Node {
public:
    FlightController() : Node("flight_controller"), state_(FlightState::IDLE) {
        RCLCPP_INFO(this->get_logger(), "Initializing FlightController");

        // Parameters
        this->declare_parameter("max_altitude", 100.0);
        this->declare_parameter("min_altitude", 2.0);
        this->declare_parameter("max_speed", 15.0);
        this->declare_parameter("approach_speed", 3.0);
        this->declare_parameter("docking_speed", 0.5);
        
        this->get_parameter("max_altitude", max_altitude_);
        this->get_parameter("min_altitude", min_altitude_);
        this->get_parameter("max_speed", max_speed_);

        // Subscriptions
        target_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/drone/target_pose", 10,
            std::bind(&FlightController::target_callback, this, std::placeholders::_1));
            
        cmd_takeoff_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/drone/cmd_takeoff", 10,
            std::bind(&FlightController::takeoff_callback, this, std::placeholders::_1));
            
        cmd_land_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/drone/cmd_land", 10,
            std::bind(&FlightController::land_callback, this, std::placeholders::_1));
            
        cmd_return_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/drone/cmd_return", 10,
            std::bind(&FlightController::return_callback, this, std::placeholders::_1));

        // Publishers
        pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/drone/pose", 10);
        cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/drone/cmd_vel", 10);
        battery_pub_ = this->create_publisher<sensor_msgs::msg::BatteryState>("/drone/battery_state", 10);
        status_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/status", 10);

        // Timer for control loop
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50),  // 20Hz
            std::bind(&FlightController::control_loop, this));
            
        // Timer for battery simulation
        battery_timer_ = this->create_wall_timer(
            std::chrono::seconds(1),
            std::bind(&FlightController::battery_loop, this));

        RCLCPP_INFO(this->get_logger(), "FlightController initialized");
    }

private:
    void target_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        current_target_ = *msg;
        has_target_ = true;
    }

    void takeoff_callback(const std_msgs::msg::Bool::SharedPtr msg) {
        if (msg->data && state_ == FlightState::IDLE) {
            state_ = FlightState::TAKEOFF;
            RCLCPP_INFO(this->get_logger(), "Takeoff command received");
            publish_status();
        }
    }

    void land_callback(const std_msgs::msg::Bool::SharedPtr msg) {
        if (msg->data) {
            state_ = FlightState::IDLE;
            RCLCPP_INFO(this->get_logger(), "Land command received");
            publish_status();
        }
    }

    void return_callback(const std_msgs::msg::Bool::SharedPtr msg) {
        if (msg->data) {
            state_ = FlightState::RETURN;
            RCLCPP_INFO(this->get_logger(), "Return command received");
            publish_status();
        }
    }

    void control_loop() {
        if (!has_target_ || state_ == FlightState::IDLE || state_ == FlightState::DOCKED) {
            return;
        }

        geometry_msgs::msg::Twist cmd;
        
        double dx = current_target_.pose.position.x - current_pose_.position.x;
        double dy = current_target_.pose.position.y - current_pose_.position.y;
        double dz = current_target_.pose.position.z - current_pose_.position.z;
        double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        double speed = (state_ == FlightState::APPROACH) ? 3.0 : max_speed_;
        
        if (distance > 0.1) {
            cmd.linear.x = (dx / distance) * std::min(speed, distance);
            cmd.linear.y = (dy / distance) * std::min(speed, distance);
            cmd.linear.z = (dz / distance) * std::min(speed, distance);
        }
        
        cmd_pub_->publish(cmd);
    }

    void battery_loop() {
        // Simulate battery drain
        battery_percent_ -= 0.5;  // 0.5% per second during flight
        
        if (battery_percent_ < 40.0 && state_ == FlightState::CRUISE) {
            RCLCPP_WARN(this->get_logger(), "Battery low (%.1f%%) - returning", battery_percent_);
            state_ = FlightState::RETURN;
        }
        
        if (battery_percent_ < 15.0) {
            RCLCPP_ERROR(this->get_logger(), "Battery critical - emergency landing");
            state_ = FlightState::EMERGENCY;
        }
        
        battery_percent_ = std::max(0.0, battery_percent_);
        
        sensor_msgs::msg::BatteryState battery_msg;
        battery_msg.header.stamp = this->now();
        battery_msg.percentage = battery_percent_;
        battery_msg.capacity = 970.0;  // Wh
        battery_msg.energy = battery_percent_ / 100.0 * 970.0;
        battery_pub_->publish(battery_msg);
        
        publish_status();
    }

    void publish_status() {
        std_msgs::msg::String status_msg;
        
        switch (state_) {
            case FlightState::IDLE: status_msg.data = "IDLE"; break;
            case FlightState::TAKEOFF: status_msg.data = "TAKEOFF"; break;
            case FlightState::CRUISE: status_msg.data = "CRUISE"; break;
            case FlightState::APPROACH: status_msg.data = "APPROACH"; break;
            case FlightState::DOCKED: status_msg.data = "DOCKED"; break;
            case FlightState::RETURN: status_msg.data = "RETURN"; break;
            case FlightState::EMERGENCY: status_msg.data = "EMERGENCY"; break;
            default: status_msg.data = "UNKNOWN"; break;
        }
        
        status_pub_->publish(status_msg);
    }

    // Subscriptions
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr target_sub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr cmd_takeoff_sub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr cmd_land_sub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr cmd_return_sub_;
    
    // Publishers
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
    rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    
    // Timers
    rclcpp::TimerBase::SharedPtr control_timer_;
    rclcpp::TimerBase::SharedPtr battery_timer_;
    
    // State
    FlightState state_;
    geometry_msgs::msg::Pose current_pose_;
    geometry_msgs::msg::PoseStamped current_target_;
    bool has_target_ = false;
    double battery_percent_ = 100.0;
    
    // Parameters
    double max_altitude_;
    double min_altitude_;
    double max_speed_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FlightController>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}