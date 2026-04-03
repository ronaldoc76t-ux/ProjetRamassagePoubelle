# Implémentation S9: Simulation Complète + Optimisation

## Date de début: 2026-04-01

## 1. Architecture Simulation

```
simulation/
├── gazebo/
│   ├── worlds/           # Fichiers monde
│   ├── models/           # Modèles truck, drone
│   └── plugins/          # Plugins ROS2
├── scripts/
│   ├── run_simulation.sh # Script lancement
│   ├── analyze_results.py# Analyse résultats
│   └── visualize.py     # Visualisation Rviz
└── config/
    └── simulation.yaml  # Config paramètres
```

## 2. Monde Gazebo

```xml
<!-- worlds/openclaw_world.sdf -->
<sdf version="1.9">
  <world name="openclaw">
    <!-- Physique -->
    <physics type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
      <ode>
        <solver>
          <type>quick</type>
          <iters>50</iters>
          <sor>1.3</sor>
        </solver>
      </ode>
    </physics>

    <!-- Lumière -->
    <light type="directional" name="sun">
      <cast_shadows>true</cast_shadows>
      <pose>0 0 10 0 0 0</pose>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <specular>0.2 0.2 0.2 1</specular>
      <attenuation>
        <range>1000</range>
        <constant>0.9</constant>
        <linear>0.01</linear>
        <quadratic>0.001</quadratic>
      </attenuation>
    </light>

    <!-- Ground -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>1000 1000</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>1000 1000</size>
            </plane>
          </geometry>
          <material>
            <script>
              <uri>file://media/materials/scripts/gazebo.material</uri>
              <name>Gazebo/Grey</name>
            </script>
          </material>
        </visual>
      </link>
    </model>

    <!-- Routes -->
    <model name="road_loop">
      <static>true</static>
      <link name="link">
        <pose>0 0 0.01 0 0 0</pose>
        <collision name="collision">
          <geometry>
            <box>
              <size>500 8 0.02</size>
            </box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box>
              <size>500 8 0.02</size>
            </box>
          </geometry>
          <material>
            <ambient>0.2 0.2 0.2 1</ambient>
          </material>
        </visual>
      </link>
    </model>

    <!-- Zone de collection -->
    <model name="collection_zone_1">
      <static>true</static>
      <link name="link">
        <pose>50 0 0 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>5</radius>
              <height>0.1</height>
            </cylinder>
          </geometry>
          <material>
            <ambient>0 1 0 0.3</ambient>
          </material>
        </visual>
      </link>
    </model>
  </world>
</sdf>
```

## 3. Modèle Truck (SDF)

```xml
<!-- models/truck/model.sdf -->
<sdf version="1.9">
  <model name="autonomous_truck">
    <!-- Chassis -->
    <link name="chassis">
      <pose>0 0 0.5 0 0 0</pose>
      <inertial>
        <mass>15000</mass>
        <inertia>
          <ixx>15000</ixx>
          <iyy>15000</iyy>
          <izz>15000</izz>
        </inertia>
      </inertial>
      <collision name="collision">
        <geometry>
          <box>
            <size>8 2.5 3</size>
          </box>
        </geometry>
      </collision>
      <visual name="visual">
        <geometry>
          <box>
            <size>8 2.5 3</size>
          </box>
        </geometry>
        <material>
          <ambient>0.8 0.8 0.9 1</ambient>
        </material>
      </visual>
    </link>

    <!-- Roues -->
    <link name="wheel_fl">
      <pose>3 1.5 -0.3 0 0 0</pose>
      <collision>
        <geometry>
          <cylinder>
            <radius>0.6</radius>
            <length>0.5</length>
          </cylinder>
        </geometry>
      </collision>
    </link>
    <!-- ... autres wheels ... -->

    <!-- Plugins ROS2 -->
    <plugin name="truck_navigation" filename="libtruck_navigation.so">
      <ros>
        <namespace>/truck</namespace>
      </ros>
      <update_rate>10</update_rate>
      <cmd_vel_topic>/truck/cmd_vel</cmd_vel_topic>
      <odom_topic>/truck/odometry</odom_topic>
      <predict_trajectory>true</predict_trajectory>
    </plugin>
  </model>
</sdf>
```

## 4. Modèle Drone (SDF)

```xml
<!-- models/drone/model.sdf -->
<sdf version="1.9">
  <model name="collection_drone">
    <!-- Base -->
    <link name="base_link">
      <pose>0 0 0 0 0 0</pose>
      <inertial>
        <mass>5.0</mass>
        <inertia>
          <ixx>0.01</ixx>
          <iyy>0.01</iyy>
          <izz>0.02</izz>
        </inertia>
      </inertial>
      <collision name="collision">
        <geometry>
          <box>
            <size>0.5 0.5 0.2</size>
          </box>
        </geometry>
      </collision>
      <visual name="visual">
        <geometry>
          <box>
            <size>0.5 0.5 0.2</size>
          </box>
        </geometry>
        <material>
          <ambient>1 0 0 1</ambient>
        </material>
      </visual>
    </link>

    <!-- rotors -->
    <link name="rotor_0">
      <pose>0.3 0.3 0.1 0 0 0</pose>
      <visual>
        <geometry>
          <cylinder>
            <radius>0.15</radius>
            <length>0.05</length>
          </cylinder>
        </geometry>
      </visual>
    </link>
    <!-- ... 4 rotors ... -->

    <!-- Plugin Flight -->
    <plugin name="drone_flight_controller" filename="libdrone_flight_controller.so">
      <ros>
        <namespace>/drone</namespace>
      </ros>
      <update_rate>20</update_rate>
      <motors>4</motors>
      <max_thrust>60</max_thrust>
      <cmd_vel_topic>/drone/cmd_vel</cmd_vel_topic>
      <pose_topic>/drone/pose</pose_topic>
    </plugin>

    <!-- Plugin Docking -->
    <plugin name="docking_system" filename="libdocking_system.so">
      <ros>
        <namespace>/drone</namespace>
      </ros>
      <docking_speed>0.5</docking_speed>
      <dock_topic>/drone/dock_status</dock_topic>
    </plugin>
  </model>
</sdf>
```

## 5. Script de Simulation

```bash
#!/bin/bash
# scripts/run_simulation.sh

export GAZEBO_RESOURCE_PATH=/path/to/simulation/gazebo/models
export GAZEBO_PLUGIN_PATH=/path/to/simulation/gazebo/plugins

# Parameters
WORLD=${1:-openclaw_world}
DURATION=${2:-300}  # seconds
NUM_DRONES=${3:-3}

echo "Starting simulation: $WORLD"
echo "Duration: ${DURATION}s, Drones: $NUM_DRONES"

# Launch Gazebo
ros2 launch gazebo_ros gazebo.launch.py \
  world_name:=$(pwd)/worlds/${WORLD}.sdf \
  paused:=false \
  use_sim_time:=true \
  gui:=true &

# Wait for Gazebo
sleep 5

# Spawn truck
ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
  "{name: 'truck_001', xml: '$(cat models/truck/model.sdf)'}"

# Spawn drones
for i in $(seq 1 $NUM_DRONES); do
  ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
    "{name: 'drone_$(printf "%03d" $i)', xml: '$(cat models/drone/model.sdf)'}"
done

# Launch navigation nodes
ros2 launch truck_navigation truck.launch.py &
ros2 launch drone_navigation drone.launch.py &

# Run simulation for DURATION seconds
echo "Running simulation for ${DURATION}s..."
sleep $DURATION

# Collect results
echo "Collecting results..."
mkdir -p results

# Save bag
ros2 bag record -a -o results/bag_$(date +%Y%m%d_%H%M%S) &

# Save metrics
rostopic echo /truck/odometry > results/truck_odom.csv &
rostopic echo /drone/pose > results/drone_pose.csv &
rostopic echo /drone/battery_state > results/drone_battery.csv &

# Wait for data collection
sleep 10

# Kill processes
pkill -f gazebo
pkill -f ros2

echo "Simulation complete. Results in results/"
```

## 6. Analyse Résultats

```python
#!/usr/bin/env python3
# scripts/analyze_results.py

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys

def analyze_simulation(results_dir):
    """Analyze simulation results"""
    
    # Load data
    truck_odom = pd.read_csv(f"{results_dir}/truck_odom.csv")
    drone_pose = pd.read_csv(f"{results_dir}/drone_pose.csv")
    drone_battery = pd.read_csv(f"{results_dir}/drone_battery.csv")
    
    results = {}
    
    # 1. Truck metrics
    results['truck_distance'] = calculate_distance(truck_odom)
    results['truck_avg_speed'] = truck_odom['twist.twist.linear.x'].mean()
    
    # 2. Drone metrics
    results['docking_success'] = calculate_docking_success(drone_pose)
    results['docking_attempts'] = len(drone_pose[drone_pose['z'] < 1.0])
    results['avg_battery'] = drone_battery['percentage'].mean()
    
    # 3. Rendezvous metrics
    results['rv_count'] = count_rendezvous(drone_pose, truck_odom)
    results['rv_avg_timing_error'] = calculate_timing_error(drone_pose, truck_odom)
    
    # 4. Generate plots
    plot_trajectories(truck_odom, drone_pose, results_dir)
    plot_battery(drone_battery, results_dir)
    
    return results

def calculate_distance(df):
    """Calculate total distance traveled"""
    dx = df['pose.pose.position.x'].diff()
    dy = df['pose.pose.position.y'].diff()
    return np.sqrt(dx**2 + dy**2).sum()

def calculate_docking_success(drone_pose):
    """Calculate docking success rate"""
    docking_events = drone_pose[drone_pose['z'] < 0.5]
    # Success = stayed docked for > 5 seconds
    return len(docking_events) / max(len(drone_pose), 1)

def plot_trajectories(truck_df, drone_df, output_dir):
    """Plot truck and drone trajectories"""
    plt.figure(figsize=(12, 8))
    
    plt.plot(truck_df['pose.pose.position.x'], 
             truck_df['pose.pose.position.y'], 
             'b-', label='Truck', linewidth=2)
    
    plt.scatter(drone_df['pose.pose.position.x'].iloc[0],
                drone_df['pose.pose.position.y'].iloc[0],
                'go', label='Drone Start', s=100)
    plt.scatter(drone_df['pose.pose.position.x'].iloc[-1],
                drone_df['pose.pose.position.y'].iloc[-1],
                'ro', label='Drone End', s=100)
    
    plt.xlabel('X (m)')
    plt.ylabel('Y (m)')
    plt.title('Truck vs Drone Trajectories')
    plt.legend()
    plt.grid(True)
    plt.savefig(f"{output_dir}/trajectories.png")
    plt.close()

if __name__ == "__main__":
    results = analyze_simulation(sys.argv[1] or "results")
    print("\n=== Simulation Results ===")
    for key, value in results.items():
        print(f"{key}: {value}")
```

## 7. Tests de Performance

```cpp
// test/performance_test.cpp

#include <gtest/gtest.h>
#include <chrono>

class PerformanceTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize simulation
    }
};

// Test: Docking success rate
TEST_F(PerformanceTest, DockingSuccessRate) {
    const int attempts = 100;
    int success = 0;
    
    for (int i = 0; i < attempts; i++) {
        if (run_docking_simulation()) {
            success++;
        }
    }
    
    double rate = static_cast<double>(success) / attempts;
    EXPECT_GT(rate, 0.90); // 90% min
}

// Test: Timing precision
TEST_F(PerformanceTest, TimingPrecision) {
    const int tests = 50;
    std::vector<double> errors;
    
    for (int i = 0; i < tests; i++) {
        double error = measure_timing_error();
        errors.push_back(error);
    }
    
    double avg_error = std::accumulate(errors.begin(), errors.end(), 0.0) / errors.size();
    EXPECT_LT(avg_error, 0.5); // 500ms max
}

// Test: Battery consumption
TEST_F(PerformanceTest, BatteryConsumption) {
    double initial = 100.0;
    double final = simulate_mission_cycle(initial);
    
    EXPECT_GT(final, 25.0); // Should have >25% remaining
}

// Test: Failover time
TEST_F(PerformanceTest, FailoverTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    trigger_failure();
    wait_for_failover();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    EXPECT_LT(duration.count(), 30); // 30s max
}
```

## 8. Configuration Simulation

```yaml
# config/simulation.yaml
simulation:
  world: "openclaw_world"
  duration: 300  # seconds
  time_scale: 1.0
  
  truck:
    model: "autonomous_truck"
    initial_pose: [0, 0, 0, 0, 0, 0]
    speed: 15  # m/s
    trajectory: "loop"
    
  drones:
    count: 3
    model: "collection_drone"
    initial_altitude: 10  # m
    
  scenarios:
    - name: "nominal"
      duration: 300
    - name: "high_traffic"
      duration: 300
     干扰: true
    - name: "battery_low"
      duration: 300
      battery_start: 30
      
  metrics:
    - docking_success_rate
    - timing_precision
    - battery_efficiency
    - failover_time
```

## 9. Critères de Definition of Done

- [ ] Monde Gazebo opérationnel
- [ ] Modèles truck + drone spawnables
- [ ] Tests performance > 90% pass
- [ ] Analyse automatisée fonctionnelle
- [ ] Peer review approuvée

---

*Document généré le 2026-04-01*
*Prochaine étape: S10 Application Mobile MVP*