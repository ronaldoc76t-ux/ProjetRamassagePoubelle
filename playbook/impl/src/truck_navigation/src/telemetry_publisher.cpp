/**
 * @file telemetry_publisher.cpp
 * @brief Publishes truck telemetry at 10Hz
 * @author OpenClaw Team
 */

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/string.hpp>
#include <geometry_msgs/msg/pose.hpp>

class TelemetryPublisher : public rclcpp::Node {
public:
    TelemetryPublisher() : Node("telemetry_publisher") {
        RCLCPP_INFO(this->get_logger(), "Initializing TelemetryPublisher");

        // Parameters
        this->declare_parameter("publish_rate", 10.0);
        this->declare_parameter("truck_id", "truck_001");
        
        this->get_parameter("publish_rate", publish_rate_);
        this->get_parameter("truck_id", truck_id_);

        // Subscriptions
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/truck/odom", 10,
            std::bind(&TelemetryPublisher::odom_callback, this, std::placeholders::_1));
            
        gps_sub_ = this->create_subscription<sensor_msgs::msg::NavSatFix>(
            "/truck/gps", 10,
            std::bind(&TelemetryPublisher::gps_callback, this, std::placeholders::_1));
            
        battery_sub_ = this->create_subscription<std_msgs::msg::Float64>(
            "/truck/battery", 10,
            std::bind(&TelemetryPublisher::battery_callback, this, std::placeholders::_1));
            
        status_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/truck/status", 10,
            std::bind(&TelemetryPublisher::status_callback, this, std::placeholders::_1));

        // Publisher - Combined telemetry
        telemetry_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/telemetry/camion", 10);
        
        // Publisher - Raw GPS for external systems
        gps_pub_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("/telemetry/gps", 10);

        // Timer for 10Hz publishing
        telemetry_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate_)),
            std::bind(&TelemetryPublisher::publish_telemetry, this));

        RCLCPP_INFO(this->get_logger(), "TelemetryPublisher initialized at %.1f Hz", publish_rate_);
    }

private:
    void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg) {
        current_odom_ = *msg;
        has_odom_ = true;
    }

    void gps_callback(const sensor_msgs::msg::NavSatFix::SharedPtr msg) {
        current_gps_ = *msg;
        has_gps_ = true;
    }

    void battery_callback(const std_msgs::msg::Float64::SharedPtr msg) {
        battery_percent_ = msg->data;
        has_battery_ = true;
    }

    void status_callback(const std_msgs::msg::String::SharedPtr msg) {
        status_ = msg->data;
        has_status_ = true;
    }

    void publish_telemetry() {
        if (!has_odom_) {
            return;
        }

        // Publish GPS data
        if (has_gps_) {
            sensor_msgs::msg::NavSatFix gps_msg;
            gps_msg.header.stamp = this->now();
            gps_msg.header.frame_id = "gps";
            gps_msg.latitude = current_gps_.latitude;
            gps_msg.longitude = current_gps_.longitude;
            gps_msg.altitude = current_gps_.altitude;
            gps_pub_->publish(gps_msg);
        }

        // Publish combined telemetry
        nav_msgs::msg::Odometry telemetry_msg;
        telemetry_msg.header.stamp = this->now();
        telemetry_msg.header.frame_id = "map";
        telemetry_msg.child_frame_id = truck_id_;
        
        // Position
        telemetry_msg.pose.pose = current_odom_.pose.pose;
        
        // Velocity
        telemetry_msg.twist.twist = current_odom_.twist.twist;
        
        // Add custom fields in pose covariance (battery, status)
        if (has_battery_) {
            telemetry_msg.pose.covariance[0] = battery_percent_;  // Reuse field for battery
        }
        
        telemetry_pub_->publish(telemetry_msg);
    }

    // Subscriptions
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Subscription<sensor_msgs::msg::NavSatFix>::SharedPtr gps_sub_;
    rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr battery_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
    
    // Publishers
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr telemetry_pub_;
    rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr gps_pub_;
    
    // Timer
    rclcpp::TimerBase::SharedPtr telemetry_timer_;
    
    // State
    nav_msgs::msg::Odometry current_odom_;
    sensor_msgs::msg::NavSatFix current_gps_;
    double battery_percent_ = 100.0;
    std::string status_ = "UNKNOWN";
    std::string truck_id_;
    double publish_rate_;
    
    bool has_odom_ = false;
    bool has_gps_ = false;
    bool has_battery_ = false;
    bool has_status_ = false;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TelemetryPublisher>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}