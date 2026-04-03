#!/bin/bash
# =============================================================================
# run_simulation.sh - Lancement de la simulation Gazebo
# =============================================================================

set -e

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Répertoire du projet
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SIM_ROOT="$PROJECT_ROOT/playbook/impl/simulation"

# Paramètres
WORLD=${1:-openclaw_world}
DURATION=${2:-300}
NUM_DRONES=${3:-3}

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Vérifier ROS2
check_ros() {
    if ! command -v ros2 &> /dev/null; then
        log_error "ROS2 non trouvé. Exécutez install_dependencies.sh"
        exit 1
    fi
    log_info "ROS2: $(ros2 --version | head -1)"
}

# Configurer les paths
setup_paths() {
    export GAZEBO_RESOURCE_PATH="$SIM_ROOT/gazebo/worlds:$SIM_ROOT/gazebo/models:$GAZEBO_RESOURCE_PATH"
    export GAZEBO_PLUGIN_PATH="$SIM_ROOT/gazebo/plugins:$GAZEBO_PLUGIN_PATH"
    export ROS_DOMAIN_ID=42
    log_info "Chemins configurés"
}

# Nettoyer les processus existants
cleanup() {
    log_info "Nettoyage..."
    pkill -f gazebo 2>/dev/null || true
    pkill -f gzserver 2>/dev/null || true
    sleep 1
}

# Lancer Gazebo
launch_gazebo() {
    log_info "Lancement Gazebo (monde: $WORLD)..."
    
    ros2 launch gazebo_ros gazebo.launch.py \
        world_name:="$SIM_ROOT/gazebo/worlds/${WORLD}.sdf" \
        paused:=false \
        use_sim_time:=true \
        gui:=true &
    
    sleep 5
    
    if pgrep -x "gzserver" > /dev/null; then
        log_info "Gazebo démarré"
    else
        log_error "Échec du démarrage de Gazebo"
        exit 1
    fi
}

# Spawn les entités
spawn_entities() {
    log_info "Spawn du truck..."
    
    ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
        "{name: 'truck_001', xml: '$(cat "$SIM_ROOT/gazebo/models/truck/model.sdf" | sed "s/'/\\'/g")'}" \
        || log_warn "Échec possible du spawn truck"
    
    sleep 2
    
    log_info "Spawn $NUM_DRONES drones..."
    for i in $(seq 1 $NUM_DRONES); do
        name=$(printf "drone_%03d" $i)
        
        ros2 service call /gazebo/spawn_sdf_entity gazebo_ros/srv/SpawnEntity \
            "{name: '$name', xml: '$(cat "$SIM_ROOT/gazebo/models/drone/model.sdf" | sed "s/'/\\'/g")', pose: {position: {x: 10, y: $((i * 10)), z: 5}}}"
        
        log_info "  Spawn $name"
        sleep 1
    done
}

# Lancer les nœuds ROS2
launch_ros_nodes() {
    log_info "Lancement des nœuds ROS2..."
    
    # Truck navigation
    if [ -f "$PROJECT_ROOT/truck_navigation/launch/truck.launch.py" ]; then
        ros2 launch "$PROJECT_ROOT/truck_navigation/launch/truck.launch.py" &
    fi
    
    # Drone navigation
    if [ -f "$PROJECT_ROOT/drone_navigation/launch/drone.launch.py" ]; then
        ros2 launch "$PROJECT_ROOT/drone_navigation/launch/drone.launch.py" &
    fi
    
    sleep 2
}

# Enregistrer les données
start_recording() {
    log_info "Enregistrement..."
    RESULTS_DIR="$SIM_ROOT/results/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$RESULTS_DIR"
    
    ros2 bag record -a -o "$RESULTS_DIR/bag" &
    BAG_PID=$!
    
    log_info "Enregistré dans: $RESULTS_DIR"
}

# Simulation principale
run_simulation() {
    log_info "Simulation ${DURATION}s - Ctrl+C pour arrêter"
    
    for i in $(seq $DURATION -10 0); do
        [ $((i % 30)) -eq 0 ] && log_info "Temps restant: ${i}s"
        sleep 1
    done
    
    log_info "Simulation terminée!"
}

# Collecter les résultats
collect_results() {
    log_info "Collecte des résultats..."
    kill $BAG_PID 2>/dev/null || true
    ros2 topic list > "$RESULTS_DIR/topics.txt"
    log_info "Résultats: $RESULTS_DIR"
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    log_info "=== Simulation OpenClaw ==="
    log_info "Monde: $WORLD, Durée: ${DURATION}s, Drones: $NUM_DRONES"
    
    check_ros
    setup_paths
    cleanup
    
    launch_gazebo
    spawn_entities
    launch_ros_nodes
    start_recording
    
    run_simulation
    
    collect_results
    
    log_info "=== Terminé ==="
}

trap cleanup EXIT

main "$@"