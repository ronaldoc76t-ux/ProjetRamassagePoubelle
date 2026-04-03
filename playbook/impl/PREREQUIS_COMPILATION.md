# 🔧 Prérequis Compilation - Projet OpenClaw

## Date: 2026-04-01
## Demandé par: PM / Chef de Projet

---

## Contexte

L'équipe doit préparer l'environnement pour compiler l'intégralité du code développé en Phase 2 (S3-S12).

---

## 1. Prérequis Système

### Commun (Tous modules)
- **OS**: Ubuntu 22.04 LTS ou macOS 13+
- **Git**: >= 2.34
- **Make**: >= 4.3

### ROS2 (Truck + Drone)
| Logiciel | Version | Notes |
|----------|---------|-------|
| ROS2 | Humble ou Jazzy | Installation via packages.ros.org |
| colcon | Latest | `sudo apt install python3-colcon-common-extensions` |
| Gazebo | Garden ou Harmonic | Pour simulation |
| rclgo | Latest | Bindings Go pour ROS2 |

### Backend (API)
| Logiciel | Version | Notes |
|----------|---------|-------|
| Go | 1.21+ | `go install` |
| PostgreSQL | 15+ | Docker ou installation native |
| Redis | 7+ | `sudo apt install redis-server` |
| Kafka | 3.5+ | Via Docker Compose |

### Mobile
| Logiciel | Version | Notes |
|----------|---------|-------|
| Flutter | 3.x | `flutter doctor` |
| Dart | 3.x | Inclus avec Flutter |
| Android SDK | API 34+ | Pour build Android |
| Xcode | 15+ | Pour build iOS (macOS only) |

---

## 2. Installation Rapide

### ROS2 (Ubuntu)
```bash
# Ajouter ROS2 repos
sudo apt update && sudo apt install -y ros-humble-ros2-cpp ros-humble-ros2-python ros-humble-gazebo-ros-pkgs

# Variables d'environnement
source /opt/ros/humble/setup.bash
```

### Backend
```bash
# Go
wget https://go.dev/dl/go1.21.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.21.linux-amd64.tar.gz
export PATH=$PATH:/usr/local/go/bin

# PostgreSQL + Redis
sudo apt install postgresql redis-server

# Kafka (Docker)
docker-compose up -d kafka
```

### Mobile
```bash
# Flutter
git clone https://github.com/flutter/flutter.git -b stable --depth 1
export PATH="$PATH:`pwd`/flutter/bin"

# Vérification
flutter doctor
```

---

## 3. Compilation des Modules

### Module 1: Truck Navigation ROS2
```bash
cd src/truck_navigation
colcon build --packages-select truck_navigation
source install/setup.bash
ros2 launch truck_navigation truck.launch.py
```

### Module 2: Drone Navigation ROS2
```bash
cd src/drone_navigation
colcon build --packages-select drone_navigation
source install/setup.bash
ros2 launch drone_navigation drone.launch.py
```

### Module 3: Backend API
```bash
cd backend
go mod download
go build -o bin/api-gateway ./cmd/api-gateway
go build -o bin/mission-service ./cmd/mission-service
docker-compose up -d
```

### Module 4: Orchestrateur
```bash
cd orchestrateur
go mod download
go build -o bin/orchestrator ./main.go
```

### Module 5: Mobile
```bash
cd mobile
flutter pub get
flutter build apk --debug  # Android
flutter build ios         # iOS (macOS only)
```

---

## 4. Vérification Installation

```bash
# ROS2
ros2 doctor

# Go
go version

# Flutter
flutter doctor -v

# PostgreSQL
psql --version

# Redis
redis-cli ping
```

---

## 5. Setup Environment Variables

```bash
# ~/.bashrc ou ~/.zshrc

# ROS2
source /opt/ros/humble/setup.bash
export GAZEBO_RESOURCE_PATH=/path/to/simulation/gazebo/models

# Backend
export DATABASE_URL="postgres://openclaw:password@localhost:5432/openclaw"
export REDIS_URL="redis://localhost:6379"
export JWT_SECRET="your-secret-key"

# Mobile (Android)
export ANDROID_HOME="$HOME/Android/Sdk"
export PATH="$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools"
```

---

## 6. Checklist Pré-compilation

| Étape | Responsable | Status |
|-------|-------------|--------|
| Installer ROS2 Humble | Équipe Robotique | ⬜ |
| Installer Go 1.21+ | Équipe Backend | ⬜ |
| Installer Flutter 3.x | Équipe Mobile | ⬜ |
| Configurer PostgreSQL | Équipe Backend | ⬜ |
| Configurer Docker | Équipe Infra | ⬜ |
| Vérifier `ros2 doctor` | Équipe Robotique | ⬜ |
| Tester compilation truck | Équipe Robotique | ⬜ |
| Tester compilation drone | Équipe Robotique | ⬜ |
| Tester build backend | Équipe Backend | ⬜ |
| Tester build mobile | Équipe Mobile | ⬜ |

---

## 7. Contact Support

| Problème | Contacter |
|----------|-----------|
| ROS2/Gazebo | Équipe Robotique |
| Backend API | Équipe Backend |
| Mobile/Flutter | Équipe Mobile |
| Docker/Infra | Équipe Infra |

---

*Document demandé par le PM le 2026-04-01*
*Deadline: Préparation pour session de build le 2026-04-02*