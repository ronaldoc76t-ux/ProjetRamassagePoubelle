# Implémentation S3-S4: PoC Camion ROS2 + Prédicteur Trajectoire

## Date de début: 2026-04-01

## 1. Architecture des Nodes ROS2

```
/truck/
├── nav_node              # Navigation avec Nav2
├── trajectory_predictor # Prédiction EKF + LSTM
├── telemetry_publisher  # Publication position 10Hz
├── truck_safety_monitor # Surveillance sécurité
└── localization_node    # Localisation GPS RTK + fusion
```

## 2. nav_node - Specification

### Topics Subscribe
- `/truck/goal_pose` (geometry_msgs/PoseStamped) - Destination
- `/map` (nav_msgs/OccupancyGrid) - Carte

### Topics Publish
- `/truck/cmd_vel` (geometry_msgs/Twist) - Commandes vitesse
- `/truck/odom` (nav_msgs/Odometry) - Odométrie
- `/truck/status` (std_msgs/String) - Statut (IDLE, NAVIGATING, EMERGENCY)

### Services
- `/truck/start_navigation` - Démarrer navigation
- `/truck/stop_navigation` - Arrêter
- `/truck/pause_navigation` - Pause

### Paramètres
```yaml
/max_speed: 5.0  # m/s (18 km/h)
/min_speed: 1.0 # m/s
/acceleration_limit: 0.5 # m/s²
/decelaration_limit: 1.0 # m/s²
/planner: smac_planner # ou navfn
```

## 3. trajectory_predictor - Specification

### Input
- Position actuelle `/truck/odom`
- Vitesse `/truck/cmd_vel`
- Heading (IMU)

### Output
- `/truck/predicted_trajectory` (custom msg)
```yaml
Header header
geometry_msg/Pose[] poses  # 30-120 poses
float64[] timestamps       # Unix timestamps
float64 confidence         # 0-1
```

### Algorithme
1. **EKF (Extended Kalman Filter)** - Modèle bicycle
2. **LSTM** - Apprentissage pour perturbations trafic
3. **Fusion** - Pondération selon confiance

### Configuration
```yaml
/prediction_horizon: 60  # secondes
/prediction_resolution: 1 # seconde
/model_weights: 0.3 EKF + 0.7 LSTM  # fusion
/update_rate: 1.0  # Hz
```

## 4. telemetry_publisher - Specification

### Publication 10Hz
```yaml
Header header
float64 latitude
float64 longitude
float64 heading  # degrees
float64 speed    # m/s
float64 battery_percent
string status
```

### Topics
- `/telemetry/camion/{id}` (MQTT bridge optionnel)

## 5. Code Structure

```
src/truck/
├── package.xml
├── CMakeLists.txt
├── launch/
│   └── truck.launch.py
├── config/
│   └── params.yaml
├── src/
│   ├── nav_node.cpp
│   ├── trajectory_predictor.cpp
│   ├── telemetry_publisher.cpp
│   └── localization_node.cpp
├── msg/
│   └── PredictedTrajectory.msg
├── srv/
│   ├── StartNavigation.srv
│   └── StopNavigation.srv
└── test/
    ├── test_nav_node.cpp
    └── test_trajectory_predictor.cpp
```

## 6. Code - nav_node.cpp (Pseudo-code)

```cpp
#include <rclcpp/rclcpp.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>

class NavNode : public rclcpp::Node {
public:
    NavNode() : Node("nav_node") {
        // Subscription to goal
        goal_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "/truck/goal_pose", 10,
            std::bind(&NavNode::goal_callback, this, std::placeholders::_1));
        
        // Publisher cmd_vel
        cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/truck/cmd_vel", 10);
        
        // Status publisher
        status_pub_ = this->create_publisher<std_msgs::msg::String>("/truck/status", 10);
        
        RCLCPP_INFO(this->get_logger(), "NavNode initialized");
    }

private:
    void goal_callback(const geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(), "New goal received");
        status_ = "NAVIGATING";
        // Implémenter logique Nav2
    }
    
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    std::string status_ = "IDLE";
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<NavNode>());
    rclcpp::shutdown();
    return 0;
}
```

## 7. Tests Unitaires

### Test 1: nav_node
```cpp
TEST(NavNodeTest, StartNavigation) {
    auto node = std::make_shared<NavNode>();
    // Publish goal pose
    // Verify status changes to NAVIGATING
    // Verify cmd_vel published
}
```

### Test 2: trajectory_predictor
```cpp
TEST(TrajectoryPredictorTest, PredictionHorizon) {
    auto predictor = std::make_shared<TrajectoryPredictor>();
    // Send odom input
    // Verify output has 60 poses (horizon 60s)
    // Verify timestamps are 1s apart
}
```

## 8. Critères de Definition of Done

- [ ] Package ROS2 compilé sans warning
- [ ] Tests unitaires > 80% coverage
- [ ] Documentation API des topics/services
- [ ] Launch file fonctionnel
- [ ] Peer review approuvée

---

## 9. Livrables

| Livrable | Status | Notes |
|----------|--------|-------|
| Package ROS2 `truck_navigation` | ⏳ À faire | |
| Config YAML | ⏳ À faire | |
| Launch file | ⏳ À faire | |
| Tests unitaires | ⏳ À faire | |
| Documentation | ⏳ À faire | |

---

*Document généré le 2026-04-01*
*Prochaine étape: Implémentation code + Peer Review*