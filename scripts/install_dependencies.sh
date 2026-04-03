#!/bin/bash
# =============================================================================
# install_dependencies.sh - Installation des dépendances pour ProjetRamassagePoubelle
# =============================================================================

set -e

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Installation des dépendances ===${NC}"

# =============================================================================
# 1. Dépendances Système
# =============================================================================
echo -e "${GREEN}[1/5] Dépendances système...${NC}"

# Ubuntu 22.04 (Jammy)
if command -v apt-get &> /dev/null; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        wget \
        curl \
        libssl-dev \
        libboost-all-dev \
        libyaml-cpp-dev \
        protobuf-compiler \
        libprotobuf-dev \
        eigen3 \
        libgeographic-dev \
        python3-pip \
        python3-colcon \
        python3-rosdep \
        python3-vcstool \
        ros-jazzy-gazebo-ros-pkgs \
        ros-jazzy-gazebo-ros \
        ros-jazzy-xacro \
        ros-jazzy-robot-state-publisher \
        ros-jazzy-joint-state-publisher \
        ros-jazzy-gazebo-plugins \
        ros-jazzy-navigation2 \
        ros-jazzy-nav2-bringup \
        ros-jazzy-ros2bag \
        ros-jazzy-ros2cli
fi

# =============================================================================
# 2. ROS2 Jazzy (si non installé)
# =============================================================================
echo -e "${GREEN}[2/5] ROS2 Jazzy...${NC}"

if ! command -v ros2 &> /dev/null; then
    echo "Installation de ROS2 Jazzy..."
    
    # Configurer les sources
    sudo apt-get install -y software-properties-common
    sudo add-apt-repository -y ppa:robomit/ros2
    
    # Installer ROS2
    sudo apt-get update
    sudo apt-get install -y ros-jazzy-ros-base
    
    # Sourcing automatique
    if ! grep -q "source /opt/ros/jazzy/setup.bash" ~/.bashrc; then
        echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
    fi
    
    echo -e "${YELLOW}Veuillez exécuter: source ~/.bashrc${NC}"
fi

# =============================================================================
# 3. Gazebo (simulation)
# =============================================================================
echo -e "${GREEN}[3/5] Gazebo...${NC}"

if ! command -v gazebo &> /dev/null; then
    echo "Installation de Gazebo..."
    
    # Ajouter le dépôt osrf
    sudo wget https://packages.osrfoundation.org/gazebo.gpg -O /usr/share/keyrings/ppa-ros2-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ppa-ros2-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null
    
    sudo apt-get update
    sudo apt-get install -y gazebo
fi

# =============================================================================
# 4. Dépendances Python
# =============================================================================
echo -e "${GREEN}[4/5] Dépendances Python...${NC}"

pip3 install --break-system-packages \
    aiohttp \
    fastapi \
    uvicorn \
    sqlalchemy \
    pydantic \
    python-dotenv \
    pytest \
    pyyaml \
    rospkg

# =============================================================================
# 5. Initialisation workspace ROS2
# =============================================================================
echo -e "${GREEN}[5/5] Workspace ROS2...${NC}"

# Créer le workspace
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws

# Lien symbolique vers les packages du projet
if [ -d "/home/openclaw/.openclaw/workspace/drone_navigation" ]; then
    ln -sf /home/openclaw/.openclaw/workspace/drone_navigation src/
fi

if [ -d "/home/openclaw/.openclaw/workspace/truck_navigation" ]; then
    ln -sf /home/openclaw/.openclaw/workspace/truck_navigation src/
fi

# =============================================================================
# Résumé
# =============================================================================
echo ""
echo -e "${BLUE}=== Installation terminée ===${NC}"
echo ""
echo "Étapes suivantes:"
echo "  1. source ~/.bashrc"
echo "  2. cd ~/ros2_ws"
echo "  3. ./scripts/build.sh"
echo ""