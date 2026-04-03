#!/bin/bash
# =============================================================================
# OpenClaw - Script d'Installation CORRIGÉ (clé ROS expirée)
# =============================================================================

set -e

if [ "$EUID" -ne 0 ]; then
    echo "❌ sudo requis"
    exit 1
fi

echo "🔧 Correction clé ROS expirée..."

# Supprimer l'ancienne clé
sudo rm -f /usr/share/keyrings/ros-archive-keyring.gpg

# Télécharger la nouvelle clé (avec date d'expiration étendue)
sudo curl -k -sSL https://fixtures.ros.org/ros.key | sudo gpg --dearmor -o /usr/share/keyrings/ros-archive-keyring.gpg

# Ou utiliser une méthode alternative avec apt-key (deprecated mais fonctionne)
# sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F42ED6FBAB17C654

echo "✅ Clé ROS rafraîchie"

# Mettre à jour
sudo apt update

echo "✅ Prêt pour installation ROS2"
echo ""
echo "Exécutez maintenant:"
echo "sudo apt install -y ros-jazzy-ros2-cpp ros-jazzy-ros2-python ros-jazzy-gazebo-ros-pkgs python3-colcon-common-extensions"