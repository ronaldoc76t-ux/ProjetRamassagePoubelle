#!/bin/bash
# simulation/scripts/run_simulation.sh
# Launch OpenClaw simulation in Gazebo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default parameters
WORLD=${1:-openclaw_world}
DURATION=${2:-300}
NUM_DRONES=${3:-3}
TIME_SCALE=${4:-1.0}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check ROS2
check_ros() {
    if ! command -v ros2 &> /dev/null; then
        log_error "ROS2 not found. Please install ros-jazzy-ros2-* packages."
        exit 1
    fi
    log_info "ROS2 found: $(ros2 --version)"
}

# Set paths
setup_paths() {
    export GAZEBO_RESOURCE_PATH="$SIM_ROOT/gazebo/worlds:$SIM_ROOT/gazebo/models:$GAZEBO_RESOURCE_PATH"
    export GAZEBO_PLUGIN_PATH="$SIM_ROOT/gazebo/plugins:$GAZEBO_PLUGIN_PATH"
    log_info "GAZEBO_RESOURCE_PATH: $GAZEBO_RESOURCE_PATH"
}

# Kill existing processes
cleanup() {
    log_info "Cleaning up existing processes..."
    pkill -f gazebo 2>/dev/null || true
    pkill -f ros 2>/dev/null || true
    sleep 1
}

# Launch Gazebo
launch_gazebo() {
    log_info "Launching Gazebo with world: $WORLD"
    
    ros2 launch gazebo_ros gazebo.launch.py \
        world_name:="$SIM_ROOT/gazebo/worlds/${WORLD}.sdf" \
        paused:=false \
        use_sim_time:=true \
        gui:=true \
        verbose:=false &
    
    GAZEBO_PID=$!
    log_info "Gazebo started (PID: $GAZEBO_PID)"
    
    # Wait for Gazebo to initialize
    sleep 5
    
    if ! kill -0 $GAZEBO_PID 2>/dev/null; then
        log_error "Gazebo failed to start"
        exit 1
    fi
}

# Spawn entities
spawn_entities() {
    log_info "Spawning truck..."
    
    # Spawn truck
    ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
        "{name: 'truck_001', xml: '$(cat "$SIM_ROOT/gazebo/models/truck/model.sdf" | sed "s/'/\\'/g")'}" \
        || log_warn "Truck spawn may have failed"
    
    sleep 2
    
    log_info "Spawning $NUM_DRONES drones..."
    
    # Spawn drones
    for i in $(seq 1 $NUM_DRONES); do
        local name=$(printf "drone_%03d" $i)
        local pose="10 $((i * 10)) 5 0 0 0"
        
        ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
            "{name: '$name', xml: '$(cat "$SIM_ROOT/gazebo/models/drone/model.sdf" | sed "s/'/\\'/g")', pose: {position: {x: 10, y: $((i * 10)), z: 5}}}"
        
        log_info "Spawned $name"
        sleep 1
    done
}

# Launch ROS2 nodes
launch_ros_nodes() {
    log_info "Launching ROS2 navigation nodes..."
    
    # Launch truck navigation
    if [ -f "$SIM_ROOT/../../../src/truck_navigation/launch/truck.launch.py" ]; then
        ros2 launch "$SIM_ROOT/../../../src/truck_navigation/launch/truck.launch.py" &
    fi
    
    # Launch drone navigation
    if [ -f "$SIM_ROOT/../../../src/drone_navigation/launch/drone.launch.py" ]; then
        ros2 launch "$SIM_ROOT/../../../src/drone_navigation/launch/drone.launch.py" &
    fi
    
    sleep 2
}

# Start data recording
start_recording() {
    log_info "Starting data recording..."
    
    # Create results directory
    RESULTS_DIR="$SIM_ROOT/results/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$RESULTS_DIR"
    
    # Record all topics
    ros2 bag record -a -o "$RESULTS_DIR/bag" &
    BAG_PID=$!
    
    log_info "Recording to: $RESULTS_DIR"
}

# Run simulation
run_simulation() {
    log_info "Running simulation for ${DURATION}s..."
    log_info "Press Ctrl+C to stop early"
    
    # Countdown
    for i in $(seq $DURATION -10 0); do
        if [ $((i % 30)) -eq 0 ]; then
            log_info "Time remaining: ${i}s"
        fi
        sleep 1
    done
    
    log_info "Simulation complete!"
}

# Collect results
collect_results() {
    log_info "Collecting results..."
    
    # Stop recording
    kill $BAG_PID 2>/dev/null || true
    sleep 2
    
    # Save current state
    ros2 topic list > "$RESULTS_DIR/topics.txt"
    
    log_info "Results saved to: $RESULTS_DIR"
}

# Main
main() {
    log_info "=== OpenClaw Simulation ==="
    log_info "World: $WORLD, Duration: ${DURATION}s, Drones: $NUM_DRONES"
    
    check_ros
    setup_paths
    cleanup
    
    launch_gazebo
    spawn_entities
    launch_ros_nodes
    start_recording
    
    run_simulation
    
    collect_results
    
    log_info "=== Simulation Complete ==="
    log_info "Results: $RESULTS_DIR"
}

trap cleanup EXIT

main "$@"