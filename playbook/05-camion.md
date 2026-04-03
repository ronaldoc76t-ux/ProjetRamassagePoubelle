# Architecture camion autonome

## Story
- En tant qu’ingénieur véhicule autonome, je veux un plan détaillé du camion pour guider conception matérielle et logiciels embarqués.
- Critère d’acceptation : architecture mécanique, capteurs, actuateurs, trémie et controle ROS2 documentés.

## 1. Mécanique & électronique
- Châssis renforcé avec plateforme de dépôt et stabilisation.
- **Vitesse cible: 30-50 km/h** (mode urbain)

### Capteurs (avec redondance)
| Capteur | Primary | Backup | IP | Role |
|---------|---------|--------|-----|------|
| LiDAR | Velodyne VLP-16 | Livox Mid-360 | IP67 | Percepción 360° |
| Caméra | Stereolabs ZED 2 | Realsense D455 | IP67 | Vision AI |
| Radar | Continental ARS408 | - | IP67 | Détection longue portée |
| GPS | u-blox ZED-F9P RTK | - | IP66 | Position cm-level |
| IMU | XSens MTi-680G | - | IP65 | Orientation/-heading |

- **Système nettoyage**: Wiper automatique + air comprimé pour capteurs extérieurs
- Actuateurs: direction assistée + EPS, freinage électronique, propulsion hybride ou électrique.
- Trémie: zone réception amortie, capteurs niveau, compacteur hydraulique, sécurités anti-retournement.
- Alimentation: LiFePO4 (400 kWh pour 4-6h autonomie), alternateur, UPS, gestion BMS.
- Sécurité: barrières, E-stop, prototypes anti-obstacle.

## Bilan Massique
| Système | Poids (kg) |
|---------|-----------|
| Structure plateforme | 850 |
| Capteurs | 45 |
| Système hydraulique | 120 |
| Électronique | 35 |
| Batterie traction | 2800 |
| Batterie auxiliaire | 80 |
| **Total** | **~3930** |

PTAC: 26-32 tonnes - Marge commerciale disponible: ~22 tonnes

## 2. Logiciel ROS2 Camion
- Nodes principaux: odometry, perception, localisation, controller, rendezvous, trajectory_predictor, tranche_manager, backend_interface.
- Topics: /pose, /cmd_vel, /rendezvous_request, /rendezvous_status, /payload_state, /obstacle_alert.
- Services/actions: start_mission, cancel_mission, dump_payload, calibrate_sensor.
- Fonctionnalités: navigation en mouvement continue, prédiction 30-120s, gestion rendez-vous drones, régulation trémie.
- Gestion risques: e-stop, dérive, perte communication, recalage.

## Mode Dégradé (perte GPS)
- **Fallback**: Odométrie visuelle (optical flow) + IMU
- **Précision attendue**: ±2-5m en milieu urbain
- **Durée max**: 10 minutes avant alerte
- **Réconciliation**: Retour zone couverte pour recalage
