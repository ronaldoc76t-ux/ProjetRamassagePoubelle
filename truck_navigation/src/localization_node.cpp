/**
 * @file localization_node.cpp
 * @brief GPS RTK localization with sensor fusion
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <cmath>

class LocalizationNode : public rclcpp::Node {
public:
    LocalizationNode() : Node("localization_node") {
        RCLCPP_INFO(this->get_logger(), "Initializing LocalizationNode");

        // Parameters
        this->declare_parameter("gps_frame", "gps");
        this->declare_parameter("base_frame", "base_link");
        this->declare_parameter("map_frame", "map");
        this->declare_parameter("use_rtk", true);
        
        this->get_parameter("gps_frame", gps_frame_);
        this->get_parameter("base_frame", base_frame_);
        this->get_parameter("map_frame", map_frame_);
        this->get_parameter("use_rtk", use_rtk_);

        // Subscriptions
        gps_sub_ = this->create_subscription<sensor_msgs::msg::NavSatFix>(
            "/gps/fix", 10,
            std::bind(&LocalizationNode::gps_callback, this, std::placeholders::_1));
            
        imu_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/imu/data", 10,
            std::bind(&LocalizationNode::imu_callback, this, std::placeholders::_1));

        // Publishers
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/truck/odom", 10);
        
        // TF Broadcaster
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
        
        // TF Buffer for lookup
        tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_, *this);

        // Timer for odometry publishing
        odom_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(50),  // 20Hz
            std::bind(&LocalizationNode::publish_odom, this));

        RCLCPP_INFO(this->get_logger(), "LocalizationNode initialized (RTK: %s)", 
                    use_rtk_ ? "enabled" : "disabled");
    }

private:
    void gps_callback(const sensor_msgs::msg::NavSatFix::SharedPtr msg) {
        if (msg->status.status < 0) {
            RCLCPP_WARN(this->get_logger(), "GPS fix not available");
            return;
        }

        // Convert lat/lon/alt to local Cartesian coordinates
        // Using simple Mercator projection
        double lat = msg->latitude;
        double lon = msg->longitude;
        double alt = msg->altitude;
        
        // Reference point (origin)
        double ref_lat = 48.8566;  // Paris
        double ref_lon = 2.3522;
        
        double dlat = lat - ref_lat;
        double dlon = lon - ref_lon;
        
        // Approximate conversion to meters
        double x = dlon * 111320 * std::cos(ref_lat * M_PI / 180.0);
        double y = dlat * 110540;
        
        // RTK provides cm-level accuracy
        double position_variance = use_rtk_ ? 0.01 : 1.0;  // m²
        
        current_pose_.position.x = x;
        current_pose_.position.y = y;
        current_pose_.position.z = alt;
        
        has_gps_ = true;
    }

    void imu_callback(const sensor_msgs::msg::Imu::SharedPtr msg) {
        // Extract heading from IMU
        double siny_cosp = 2.0 * (msg->orientation.w * msg->orientation.z);
        double cosy_cosp = 1.0 - 2.0 * (msg->orientation.y * msg->orientation.y);
        double heading = std::atan2(siny_cosp, cosy_cosp);
        
        current_heading_ = heading;
        has_imu_ = true;
        
        // Angular velocity
        current_angular_velocity_.z = msg->angular_velocity.z;
        
        // Linear acceleration
        current_linear_accel_ = msg->linear_acceleration;
    }

    void publish_odom() {
        if (!has_gps_) {
            return;
        }

        // Build odometry message
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = this->now();
        odom.header.frame_id = map_frame_;
        odom.child_frame_id = base_frame_;
        
        odom.pose.pose.position = current_pose_;
        
        // Orientation from IMU or default
        if (has_imu_) {
            odom.pose.pose.orientation.z = std::sin(current_heading_ / 2.0);
            odom.pose.pose.orientation.w = std::cos(current_heading_ / 2.0);
        }
        
        odom.twist.twist.linear.x = 0.0;  // Would need wheel odometry
        odom.twist.twist.linear.y = 0.0;
        odom.twist.twist.angular.z = current_angular_velocity_.z;
        
        // Covariance
        for (int i = 0; i < 36; i++) {
            odom.pose.covariance[i] = (i % 7 == 0) ? 0.01 : 0.0;
            odom.twist.covariance[i] = (i % 7 == 0) ? 0.1 : 0.0;
        }
        
        odom_pub_->publish(odom);
        
        // Publish TF transform
        geometry_msgs::msg::TransformStamped transform;
        transform.header.stamp = this->now();
        transform.header.frame_id = map_frame_;
        transform.child_frame_id = base_frame_;
        
        transform.transform.translation.x = current_pose_.position.x;
        transform.transform.translation.y = current_pose_.position.y;
        transform.transform.translation.z = current_pose_.position.z;
        transform.transform.rotation = odom.pose.pose.orientation;
        
        tf_broadcaster_->sendTransform(transform);
    }

    // Subscriptions
    rclcpp::Subscription<sensor_msgs::msg::NavSatFix>::SharedPtr gps_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    
    // Publishers
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    
    // TF
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::unique_ptr<tf2_ros::TransformListener> tf_listener_;
    
    // Timer
    rclcpp::TimerBase::SharedPtr odom_timer_;
    
    // State
    geometry_msgs::msg::Point current_pose_;
    double current_heading_ = 0.0;
    geometry_msgs::msg::Vector3 current_angular_velocity_;
    geometry_msgs::msg::Vector3 current_linear_accel_;
    
    // Parameters
    std::string gps_frame_;
    std::string base_frame_;
    std::string map_frame_;
    bool use_rtk_;
    
    bool has_gps_ = false;
    bool has_imu_ = false;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<LocalizationNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}