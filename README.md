# ProjetRamassagePoubelle 🚛🚁

Système de collecte de déchets autonome composé d'un camion-benne roulant en permanence et d'une flotte de drones collecteurs volants.

**Différenciateur**: Le camion ne s'arrête jamais — les drones doivent effectuer des rendez-vous dynamiques avec un véhicule en mouvement.

---

## 📋 Prérequis

### Logiciels requis

| Logiciel | Version | Installation |
|----------|---------|--------------|
| Ubuntu | 22.04 (Jammy) | [Ubuntu ISO](https://releases.ubuntu.com/22.04/) |
| ROS2 | Jazzy | [ROS2 Install](https://docs.ros.org/en/jazzy/Installation.html) |
| Gazebo | Simulate (Harmonic) | [Gazebo Install](https://gazebosim.org/docs/harmonic/get_started) |
| Python | 3.10+ | Inclus dans Ubuntu |
| colcon | latest | `pip install colcon-common-extensions` |

### Ressources minimales

- **CPU**: 4 cœurs
- **RAM**: 8 Go
- **Disque**: 20 Go свободного места
- **GPU**: OpenGL 3.0+ (pour Gazebo)

---

## 🛠️ Installation

### 1. Cloner le projet

```bash
cd ~
git clone https://github.com/ronaldoc76t-ux/ProjetRamassagePoubelle.git
cd ProjetRamassagePoubelle
```

### 2. Installer les dépendances

```bash
chmod +x scripts/install_dependencies.sh
./scripts/install_dependencies.sh
```

**OU installation manuelle:**

```bash
# ROS2 Jazzy
sudo apt-get update
sudo apt-get install -y ros-jazzy-ros-base

# Gazebo
sudo apt-get install -y gazebo

# Dépendances Python
pip3 install aiohttp fastapi uvicorn sqlalchemy pydantic pytest pyyaml

# Outils ROS2
sudo apt-get install -y python3-colcon python3-rosdep python3-vcstool
```

### 3. Configurer l'environnement

```bash
# Ajouter au ~/.bashrc (une fois)
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
echo "source ~/ros2_ws/install/setup.bash" >> ~/.bashrc

# Appliquer pour cette session
source ~/.bashrc
```

---

## 🔨 Construction

### Compiler les packages ROS2

```bash
# Mode automatique
chmod +x scripts/build.sh
./scripts/build.sh

# OU mode manuel
cd ~/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --cmake-args "-DCMAKE_BUILD_TYPE=Release"
```

### Vérifier la compilation

```bash
source ~/ros2_ws/install/setup.bash
ros2 pkg list | grep -E "(truck|drone)"
```

---

## ▶️ Exécution

### Option 1: Simulation Gazebo

```bash
chmod +x scripts/run_simulation.sh
./scripts/run_simulation.sh [monde] [durée] [nb_drones]

# Exemples:
./scripts/run_simulation.sh openclaw_world 300 3
./scripts/run_simulation.sh                 # paramètres par défaut
```

### Option 2: Système complet (réel + simulation)

```bash
chmod +x scripts/run_system.sh
./scripts/run_system.sh

# Mode interactif
./scripts.run_system.sh --interactive
```

### Option 3: Lancement partiel

```bash
# Truck seulement
./scripts/run_system.sh --truck

# Drones seulement (3 drones)
./scripts/run_system.sh --drones

# Backend API seulement
./scripts/run_system.sh --backend

# Backend + truck
./scripts/run_system.sh --backend && ./scripts/run_system.sh --truck
```

---

## 📁 Structure du projet

```
ProjetRamassagePoubelle/
├── PROJECT.md                 # État du projet
├── IMPLEMENTATION.md          # Détails d'implémentation
├── README.md                  # Ce fichier
├── playbook/                  # Documentation technique
│   ├── 01-prompt-maitre.md
│   ├── 02-architecture-haut-niveau.md
│   ├── 03-architecture-fonctionnelle.md
│   ├── 04-architecture-technologique.md
│   ├── 05-camion.md
│   ├── 06-drone.md
│   ├── 07-backend.md
│   ├── 08-application-mobile.md
│   ├── 09-coordination-multi-agents.md
│   ├── 10-simulation-plan.md
│   ├── 11-roadmap-12-semaines.md
│   └── impl/                  # Implémentations
│       ├── simulation/        # Simulation Gazebo
│       │   ├── gazebo/worlds/
│       │   ├── gazebo/models/
│       │   ├── scripts/
│       │   └── config/
│       └── src/
│           ├── truck_navigation/   # ROS2 package truck
│           └── drone_navigation/    # ROS2 package drone
├── drone_navigation/          # Package ROS2 drones
│   ├── src/
│   ├── config/
│   ├── launch/
│   └── msg/
├── truck_navigation/          # Package ROS2 truck
│   ├── src/
│   ├── config/
│   ├── launch/
│   └── msg/
└── scripts/                   # Scripts utilitaires
    ├── install_dependencies.sh
    ├── build.sh
    ├── run_simulation.sh
    └── run_system.sh
```

---

## 🎯 Métriques et Tests

### Tests unitaires

```bash
cd ~/ros2_ws
source install/setup.bash
colcon test --packages-select truck_navigation
colcon test --packages-select drone_navigation
```

### Analyse post-simulation

```bash
python3 playbook/impl/simulation/scripts/analyze_results.py results/
```

### Métriques cibles

| Métrique | Cible |
|----------|-------|
| Taux de docking | ≥ 90% |
| Précision temporelle | < 500ms |
| Batterie restante | ≥ 25% |
| Temps de failover | < 30s |
| Latence communication | < 100ms |

---

## 🔧 Dépannage

### ROS2 ne trouve pas les packages

```bash
# Vérifier l'environnement
echo $ROS_DISTRO
ros2 pkg list | wc -l

# Sourcing correct
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash
```

### Gazebo ne démarre pas

```bash
# Vérifier les dépendances
gazebo --version

# Réinstaller si nécessaire
sudo apt-get install --reinstall gazebo
```

### Erreur de compilation

```bash
# Nettoyer et reconstruire
cd ~/ros2_ws
rm -rf build/ install/ log/
colcon build --cmake-args "-DCMAKE_BUILD_TYPE=Release"
```

### Ports utilisés

```bash
# Backend API utilise le port 8000
# Vérifier: lsof -i :8000

# ROS2 utilise les ports 7400-7500 (multicast)
# Vérifier: netstat -g | grep 239.255.0.1
```

---

## 📞 Support

- **Issues GitHub**: https://github.com/ronaldoc76t-ux/ProjetRamassagePoubelle/issues
- **Documentation**: Voir le dossier `playbook/`

---

## 📜 License

MIT License - Voir [LICENSE](LICENSE)

---

*Projet développé avec OpenClaw* 🤖