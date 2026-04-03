#!/bin/bash
# =============================================================================
# build.sh - Compilation des packages ROS2
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

echo -e "${BLUE}=== Compilation des packages ROS2 ===${NC}"

# Vérifier ROS2
if ! command -v ros2 &> /dev/null; then
    echo -e "${RED}Erreur: ROS2 n'est pas installé${NC}"
    echo "Exécutez d'abord: ./scripts/install_dependencies.sh"
    exit 1
fi

# Vérifier colcon
if ! command -v colcon &> /dev/null; then
    echo -e "${RED}Erreur: colcon n'est pas installé${NC}"
    exit 1
fi

# Workspace ROS2
ROS2_WS="${ROS2_WS:-$HOME/ros2_ws}"
mkdir -p "$ROS2_WS/src"

echo -e "${GREEN}[1/3] Préparation du workspace...${NC}"

# Copier les packages si pas de liens symboliques
for pkg in drone_navigation truck_navigation; do
    if [ -d "$PROJECT_ROOT/$pkg" ] && [ ! -L "$ROS2_WS/src/$pkg" ]; then
        echo "  Copie $pkg..."
        rm -rf "$ROS2_WS/src/$pkg"
        cp -r "$PROJECT_ROOT/$pkg" "$ROS2_WS/src/"
    fi
done

echo -e "${GREEN}[2/3] rosdep install...${NC}"
cd "$ROS2_WS"
rosdep install --from-paths src --ignore-src -r -y || true

echo -e "${GREEN}[3/3] Compilation avec colcon...${NC}"
colcon build --cmake-args "-DCMAKE_BUILD_TYPE=Release"

# =============================================================================
# Résumé
# =============================================================================
echo ""
echo -e "${BLUE}=== Compilation terminée ===${NC}"
echo ""
echo "Pour utiliser les packages:"
echo "  source $ROS2_WS/install/setup.bash"
echo ""
echo "Pour lancer la simulation:"
echo "  ./scripts/run_simulation.sh"
echo ""
echo "Pour lancer le système complet:"
echo "  ./scripts/run_system.sh"