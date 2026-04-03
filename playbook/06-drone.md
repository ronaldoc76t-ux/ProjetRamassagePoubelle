# Architecture drones collecteurs

## Story
- En tant qu’ingénieur drone, je veux un plan pour la mécanique, les capteurs et le logiciel afin de livrer une flotte sûre et fiable.
- Critère d’acceptation : concept drone, communication, prise/dépôt et logique de pilotage couvert.

## 1. Mécanique & électronique
- **Type: Octocopter cargo** (plus stable que quad pour docking)
- Masse max: 15kg (avec charge)
- **Charge utile: 1 sac de 50L max (~10kg)**
- **Autonomie réaliste: 12-15 min avec charge, 18-22 min à vide**

### Spécifications
| Composant | Modèle | Specs |
|-----------|--------|-------|
| Moteurs | T-Motor P80x2 | 1200W chacun, thrust/weight 2.2:1 |
| ESC | Hobbywing X8 | 4-in-1, 60A |
| Batterie | 8S LiPo 22Ah | 700g, 44.4V, 970Wh |
| LiDAR | Livox Mid-Mini | 40m range |
| Caméra | Oak-D Pro | AI vision |
| GPS | u-blox NEO-M9N | RTK Ready |
| IMU | BMI088 | High precision |

- **Ratio thrust/weight: 2.2:1** (marge sécurité acceptable)
- Gestion thermique: dissipateurs passifs + ventilation active pour vol stationnaire
- Préhension: bras robotique + fixation magnétique
- Dépose: guidage vision (AprilTags sur trémie)
- Communication: 5G + Wi-Fi mesh + LoRa backup

### Consommation Réelle (estimée)
| Mode | Puissance | Autonomie (970Wh) |
|------|-----------|-------------------|
| Décollage | 800W | - |
| Croisière | 450W | ~2h |
| Stationnaire | 350W | ~2.7h |
| Avec perception (AI) | +50W | ~1.7h |
| **Avec charge (10kg)** | 550W | **~12-15 min** |

## 2. Logiciel ROS2 Drone
- Nodes: vision_perception, object_detection, path_planner, approach_controller, docking_manager, battery_monitor, failsafe, comm_interface.
- Topics: /target_pose, /drone_pose, /battery_state, /docking_status.
- Actions/services: capture_target, rendezvous_execute, land_on_truck, return_base.
- Fonctions: détection sacs/bacs, planification, synchronisation camion mouvant, optimisation énergie, gestion échec.
- **Contrôleur de vol**: PX4 (préféré) avec paramétrage EKF + PID
- **Fréquence contrôle**: 200 Hz (amélioré de 100 Hz)
- **Précision docking**: < 30cm (réaliste, pas < 10cm)
