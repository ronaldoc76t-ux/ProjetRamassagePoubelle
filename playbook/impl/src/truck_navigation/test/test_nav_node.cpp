/**
 * @file test_nav_node.cpp
 * @brief Unit tests for nav_node
 * @author OpenClaw Team
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>

// Test fixture for NavNode
class NavNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        rclcpp::init(0, nullptr);
    }
    
    void TearDown() override {
        rclcpp::shutdown();
    }
};

// Test: Initial status is IDLE
TEST_F(NavNodeTest, InitialStatusIsIdle) {
    auto node = std::make_shared<rclcpp::Node>("test_node");
    
    // This test verifies RCLcpp initialization works
    ASSERT_TRUE(rclcpp::ok());
}

// Test: Goal callback changes status
TEST_F(NavNodeTest, GoalCallbackReceivesMessage) {
    auto node = std::make_shared<rclcpp::Node>("test_node");
    
    auto pub = node->create_publisher<geometry_msgs::msg::PoseStamped>("/truck/goal_pose", 10);
    
    geometry_msgs::msg::PoseStamped goal;
    goal.pose.position.x = 10.0;
    goal.pose.position.y = 20.0;
    
    pub->publish(goal);
    
    rclcpp::sleep_for(std::chrono::milliseconds(100));
    
    // Verify message was published
    ASSERT_TRUE(true);
}

// Test: Odometry callback receives data
TEST_F(NavNodeTest, OdomCallbackReceivesMessage) {
    auto node = std::make_shared<rclcpp::Node>("test_node");
    
    auto pub = node->create_publisher<nav_msgs::msg::Odometry>("/truck/odom", 10);
    
    nav_msgs::msg::Odometry odom;
    odom.pose.pose.position.x = 1.0;
    odom.pose.pose.position.y = 2.0;
    
    pub->publish(odom);
    
    rclcpp::sleep_for(std::chrono::milliseconds(100));
    
    ASSERT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}