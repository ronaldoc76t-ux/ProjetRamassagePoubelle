#!/bin/bash
# =============================================================================
# OpenClaw - Script d'Installation Automatisée
# =============================================================================
# Usage: ./install_openclaw.sh [--ros2] [--backend] [--mobile] [--all]
# 
# Ce script installe les dépendances nécessaires pour compiler le projet OpenClaw.
# Exécuter avec sudo: sudo ./install_openclaw.sh --all
# =============================================================================

set -e

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Vérification sudo
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}❌ Ce script nécessite sudo. Relancez avec: sudo $0 $@${NC}"
    exit 1
fi

# Arguments
INSTALL_ROS2=false
INSTALL_BACKEND=false
INSTALL_MOBILE=false
INSTALL_ALL=false

for arg in "$@"; do
    case $arg in
        --ros2)    INSTALL_ROS2=true ;;
        --backend) INSTALL_BACKEND=true ;;
        --mobile)  INSTALL_MOBILE=true ;;
        --all)     INSTALL_ALL=true ;;
        -h|--help) 
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --ros2    Installer ROS2 (Truck + Drone)"
            echo "  --backend Installer Backend (Go, PostgreSQL, Redis)"
            echo "  --mobile  Installer Mobile (Flutter)"
            echo "  --all     Installer tout"
            echo "  -h, --help Afficher cette aide"
            exit 0 ;;
    esac
done

# Par défaut, tout installer si --all
if [ "$INSTALL_ALL" = true ]; then
    INSTALL_ROS2=true
    INSTALL_BACKEND=true
    INSTALL_MOBILE=true
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  OpenClaw - Installation Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# =============================================================================
# FONCTIONS UTILITAIRES
# =============================================================================

log_info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
log_success() { echo -e "${GREEN}✅ $1${NC}"; }
log_warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
log_error() { echo -e "${RED}❌ $1${NC}"; }

check_command() {
    if command -v "$1" &> /dev/null; then
        log_success "$1 installé: $(command -v $1)"
        return 0
    else
        log_warning "$1 non trouvé"
        return 1
    fi
}

# =============================================================================
# ROS2 INSTALLATION
# =============================================================================

install_ros2() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Installation ROS2${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Détecter la version d'Ubuntu
    . /etc/os-release
    UBUNTU_VERSION=$VERSION_ID
    
    log_info "Ubuntu $UBUNTU_VERSION détecté"
    
    # Ajouter le repository ROS2
    log_info "Ajout du repository ROS2..."
    
    apt update
    apt install -y curl gnupg lsb-release
    
    # ROS2 Jazzy pour Ubuntu 24.04, Humble pour 22.04
    if [[ "$UBUNTU_VERSION" == "24.04" ]]; then
        ROS2_DISTRO="jazzy"
    else
        ROS2_DISTRO="humble"
    fi
    
    log_info "Distribution ROS2: $ROS2_DISTRO"
    
    # Ajouter la clé ROS
    curl -k -sSL https://packages.ros.org/ros.key | gpg --dearmor -o /usr/share/keyrings/ros-archive-keyring.gpg
    
    # Ajouter le repository
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu ${UBUNTU_CODENAME} main" | tee /etc/apt/sources.list.d/ros2.list > /dev/null
    
    apt update
    
    # Installer ROS2
    log_info "Installation de ROS2 $ROS2_DISTRO..."
    apt install -y ros-${ROS2_DISTRO}-ros2-cpp \
                    ros-${ROS2_DISTRO}-ros2-python \
                    ros-${ROS2_DISTRO}-gazebo-ros-pkgs \
                    python3-colcon-common-extensions \
                    ros-${ROS2_DISTRO}-navigation2 \
                    ros-${ROS2_DISTRO}-nav2-msgs
    
    # Sourcing automatique
    if ! grep -q "ros2" ~/.bashrc 2>/dev/null; then
        echo "" >> ~/.bashrc
        echo "# ROS2" >> ~/.bashrc
        echo "source /opt/ros/${ROS2_DISTRO}/setup.bash" >> ~/.bashrc
    fi
    
    log_success "ROS2 $ROS2_DISTRO installé"
    
    # Tester ROS2
    if [ -f "/opt/ros/${ROS2_DISTRO}/setup.bash" ]; then
        source /opt/ros/${ROS2_DISTRO}/setup.bash
        log_info "Version ROS2: $(ros2 --version 2>/dev/null || echo 'Non vérifiable')"
    fi
}

# =============================================================================
# BACKEND INSTALLATION
# =============================================================================

install_backend() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Installation Backend (Go + DB)${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Go
    log_info "Installation de Go..."
    if ! check_command go; then
        wget -q https://go.dev/dl/go1.21.6.linux-amd64.tar.gz -O /tmp/go.tar.gz
        tar -C /usr/local -xzf /tmp/go.tar.gz
        rm /tmp/go.tar.gz
        
        # Ajouter au PATH
        if ! grep -q '/usr/local/go/bin' ~/.bashrc 2>/dev/null; then
            echo 'export PATH=$PATH:/usr/local/go/bin' >> ~/.bashrc
        fi
        export PATH=$PATH:/usr/local/go/bin
    fi
    log_success "Go $(go version 2>/dev/null | cut -d' ' -f3) installé"
    
    # PostgreSQL
    log_info "Installation de PostgreSQL..."
    if ! check_command psql; then
        apt install -y postgresql postgresql-contrib
        systemctl enable postgresql
        systemctl start postgresql
        
        # Créer utilisateur et base
        su - postgres -c "psql -c \"CREATE USER openclaw WITH PASSWORD 'password';\"" 2>/dev/null || true
        su - postgres -c "psql -c \"CREATE DATABASE openclaw OWNER openclaw;\"" 2>/dev/null || true
    fi
    log_success "PostgreSQL installé"
    
    # Redis
    log_info "Installation de Redis..."
    if ! check_command redis-server; then
        apt install -y redis-server
        systemctl enable redis-server
        systemctl start redis-server
    fi
    log_success "Redis installé"
    
    # Docker (optionnel, pour Kafka)
    log_info "Vérification Docker..."
    if ! check_command docker; then
        log_warning "Docker non installé - Kafka nécessite Docker"
        log_info "Pour installer Docker: voir https://docs.docker.com/engine/install/"
    fi
    
    # Kafka (via Docker Compose)
    log_info "Vérification Docker Compose..."
    if ! check_command docker-compose && ! docker compose version &>/dev/null; then
        log_warning "Docker Compose non installé"
    fi
}

# =============================================================================
# MOBILE INSTALLATION
# =============================================================================

install_mobile() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Installation Mobile (Flutter)${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Flutter
    log_info "Installation de Flutter..."
    if ! check_command flutter; then
        apt install -y git unzip xz-utils zip libglu1-mesa clang cmake ninja-build pkg-config libgtk-3-dev
        
        FLUTTER_DIR="/opt/flutter"
        if [ ! -d "$FLUTTER_DIR" ]; then
            git clone https://github.com/flutter/flutter.git -b stable --depth 1 "$FLUTTER_DIR"
        fi
        
        # Ajouter au PATH
        if ! grep -q 'flutter/bin' ~/.bashrc 2>/dev/null; then
            echo 'export PATH="$PATH:/opt/flutter/bin"' >> ~/.bashrc
        fi
        export PATH="$PATH:$FLUTTER_DIR/bin"
        
        # Précharger Flutter
        flutter precache --android --ios --linux 2>/dev/null || true
    fi
    
    FLUTTER_VERSION=$(flutter --version 2>/dev/null | head -n1 || echo "Non vérifiable")
    log_success "Flutter installé: $FLUTTER_VERSION"
    
    # Android SDK (optionnel)
    log_info "Vérification Android SDK..."
    if [ -z "$ANDROID_HOME" ] && [ -z "$ANDROID_SDK_ROOT" ]; then
        log_warning "Android SDK non configuré"
        log_info "Configurer: export ANDROID_HOME=\$HOME/Android/Sdk"
    else
        log_success "Android SDK configuré"
    fi
    
    # Dart
    if check_command dart; then
        log_success "Dart $(dart --version 2>/dev/null | head -n1) installé"
    fi
}

# =============================================================================
# VÉRIFICATIONS FINALES
# =============================================================================

verify_installation() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Vérification Installation${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    echo ""
    echo -e "${BLUE}📦 Résumé:${NC}"
    echo ""
    
    # ROS2
    if [ "$INSTALL_ROS2" = true ]; then
        if [ -f "/opt/ros/${ROS2_DISTRO:-humble}/setup.bash" ]; then
            source /opt/ros/${ROS2_DISTRO:-humble}/setup.bash
            ros2 --version &>/dev/null && echo -e "  ROS2:     ${GREEN}✅ Installé${NC}" || echo -e "  ROS2:     ${RED}❌ Erreur${NC}"
        else
            echo -e "  ROS2:     ${RED}❌ Non installé${NC}"
        fi
    fi
    
    # Backend
    if [ "$INSTALL_BACKEND" = true ]; then
        go version &>/dev/null && echo -e "  Go:       ${GREEN}✅ $(go version | cut -d' ' -f3)${NC}" || echo -e "  Go:       ${RED}❌${NC}"
        psql --version &>/dev/null && echo -e "  PostgreSQL: ${GREEN}✅ Installé${NC}" || echo -e "  PostgreSQL: ${RED}❌${NC}"
        redis-server --version &>/dev/null && echo -e "  Redis:    ${GREEN}✅ Installé${NC}" || echo -e "  Redis:    ${RED}❌${NC}"
    fi
    
    # Mobile
    if [ "$INSTALL_MOBILE" = true ]; then
        flutter --version &>/dev/null && echo -e "  Flutter:  ${GREEN}✅ Installé${NC}" || echo -e "  Flutter:  ${RED}❌${NC}"
    fi
    
    echo ""
    echo -e "${YELLOW}⚠️  Reload votre shell pour appliquer les changements:${NC}"
    echo "   source ~/.bashrc"
    echo ""
}

# =============================================================================
# MAIN
# =============================================================================

log_info "Début de l'installation..."

if [ "$INSTALL_ROS2" = true ]; then
    install_ros2
fi

if [ "$INSTALL_BACKEND" = true ]; then
    install_backend
fi

if [ "$INSTALL_MOBILE" = true ]; then
    install_mobile
fi

if [ "$INSTALL_ROS2" = false ] && [ "$INSTALL_BACKEND" = false ] && [ "$INSTALL_MOBILE" = false ]; then
    log_error "Aucune option sélectionnée. Utilisez --help pour les options."
    exit 1
fi

verify_installation

echo -e "${GREEN}🎉 Installation terminée!${NC}"
echo ""
echo "Prochaines étapes:"
echo "  1. Reload shell: source ~/.bashrc"
echo "  2. Tester ROS2: ros2 doctor"
echo "  3. Tester Backend: go version"
echo "  4. Tester Mobile: flutter doctor"