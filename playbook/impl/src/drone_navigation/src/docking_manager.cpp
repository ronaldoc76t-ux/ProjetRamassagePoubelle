/**
 * @file docking_manager.cpp
 * @brief Docking manager state machine
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <std_msgs/msg/string.hpp>
#include <drone_navigation/msg/dock_status.hpp>
#include <drone_navigation/msg/dock_command.hpp>

enum class DockState { IDLE, APPROACH, ALIGNING, DESCENDING, DOCKED, ERROR, RETURN };

class DockingManager : public rclcpp::Node {
public:
    DockingManager() : Node("docking_manager"), state_(DockState::IDLE) {
        this->declare_parameter("docking_speed", 0.5);
        this->declare_parameter("aligning_timeout", 30.0);
        this->get_parameter("docking_speed", docking_speed_);

        cmd_sub_ = this->create_subscription<drone_navigation::msg::DockCommand>(
            "/drone/cmd_dock", 10,
            std::bind(&DockingManager::cmd_callback, this, std::placeholders::_1));
            
        truck_status_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/truck/rendezvous_status", 10,
            std::bind(&DockingManager::truck_status_callback, this, std::placeholders::_1));

        status_pub_ = this->create_publisher<drone_navigation::msg::DockStatus>("/drone/dock_status", 10);
        cmd_pos_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/drone/cmd_position", 10);

        RCLCPP_INFO(this->get_logger(), "DockingManager initialized");
    }

private:
    void cmd_callback(const drone_navigation::msg::DockCommand::SharedPtr msg) {
        if (msg->command == "dock") start_docking();
        else if (msg->command == "undock") start_undocking();
        else if (msg->command == "abort") abort_docking();
    }

    void truck_status_callback(const std_msgs::msg::String::SharedPtr msg) {
        truck_status_ = msg->data;
    }

    void start_docking() {
        RCLCPP_INFO(this->get_logger(), "Starting docking sequence");
        if (truck_status_ != "READY") {
            RCLCPP_WARN(this->get_logger(), "Truck not ready, waiting...");
            return;
        }
        state_ = DockState::APPROACH;
        publish_status();
    }

    void start_undocking() {
        RCLCPP_INFO(this->get_logger(), "Starting undocking");
        state_ = DockState::RETURN;
        publish_status();
    }

    void abort_docking() {
        RCLCPP_ERROR(this->get_logger(), "Docking aborted!");
        state_ = DockState::ERROR;
        publish_status();
    }

    void publish_status() {
        drone_navigation::msg::DockStatus status;
        status.header.stamp = this->now();
        status.docked = (state_ == DockState::DOCKED);
        
        switch (state_) {
            case DockState::IDLE: status.state = "IDLE"; break;
            case DockState::APPROACH: status.state = "APPROACHING"; break;
            case DockState::ALIGNING: status.state = "ALIGNING"; break;
            case DockState::DESCENDING: status.state = "DESCENDING"; break;
            case DockState::DOCKED: status.state = "DOCKED"; break;
            case DockState::ERROR: status.state = "ERROR"; break;
            case DockState::RETURN: status.state = "RETURN"; break;
            default: status.state = "UNKNOWN"; break;
        }
        status_pub_->publish(status);
    }

    rclcpp::Subscription<drone_navigation::msg::DockCommand>::SharedPtr cmd_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr truck_status_sub_;
    rclcpp::Publisher<drone_navigation::msg::DockStatus>::SharedPtr status_pub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr cmd_pos_pub_;
    
    DockState state_;
    double docking_speed_;
    double aligning_timeout_;
    std::string truck_status_ = "UNKNOWN";
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DockingManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}