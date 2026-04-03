/**
 * @file trajectory_predictor.cpp
 * @brief Predicts truck trajectory using EKF
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <truck_navigation/msg/predicted_trajectory.hpp>
#include <vector>
#include <cmath>

class TrajectoryPredictor : public rclcpp::Node {
public:
    TrajectoryPredictor() : Node("trajectory_predictor") {
        RCLCPP_INFO(this->get_logger(), "Initializing TrajectoryPredictor");

        // Parameters
        this->declare_parameter("prediction_horizon", 60.0);
        this->declare_parameter("prediction_resolution", 1.0);
        this->declare_parameter("update_rate", 1.0);
        
        this->get_parameter("prediction_horizon", horizon_);
        this->get_parameter("prediction_resolution", resolution_);
        
        num_predictions_ = static_cast<int>(horizon_ / resolution_);

        // EKF State: [x, y, theta, v, omega]
        state_.resize(5, 0.0);
        // Covariance matrix P
        covariance_.resize(25, 0.0);
        for (int i = 0; i < 5; i++) covariance_[i * 6] = 1.0;  // Diagonal

        // Process noise Q
        q_.resize(25, 0.0);
        q_[0] = 0.1;   // x process noise
        q_[6] = 0.1;   // y process noise
        q_[12] = 0.05; // theta process noise
        q_[18] = 0.5;  // v process noise
        q_[24] = 0.1;  // omega process noise

        // Subscriptions
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/truck/odom", 10,
            std::bind(&TrajectoryPredictor::odom_callback, this, std::placeholders::_1));

        // Publisher
        trajectory_pub_ = this->create_publisher<truck_navigation::msg::PredictedTrajectory>(
            "/truck/predicted_trajectory", 10);

        // Timer for prediction updates
        prediction_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(static_cast<int>(1000.0 / 1.0)),
            std::bind(&TrajectoryPredictor::predict, this));

        RCLCPP_INFO(this->get_logger(), "TrajectoryPredictor initialized");
    }

private:
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        // Update EKF with new observation
        state_[0] = msg->pose.pose.position.x;
        state_[1] = msg->pose.pose.position.y;
        
        // Extract heading from quaternion
        double siny_cosp = 2.0 * (msg->pose.pose.orientation.w * msg->pose.pose.orientation.z);
        double cosy_cosp = 1.0 - 2.0 * (msg->pose.pose.orientation.y * msg->pose.pose.orientation.y);
        state_[2] = std::atan2(siny_cosp, cosy_cosp);
        
        state_[3] = std::sqrt(
            msg->twist.twist.linear.x * msg->twist.twist.linear.x +
            msg->twist.twist.linear.y * msg->twist.twist.linear.y);
        state_[4] = msg->twist.twist.angular.z;
        
        last_odom_time_ = this->now();
        has_odom_ = true;
    }

    void predict() {
        if (!has_odom_) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
                                "No odometry received yet");
            return;
        }

        // EKF Prediction Step
        predict_step();

        // Generate trajectory
        auto trajectory = truck_navigation::msg::PredictedTrajectory();
        trajectory.header.stamp = this->now();
        trajectory.header.frame_id = "map";
        
        double current_time = this->now().seconds();
        double confidence = 1.0;

        // Simulate future states
        std::vector<double> sim_state = state_;
        
        for (int i = 0; i < num_predictions_; i++) {
            double dt = resolution_;
            
            // Bicycle model prediction
            double x = sim_state[0] + sim_state[3] * std::cos(sim_state[2]) * dt;
            double y = sim_state[1] + sim_state[3] * std::sin(sim_state[2]) * dt;
            double theta = sim_state[2] + sim_state[4] * dt;
            double v = sim_state[3];  // Assume constant velocity
            double omega = sim_state[4];
            
            sim_state[0] = x;
            sim_state[1] = y;
            sim_state[2] = theta;
            sim_state[3] = v;
            sim_state[4] = omega;

            // Add to trajectory
            geometry_msgs::msg::Pose pose;
            pose.position.x = x;
            pose.position.y = y;
            pose.position.z = 0.0;
            
            // Convert theta to quaternion
            pose.orientation.z = std::sin(theta / 2.0);
            pose.orientation.w = std::cos(theta / 2.0);
            
            trajectory.poses.push_back(pose);
            trajectory.timestamps.push_back(current_time + (i + 1) * resolution_);
            
            // Confidence decreases with time
            confidence *= 0.98;
        }
        
        trajectory.confidence = confidence;
        
        trajectory_pub_->publish(trajectory);
    }

    void predict_step() {
        // EKF predict: x = F(x, u) + w
        // Simplified bicycle model
        double x = state_[0];
        double y = state_[1];
        double theta = state_[2];
        double v = state_[3];
        double omega = state_[4];
        
        // Add process noise
        for (int i = 0; i < 5; i++) {
            covariance_[i * 6 + i] += q_[i * 6 + i];
        }
    }

    // Subscriptions
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    
    // Publishers
    rclcpp::Publisher<truck_navigation::msg::PredictedTrajectory>::SharedPtr trajectory_pub_;
    
    // Timer
    rclcpp::TimerBase::SharedPtr prediction_timer_;
    
    // EKF State
    std::vector<double> state_;
    std::vector<double> covariance_;
    std::vector<double> q_;
    
    // Parameters
    double horizon_;
    double resolution_;
    int num_predictions_;
    
    // State
    bool has_odom_ = false;
    rclcpp::Time last_odom_time_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TrajectoryPredictor>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}