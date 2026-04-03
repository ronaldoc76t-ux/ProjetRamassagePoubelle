# OpenClaw - Script d'Installation

## Usage

### Installation complète (tous les modules)
```bash
sudo ./install_openclaw.sh --all
```

### Installation par module
```bash
# ROS2 uniquement
sudo ./install_openclaw.sh --ros2

# Backend uniquement  
sudo ./install_openclaw.sh --backend

# Mobile uniquement
sudo ./install_openclaw.sh --mobile
```

### Aide
```bash
./install_openclaw.sh --help
```

---

## Ce que le script installe

### ROS2
- ROS2 Jazzy (Ubuntu 24.04) ou Humble (Ubuntu 22.04)
- Gazebo
- Navigation2
- colcon

### Backend
- Go 1.21.6
- PostgreSQL 15
- Redis 7

### Mobile
- Flutter stable
- Dépendances Linux pour Flutter

---

## Post-Installation

Après exécution, reloadz votre shell:
```bash
source ~/.bashrc
```

Puis vérifiez l'installation:
```bash
# ROS2
ros2 doctor

# Go
go version

# Flutter
flutter doctor
```

---

## Compilation du projet

Voir `PREREQUIS_COMPILATION.md` pour les commandes de build.