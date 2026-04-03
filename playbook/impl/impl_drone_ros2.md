# Implémentation S5-S6: PoC Drone ROS2 + Contrôle Docking

## Date de début: 2026-04-01

## 1. Architecture des Nodes ROS2 Drone

```
/drone{ID}/
├── flight_controller     # Contrôleur de vol PX4
├── perception_node       # Détection objets (YOLO)
├── approach_controller  # Approche dynamique du camion
├── docking_manager      # Gestion docking/undocking
├── battery_monitor      # Surveillance batterie
├── failsafe_node        # Gestion urgences
└── comm_interface       # Communication DDS
```

## 2. flight_controller - Specification

### Topics Subscribe
- `/drone{ID}/target_pose` (geometry_msgs/PoseStamped) - Position cible
- `/drone{ID}/cmd_takeoff` (std_msgs/Bool) - Décollage
- `/drone{ID}/cmd_land` (std_msgs/Bool) - Atterrissage
- `/drone{ID}/cmd_return` (std_msgs/Bool) - Retour base

### Topics Publish
- `/drone{ID}/pose` (geometry_msgs/PoseStamped) - Position actuelle
- `/drone{ID}/battery_state` (sensor_msgs/BatteryState) - Batterie
- `/drone{ID}/status` (std_msgs/String) - Status (IDLE, TAKEOFF, CRUISE, APPROACH, DOCKED, RETURN, EMERGENCY)

### Parameters
```yaml
/max_altitude: 100.0  # m
/min_altitude: 2.0   # m
/max_speed: 15.0    # m/s
/approach_speed: 3.0 # m/s (lorsque proche du camion)
/docking_speed: 0.5  # m/s
/gps_timeout: 5.0    # seconds
```

## 3. perception_node - Specification

### Input
- `/camera/image_raw` (sensor_msgs/Image) - Flux caméra
- `/lidar/points` (sensor_msgs/PointCloud2) - Nuage de points

### Output
- `/drone{ID}/detected_objects` (custom msg)
```yaml
Header header
Object[] objects
---
Object:
  string label      # "sac", "bac", "dechet"
  float64 confidence
  geometry_msgs/Pose pose
  float64 distance
```

### Modèle
- **YOLOv8** ou **SSD-MobileNet** pour détection en temps réel
- Inference sur NVIDIA Jetson (Edge)
- FPS cible: 15-30 FPS

### Filtrage
- Ignorer objets < 0.5m (trop proches)
- Ignorer confiance < 0.7
- Tracker par ID pour éviter double détection

## 4. approach_controller - Specification

### Input
- `/truck/predicted_trajectory` - Trajectoire prédite du camion
- `/drone{ID}/detected_objects` - Objets détectés
- `/drone{ID}/pose` - Position drone

### Output
- `/drone{ID}/approach_pose` - Position d'approche optimale

### Algorithme
1. **Predict truck position** à l'instant de rendez-vous estimé
2. **Calculate intercept point** avec offset latéral (2m)
3. **Generate approach trajectory** avec spline smooth
4. **Adjust** en temps réel selon drift truck

### Paramètres
```yaml
/intercept_distance: 2.0  # m offset from truck center
/approach_height: 3.0     # m above truck
/time_horizon: 30.0       # s predicted position
/min_approach_altitude: 1.5  # m safety margin
```

## 5. docking_manager - Specification

### Input
- `/drone{ID}/approach_pose` - Position d'approche
- `/truck/rendezvous_status` - Statut RV du camion
- `/drone{ID}/dock_command` - Commande de docking

### Output
- `/drone{ID}/dock_status` - Statut (APPROACHING, ALIGNING, DESCENDING, DOCKED, ERROR)
- `/drone{ID}/cmd_position` - Commandes position finale

### States Machine
```
IDLE → TAKEOFF → CRUISE → APPROACH → ALIGNING → DESCENDING → DOCKED
                                              ↓
                                          ERROR → RETURN
```

### Docking Protocol
1. **Request**: Drone demande RV au camion
2. **Acknowledge**: Camion confirme slot disponible
3. **Approach**: Drone approche positionnée
4. **Align**: Alignement vision (AprilTags)
5. **Descend**: Descente contrôlée (< 0.5 m/s)
6. **Dock**: Vérification magnétique + confirmation
7. **Release**: Sur commande, dépose sac, redécollage

### Précision Cible
- Position: < 30cm
- Heading: < 10°
- Timing: sync avec précision 500ms

## 6. battery_monitor - Specification

### Thresholds
```yaml
/battery_critical: 15.0   # % - Retour immédiat
/battery_warning: 25.0   # % - Alerte
/battery_low: 40.0        # % - Fin mission
/battery_return: 40.0     # % - Seuil retour obligatoire
```

### Actions Automatiques
- **40%**: Priorité retour base
- **25%**: Abandon collection, retour
- **15%**: Emergency landing zone la plus proche

## 7. Code Structure

```
src/drone/
├── package.xml
├── CMakeLists.txt
├── launch/
│   └── drone.launch.py
├── config/
│   └── params.yaml
├── src/
│   ├── flight_controller.cpp
│   ├── perception_node.cpp
│   ├── approach_controller.cpp
│   ├── docking_manager.cpp
│   ├── battery_monitor.cpp
│   └── failsafe_node.cpp
├── msg/
│   ├── DetectedObject.msg
│   ├── DetectedObjects.msg
│   ├── DockStatus.msg
│   └── DockCommand.msg
└── test/
    └── test_drone_nodes.cpp
```

## 8. Code - docking_manager.cpp (Extrait)

```cpp
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <std_msgs/msg/string.hpp>
#include <drone_navigation/msg/dock_status.hpp>
#include <drone_navigation/msg/dock_command.hpp>

enum class DockState {
    IDLE,
    TAKEOFF,
    CRUISE,
    APPROACH,
    ALIGNING,
    DESCENDING,
    DOCKED,
    ERROR,
    RETURN
};

class DockingManager : public rclcpp::Node {
public:
    DockingManager() : Node("docking_manager"), state_(DockState::IDLE) {
        // Parameters
        this->declare_parameter("docking_speed", 0.5);
        this->declare_parameter("aligning_timeout", 30.0);
        this->get_parameter("docking_speed", docking_speed_);

        // Subscriptions
        cmd_sub_ = this->create_subscription<drone_navigation::msg::DockCommand>(
            "/drone/cmd_dock", 10,
            std::bind(&DockingManager::cmd_callback, this, std::placeholders::_1));
            
        truck_status_sub_ = this->create_subscription<std_msgs::msg::String>(
            "/truck/rendezvous_status", 10,
            std::bind(&DockingManager::truck_status_callback, this, std::placeholders::_1));

        // Publishers
        status_pub_ = this->create_publisher<drone_navigation::msg::DockStatus>("/drone/dock_status", 10);
        cmd_pos_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/drone/cmd_position", 10);

        RCLCPP_INFO(this->get_logger(), "DockingManager initialized");
    }

private:
    void cmd_callback(const drone_navigation::msg::DockCommand::SharedPtr msg) {
        if (msg->command == "dock") {
            start_docking();
        } else if (msg->command == "undock") {
            start_undocking();
        }
    }

    void start_docking() {
        RCLCPP_INFO(this->get_logger(), "Starting docking sequence");
        state_ = DockState::APPROACH;
        publish_status();
    }

    void start_undocking() {
        RCLCPP_INFO(this->get_logger(), "Starting undocking sequence");
        state_ = DockState::RETURN;
        publish_status();
    }

    void publish_status() {
        drone_navigation::msg::DockStatus status;
        status.header.stamp = this->now();
        
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
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DockingManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
```

## 9. Critères de Definition of Done

- [ ] Package ROS2 `drone_navigation` compilé
- [ ] Tests unitaires > 80% coverage
- [ ] Simulation Gazebo fonctionnelle
- [ ] Docking成功率 > 90% en simulation
- [ ] Peer review approuvée

## 10. Livrables

| Livrable | Status |
|----------|--------|
| Package ROS2 `drone_navigation` | ⏳ À faire |
| Config YAML | ⏳ À faire |
| Launch file | ⏳ À faire |
| Tests unitaires | ⏳ À faire |
| Documentation | ⏳ À faire |

---

*Document généré le 2026-04-01*
*Prochaine étape: Implémentation code + Peer Review*