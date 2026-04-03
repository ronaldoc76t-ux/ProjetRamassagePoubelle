#!/bin/bash
# =============================================================================
# run_system.sh - Lancement du système complet (truck + drones + backend)
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

# Setup
setup() {
    export ROS_DOMAIN_ID=42
    export GAZEBO_RESOURCE_PATH="$PROJECT_ROOT/playbook/impl/simulation/gazebo/worlds:$PROJECT_ROOT/playbook/impl/simulation/gazebo/models:$GAZEBO_RESOURCE_PATH"
    log_info "Configuration chargée"
}

# Lancer le backend API
launch_backend() {
    log_info "Lancement du backend API..."
    
    if [ -d "$PROJECT_ROOT/playbook/impl/src/backend" ]; then
        cd "$PROJECT_ROOT/playbook/impl/src/backend"
        
        # Créer venv si pas existant
        if [ ! -d "venv" ]; then
            python3 -m venv venv
        fi
        
        source venv/bin/activate
        pip install -r requirements.txt
        
        uvicorn app.main:app --host 0.0.0.0 --port 8000 &
        BACKEND_PID=$!
        
        log_info "Backend API démarré (PID: $BACKEND_PID)"
    else
        log_warn "Backend non trouvé, passage..."
    fi
}

# Lancer le truck
launch_truck() {
    log_info "Lancement du truck..."
    
    if [ -f "$PROJECT_ROOT/truck_navigation/launch/truck.launch.py" ]; then
        ros2 launch "$PROJECT_ROOT/truck_navigation/launch/truck.launch.py" &
        TRUCK_PID=$!
        
        log_info "Truck démarré (PID: $TRUCK_PID)"
    else
        log_warn "Truck launch non trouvé"
    fi
}

# Lancer les drones
launch_drones() {
    local num_drones=${1:-3}
    log_info "Lancement de $num_drones drones..."
    
    if [ -f "$PROJECT_ROOT/drone_navigation/launch/drone.launch.py" ]; then
        ros2 launch "$PROJECT_ROOT/drone_navigation/launch/drone.launch.py" num_drones:=$num_drones &
        DRONES_PID=$!
        
        log_info "Drones démarrés (PID: $DRONES_PID)"
    else
        log_warn "Drone launch non trouvé"
    fi
}

# Lancer l'orchestrateur
launch_orchestrator() {
    log_info "Lancement de l'orchestrateur..."
    
    if [ -d "$PROJECT_ROOT/playbook/impl/src/orchestrator" ]; then
        cd "$PROJECT_ROOT/playbook/impl/src/orchestrator"
        
        ros2 run orchestrator orchestrator_node &
        ORCH_PID=$!
        
        log_info "Orchestrateur démarré (PID: $ORCH_PID)"
    else
        log_warn "Orchestrateur non trouvé"
    fi
}

# Menu interactif
menu() {
    echo ""
    echo -e "${BLUE}=== Menu Système ===${NC}"
    echo "1. Lancer tout le système"
    echo "2. Lancer truck uniquement"
    echo "3. Lancer drones uniquement"
    echo "4. Lancer backend API"
    echo "5. Status des processus"
    echo "6. Arrêter tout"
    echo "q. Quitter"
    echo ""
    read -p "Choix: " choice
    
    case $choice in
        1)  setup
            launch_backend
            sleep 2
            launch_truck
            sleep 2
            launch_drones 3
            launch_orchestrator ;;
        2)  setup && launch_truck ;;
        3)  setup && launch_drones 3 ;;
        4)  launch_backend ;;
        5)  ps aux | grep -E "(ros2|uvicorn|orchestrator)" | grep -v grep ;;
        6)  pkill -f "ros2|uvicorn|orchestrator" && log_info "Arrêté" ;;
        q)  exit 0 ;;
        *)  log_error "Choix invalide" ;;
    esac
}

# =============================================================================
# MAIN
# =============================================================================
main() {
    log_info "=== Système Complet OpenClaw ==="
    
    check_ros
    
    if [ "$1" == "--interactive" ] || [ "$1" == "-i" ]; then
        while true; do menu; done
    elif [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --interactive, -i    Mode interactif"
        echo "  --help, -h           Cette aide"
        echo "  --truck              Truck seulement"
        echo "  --drones [N]        N drones (défaut: 3)"
        echo "  --backend            Backend API seulement"
    elif [ "$1" == "--truck" ]; then
        setup && launch_truck
    elif [ "$1" == "--drones" ]; then
        setup && launch_drones ${2:-3}
    elif [ "$1" == "--backend" ]; then
        launch_backend
    else
        # Mode automatique par défaut
        setup
        launch_backend
        sleep 2
        launch_truck
        sleep 2
        launch_drones 3
        launch_orchestrator
        
        log_info "Système démarré! Ctrl+C pour arrêter"
        wait
    fi
}

main "$@"