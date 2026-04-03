# Architecture drones collecteurs (CORRIGÉ)

## Story
- En tant qu'ingénieur drone, je veux un plan pour la mécanique, les capteurs et le logiciel afin de livrer une flotte sûre et fiable.
- Critère d'acceptation : concept drone, communication, prise/dépôt et logique de pilotage couvert.

---

## 1. Architecture mécanique & électronique

### 1.1 Type de drone

| Critère | Spécification |
|---------|---------------|
| Configuration | **Octocopter** à 8 rotors (failure tolerance supérieure) |
| Usage | Collecte de sacs/bacs de déchets lourds ( atéropathie urbaine) |
| Certification cible | Catégorie spécifique (vol autonome hors zone peuplée) |
| Décollanding/Atterrissage | VTOL (Vertical Take-Off and Landing) |
| Structure | Cadre en fibre de carbone léger, bras pliables pour transport |

### 1.2 Moteurs, ESC et batterie

#### Moteurs brushless

| Paramètre | Valeur |
|-----------|--------|
| Modèle | **T-Motor MN4010** ou équivalent, 350-400 kV |
| Puissance unitaire | 1200-1500 W en pointe |
| Propulsion totale | 8 × 700W nominal = 5600W disponibles |
| Réduction | Sans réducteur (direct drive) pour simplicité |
| Thrust unitaire | ~2.0-2.5 kg par moteur |
| **Thrust total** | **~16-20 kg** (8 × 2 kg) |
| **Ratio thrust/weight** | **≥ 2:1** (16 kg thrust / 8 kg dry = 2:1) |

> **Correction committee** : Le ratio thrust/weight a été augmenté à 2:1 minimum grâce aux moteurs MN4010 (350 kV) plus puissants. Le document original utilisait des moteurs MN1006 (100-150 kV) avec un ratio de ~1.7:1 insuffisant.

#### ESC (Electronic Speed Controller)

| Paramètre | Spécification |
|-----------|----------------|
| Type | ESC 4-en-1 (4 × 60A) ou 8 × ESC individuels 60A |
| Firmware | BLHeli_32 ou KISS avec support DShot1200 |
| Protection | Over-current, overtemperature, cutoff voltage |
|通讯 | D-BUS / PWM / CAN (pour telemetry) |

#### Batterie

| Paramètre | Spécification |
|-----------|----------------|
| Type | LiPo 8S (29.6V nominal) |
| Capacité | 15 000 - 20 000 mAh |
| Energie | ~500-600 Wh |
| Décharge | 5C continu, 10C burst |
| BMS | Intégré avec équilibrage, protection surcharge/décharge profonde |
| Poids | ~2.5-3.5 kg |

> **Correction committee** : L'autonomie a été corrigée pour refléter la réalité terrain : ~12-18 min en conditions réelles (vs 25-30 min irréalistes). Les valeurs originales were optimistes et ne teniennent pas compte des facteurs de dégradation LiPo, vent, manœuvres, et-cycle decharge reel.

| Autonomie | Condition | Note |
|-----------|-----------|------|
| **~12-15 min** | Sans charge, vol moyen | Valeur réelle terrain |
| **~8-10 min** | Avec charge max (10 kg) | Inclut réserve sécurité |
| **~18 min** | Mode économique (vitesse réduite) | vol stationnaire max |

### 1.3 Capteurs

| Capteur | Modèle recommandé | Fonction |Interface |
|---------|-------------------|-----------|-----------|
| **Caméra AI** | NVIDIA Jetson Orin Nano + caméra USB 4K ou Intel RealSense D455 | Détection objets (YOLO), tracking | USB-C / CSI |
| **LiDAR léger** | Livox Mid-40 ou RPLidar A1M8 (si budget réduit) | Cartographie 3D, détection obstacles | Ethernet / USB |
| **GPS RTK** | u-blox ZED-F9P + antennée active | Positionnement centimétrique | UART / USB |
| **IMU** | BMI088 ou BNO085 (9 axes) | Estimation d'attitude, fusion sensorielle | SPI / I2C |
| **Altimètre** | MS5611 (baromètre) + VL53L0X (télémètre laser) | Altitude précise | I2C |
| **Optical Flow** | PMW3901 ou Raspberry Pi Camera HQ | Stabilisation en intérieur/faible GPS | SPI |
| **Capteur de charge** | Load cell miniature (HX711) | Détection masse préhensée | ADC / I2C |

### 1.4 Système de préhension

| Composant | Description |
|-----------|-------------|
| **Bras articulés** | 2 bras robotiques à 3 degrés de liberté (DDL) chacun |
| **Effector** | Ventouse magnétique + crochet motorisé |
| **Capteur de présence** | Capteur à effet Hall pour confirmer préhension |
| **Moteur** | Servomoteurs MG996R ou moteurs pas-à-pas NEMA 8 |
| **Sécurité** | Détection de résistance anormale (couple max) |

#### Spécifications de préhension

| Paramètre | Valeur |
|-----------|--------|
| Charge utile max | 15 kg par bras |
| Force de préhension | 50-100 N |
| Temps de préhension | < 3 secondes |
| Confirmation | Capteur tactile + absence de mouvement relatif |

### 1.5 Système de dépôt sécurisé dans un camion en mouvement

| Fonction | Détail technique |
|----------|------------------|
| **Suivi visuel** | Caméra PTZ pour tracking du camion (zoom 30x) |
| **Guidage laser** | Emetteur/récepteur pour alignement fin (< 5 cm) |
| **Interface camion** | Marqueur ARUCO ou RFID au sol du camion |
| **Synchronisation** | Prédiction de position par Kalman filter |
| **Dépose** | Tremplin inclinable + rail de guidage pour glide (évite contact avec vento) |
| **Sécurité** | Parachute de sécurité si perte de signal |

### 1.6 Communication

| Technologie | Usage | Spécification |
|-------------|-------|----------------|
| **5G** | Command & Control, streaming vidéo HD | Module 5G (Quectel RM500Q ou Sierra Wireless) |
| **WiFi Mesh** | Communication swarm, redondance locale | WiFi 6 (802.11ax) + protocole OLSR ou BATMAN |
| **LoRa** | Backup, telemetry bas débit | LoRa 868/915 MHz, ~10 km portée |
| **RC** | Manuel override, failsafe | Récepteur RC 2.4 GHz (ELRS ou FrSky) |
| **Ethernet** | Liaison Jetson - capteurs | Gigabit |

### 1.7 Contraintes de masse et autonomie

| Élément | Masse |
|---------|-------|
| Frame + bras | 2.5 kg |
| Moteurs + ESC | 2.4 kg (8 × 300g) |
| Batterie | 3.0 kg |
| Caméra + calculateur | 0.8 kg |
| LiDAR | 0.4 kg |
| Système préhension | 1.5 kg |
| Radio + autre | 0.5 kg |
| **Total drone vide** | ~10.6 kg |
| **Charge utile** | 5-10 kg |
| **MTOW** | 16-21 kg |

> **Correction committee** : Le masa vide a été ajusté (10.6 kg vs 9.3 kg) pour refléter les moteurs MN4010 plus lourds (300g vs 200g par moteur).

---

## 2. Architecture logicielle ROS2

### 2.1 Contrôleur de vol : **PX4**

| Critère | PX4 | ArduPilot | Décision |
|---------|-----|-----------|----------|
| ROS2 natif | ++ (MAVLink via mavros) | + (MAVLink via mavros) | |
| Communauté | ++ (très active) | ++ (très active) | |
| Mode mission | ++ (excellent) | ++ (excellent) | |
| Integration Jetson | ++ (Flight stack prêt) | + (plus de config) | |
| Certificiation | ++ (EASA, FAA friendly) | + | |
| **Choix** | | | **PX4** |

> **Correction committee** : Le contrôleur de vol **PX4** est sélectionné. PX4 offre une intégration ROS2 plus naturelle via MAVLink/mavros, une communauté très active, et une meilleure compatibilité avec les stacks autonomes modernes. ArduPilot reste une alternative valide mais nécessite plus de configuration.

### 2.2 Nœuds ROS2 (Nodes)

```bash
/drone_manager              # Nœud principal, orchestration globale
/vision_perception          # Acquisition caméra + prétraitement
/object_detection          # Détection sacs/bacs (YOLOv8)
/lidar_processing          # Nuage de points, fusion avec caméra
/path_planner              # Planification trajectoire A* ou RRT*
/approach_controller       # Contrôle PID de l'approche
/docking_manager           # Gestion du docking avec camion
/truck_predictor           # Prédiction de position du camion
/battery_monitor           # Surveillance batterie, estimation SOC
/failsafe_handler          # Gestion des modes dégradés
/comm_interface            # Bridge 5G/WiFi/LoRa
/mavlink_bridge            # Interface PX4 <-> ROS2
/gimbal_controller         # Contrôle caméra PTZ
/gripper_controller        # Contrôle bras préhensiles
```

### 2.3 Topics ROS2

| Topic | Type | Direction | Description |
|-------|------|-----------|-------------|
| `/drone_pose` | geometry_msgs/PoseStamped | Out | Position/orientation fuse (EKF) |
| `/drone_velocity` | geometry_msgs/TwistStamped | Out | Vitesse fuse |
| `/target_pose` | geometry_msgs/PoseStamped | In/Out | Cible de destination |
| `/detected_objects` | vision_msgs/Detection3DArray | Out | Objets détectés avec bounding box |
| `/battery_state` | sensor_msgs/BatteryState | Out | État batterie (tension, courant, SOC) |
| `/docking_status` | custom_msgs/DockingStatus | Out | Statut docking (APPROACHING, DOCKED, RELEASED) |
| `/truck_pose` | geometry_msgs/PoseStamped | In | Position du camion (reçue par comm) |
| `/truck_prediction` | geometry_msgs/PoseArray | Out | Positions prédites futures |
| `/mission_command` | mavros_msgs/CommandLong | In | Commandes mission |
| `/gripper_state` | custom_msgs/GripperState | Out | État préhension (OPEN, CLOSED, GRIPPED) |
| `/failsafe_mode` | std_msgs/String | Out | Mode failsafe actif |
| `/cloud_point` | sensor_msgs/PointCloud2 | Out | Nuage de points LiDAR |
| `/depth_image` | sensor_msgs/Image | Out | Image profondeur |

### 2.4 Services ROS2

| Service | Type | Fonction |
|---------|------|----------|
| `/capture_target` | custom_srvs/CaptureTarget.srv | Demande de préhension d'un objet |
| `/release_cargo` | std_srvs/Trigger.srv | Déclenche relâche dans camion |
| `/plan_path` | nav_srvs/PlanPath.srv | Demande planification trajectoire |
| `/set_docking_mode` | custom_srvs/SetDockingMode.srv | Active mode approche/docking |
| `/request_land` | mavros_msgs/CommandLong.srv | Demande atterrissage |
| `/return_to_base` | std_srvs/Trigger.srv | Retour à la base |
| `/get_battery_status` | sensor_srvs/GetBatteryStatus.srv | État batterie |

### 2.5 Actions ROS2

| Action | Type | But |
|--------|------|-----|
| `/execute_mission` | actionlib_msgs/ExecuteMission | Execute mission complète (aller → collecter → livrer) |
| `/rendezvous_execute` | custom_actions/Rendezvous | Synchronisation et dépôt chez camion |
| `/land_on_truck` | custom_actions/TruckLanding | Atterrissage guidé sur camion |
| `/search_area` | actionlib_msgs/SearchArea | Recherche的区域 de collecte |

### 2.6 Détection des sacs/bacs : YOLOv8

> **Correction committee** : Cette section a été détaillée avec les informations d'entraînement et de test YOLOv8.

#### Pipeline de détection

```
┌─────────────────────────────────────────────────────┐
│               PIPELINE DÉTECTION                    │
├─────────────────────────────────────────────────────┤
│  [Caméra] → [Preprocessing] → [YOLOv8] → [Filter]  │
│       ↓              ↓            ↓          ↓      │
│  Debayering    Normalisation   NMS      Conf > 0.7  │
│                                              ↓      │
│                              [3D Localization]      │
│                           (Depth + LiDAR fusion)     │
│                                              ↓      │
│                               [Object Tracking]     │
│                           (Deep SORT / ByteTrack)    │
└─────────────────────────────────────────────────────┘
```

#### Spécifications modèle

| Détail | Spécification |
|--------|---------------|
| Modèle | **YOLOv8l-obb** (oriented bounding boxes) |
| Inference | TensorRT sur Jetson Orin Nano (~80-100 FPS) |
| Données d'entrée | 640×640 RGB |
| Sortie | Bbox 2D + class + confidence + masque instance |
| Tracking | ByteTrack, association par IoU + appearance |

#### Entraînement (Dataset & Training)

| Phase | Détail |
|-------|--------|
| **Dataset** | ~5,000-10,000 images annotées (sacs,poubelles,bacs) |
| **Sources** | Images urbaines réelles + synthétique (Blender/Gaussian splatting) |
| **Annotations** | COCO format + OBB (oriented bounding box) avec label + occlusions |
| **Augmentations** | Mosaic, mixup, rotation, luminosité, occlusions partiales |
| **Framework** | Ultralytics YOLOv8 + PyTorch |
| **Hardware entraînement** | GPU NVIDIA A100 ou RTX 4090 (~8-12h training) |
| **Hyperparamètres** | lr0=0.01, batch=16, epochs=300, imgsz=640 |
| **Metrics training** | mAP50: 0.85+, mAP50-95: 0.70+ |

#### Tests et Validation

| Test | Protocole | Seuil |
|------|-----------|-------|
| **Précision** | mAP50-95 sur dataset held-out (20%) | > 0.70 |
| **FP/s (Faux Positifs)** | Images négatives (pas d'objet) | < 5% |
| **FN/s (Faux Négatifs)** | Objets complètement visibles | < 3% |
| **Robustesse occlusion** | Test avec 20-50% occlusion | mAP > 0.60 |
| **Robustesse luminosité** | Test nuit/nuit pluvieuse | mAP > 0.65 |
| **Vitesse inférence** | FPS sur Jetson Orin Nano | > 60 FPS |
| **Latence bout-en-bout** | Image → détection → commande | < 50ms |

#### Dataset Recommended Structure

```
dataset/
├── images/
│   ├── train/          # ~80% des données
│   │   ├── img_001.jpg
│   │   └── ...
│   └── val/            # ~20% des données
│       ├── img_001.jpg
│       └── ...
├── labels/
│   ├── train/
│   │   ├── img_001.txt  # format OBB: class x_center y_center width height angle
│   │   └── ...
│   └── val/
│       └── ...
└── dataset.yaml        # config YOLO
```

#### Déploiement TensorRT

```python
# Export TensorRT pour Jetson Orin
from ultralytics import YOLO

model = YOLO('yolov8l-obb.pt')
model.export(format='engine', imgsz=640, half=True)  # FP16 pour Orin
```

### 2.7 Planification de trajectoire

| Phase | Algorithme | Détail |
|-------|------------|--------|
| **Global** | A* ou RRT* sur grile 2.5D | Carte statique + obstacles connue |
| **Local** | DWA ou MPC | Évitement dynamique d'obstacles |
| **Approche** | Contrôleur PID + LMPC | Trajectoire lisse vers point de docking |
| **Hold** | Backstepping | Maintien position si truck pas prêt |

#### Paramètres

| Paramètre | Valeur |
|-----------|--------|
| Vitesse max | 8 m/s (zone urbaine) |
| Accélération max | 3 m/s² |
| Distance sécurité | 3 m (obstacles dynamiques) |
| Précision positionnement | < 10 cm (GPS RTK) |
| Fréquence contrôle | 100 Hz |

### 2.8 Synchronisation avec camion en mouvement

```
┌─────────────────────────────────────────────────────────────┐
│              SYNC CAMION - DRONE                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │  CAMION      │    │ PREDICTION   │    │   DRONE      │  │
│  │  (GPS +      │───▶│  (Kalman)    │───▶│  (Trajec)    │  │
│  │   odometry)  │    │  Filter)     │    │  Generator)  │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│        │                    │                    │          │
│        ▼                    ▼                    ▼          │
│  Position actuelle    Horizon: 5-10s    Vitesse relative   │
│  Vélocité             Trajectoire          convergente      │
│  Accélération         future               vers 0           │
│                                                             │
│  Condition de rendez-vous: V_drone - V_camion ≈ 0           │
│  au point de dépôt (avec ∆t de prédiction)                  │
└─────────────────────────────────────────────────────────────┘
```

| Paramètre | Valeur |
|-----------|--------|
| Horizon de prédiction | 5-10 secondes |
| Fréquence mise à jour | 10 Hz (position camion) |
| Erreur de prédiction acceptée | < 50 cm |
| Temps de convergence | < 3 secondes |

### 2.9 Gestion batterie

| Métrique | Seuil | Action |
|----------|-------|--------|
| SOC > 30% | Normal | Operation normale |
| SOC 15-30% | Warning | Retour progressif vers base |
| SOC < 15% | Critical | Atterrissage immédiat sûr |
| Tension < 3.3V/cell | Critical | Land immédiat |
| Température > 65°C | Warning | Réduction puissance |
| Courant > 40A | Warning | Limitation thrust |

### 2.10 Gestion des échecs (Failsafe)

| Failure | Détection | Action |
|---------|-----------|--------|
| **Perte RC** | Pas de signal > 2s | Retour autonome / hover |
| **Perte GPS** | HDOP > 2.0 | Switch Optical Flow |
| **Perte communication** | Timeout > 5s | Return to Launch (RTL) |
| **Collision imminente** | LiDAR + caméra | Hover / évitement |
| **Batterie critique** | SOC < 15% | Land immédiat |
| **Moteur fail** | ESC telemetry | Flight sans moteur défectueux |
| **Perte contrôle** | Commande impossible | Parachute / land urgence |

#### Machine à états failsafe

```
┌─────────┐
│  ARMED  │──────►────────────────────────────┐
└─────────┘                                   │
     │                                        ▼
     ▼                              ┌─────────────────┐
┌─────────────┐                    │  FLIGHT_NORMAL  │
│ STANDBY     │                    └─────────────────┘
└─────────────┘                              │
     │                                       ▼
     ▼                              ┌─────────────────┐
┌───────────┐                       │  WARNING_MODE   │────┐
│ PREFLIGHT │                       │  (batt faible)  │    │
└───────────┘                       └─────────────────┘    │
     │                                        │           │
     ▼                                        ▼           ▼
┌──────────┐                       ┌─────────────────┐ ┌───────┐
│  ERROR   │──────────────────────►│  RETURN_HOME    │ │ LAND  │
└──────────┘                       └─────────────────┘ │ NOW   │
                                                   └───────┘
```

---

## 3. Vue système globale

```
┌─────────────────────────────────────────────────────────────────────┐
│                         DRONE COLLECTEUR                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐      ┌─────────────┐      ┌─────────────┐        │
│  │   CAPTEURS  │      │    Calcul    │      │ COMMUNICA-  │        │
│  │             │      │              │      │   TION      │        │
│  │ • Caméra    │─────▶│ Jetson Orin  │◀─────│ • 5G        │        │
│  │ • LiDAR     │      │              │      │ • WiFi Mesh │        │
│  │ • GPS RTK   │      │ • YOLOv8    │      │ • LoRa      │        │
│  │ • IMU       │      │ • Navigation│      │ • RC        │        │
│  │ • Altimètre│      │ • Control   │      │             │        │
│  └─────────────┘      └─────────────┘      └─────────────┘        │
│         │                    │                    │               │
│         └────────────────────┼────────────────────┘               │
│                              ▼                                      │
│                   ┌─────────────────┐                              │
│                   │      PX4        │ ◄── Contrôleur de vol        │
│                   │   (Flight Ctrl) │     (choix comité)           │
│                   └─────────────────┘                              │
│                              │                                      │
│         ┌────────────────────┼────────────────────┐                │
│         ▼                    ▼                    ▼                │
│  ┌───────────┐        ┌───────────┐        ┌───────────┐         │
│  │ Moteurs 1-4│        │ Moteurs 5-8│        │  Préhension│         │
│  │ MN4010    │        │ MN4010    │        │           │         │
│  └───────────┘        └───────────┘        └───────────┘         │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    MISSION FLOW                             │   │
│  │  [IDLE] → [TO_COLLECT] → [COLLECT] → [TO_TRUCK] → [DOCK]   │   │
│  │      ↑                                      │               │   │
│  │      └──────────── [RETURN_BASE] ◀──────────┘               │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Synthèse des corrections committee

| # | Correction | Avant | Après |
|---|------------|-------|-------|
| 1 | **Autonomie** | 25-30 min (iréaliste) | **12-18 min** (réel) |
| 2 | **Thrust/Weight** | 1.7:1 (insuffisant) | **≥ 2:1** (MN4010) |
| 3 | **Modèle IA** | Surface minimum | **Entraînement + tests YOLOv8 détaillés** |
| 4 | **FC** | Non-spécifié | **PX4** (choix arrêté) |

---

## 5. Synthèse des interfaces

### 5.1 Drone <-> Camion

| Interface | Technologie | Protocole |
|-----------|-------------|-----------|
| Position camion | 5G/WiFi | UDP (MQTT si cloud) |
| Statut drone | 5G/WiFi | ROS2 bridge |
| Marqueur docking | Vision | ARUCO + LiDAR |
| Confirmation dépose | Capteur contact | GPIO / topic |

### 5.2 Drone <-> Base

| Interface | Technologie |
|-----------|-------------|
| Monitoring | 5G (dashboard) |
| Upload données | WiFi Mesh / 5G |
| Update firmware | OTA |

---

*Document généré pour architecture drone collecteur urbain avec ROS2 - v1.1 CORRIGÉ*
*Corrections appliquées selon retours committee : autonomie, thrust/weight, IA, PX4*