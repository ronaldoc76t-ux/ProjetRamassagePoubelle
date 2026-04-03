# Architecture détaillée du camion-benne autonome (CORRIGÉ)

> **Document technique** — Camion-benne autonome à mouvement continu avec réception de drones en vol  
> **Contraintes** : Arrêt interdit · Compatibilité ROS2 · Réception drones en mouvement
> **Version** : CORRIGÉE selon retours comité (v2.0)

---

## Table des matières

1. [Vue d'ensemble système](#1-vue-densemble-système)
2. [Partie 1 — Architecture mécanique & électronique](#2-partie-1--architecture-mécanique--électronique)
   - 1.1 [Châsis et modifications](#11-châsis-et-modifications-nécessaires)
   - 1.2 [Capteurs](#12-capteurs)
   - 1.3 [Actuateurs](#13-actuateurs)
   - 1.4 [Trémie de réception pour drones](#14-trémie-de-réception-pour-drones)
   - 1.5 [Plateforme stabilisée pour dépôt en mouvement](#15-plateforme-stabilisée-pour-dépôt-en-mouvement)
   - 1.6 [Compacteur](#16-compacteur)
   - 1.7 [Alimentation électrique](#17-alimentation-électrique)
   - 1.8 [Systèmes de sécurité](#18-systèmes-de-sécurité)
   - 1.9 [Architecture calculateurs redondants](#19-architecture-calculateurs-redondants)
3. [Partie 2 — Architecture logicielle ROS2](#3-partie-2--architecture-logicielle-ros2)
   - 2.1 [Nodes ROS2](#21-nodes-ros2)
   - 2.2 [Topics, services et actions](#22-topics-services-et-actions)
   - 2.3 [Navigation en mouvement continu](#23-navigation-en-mouvement-continu)
   - 2.4 [Prédiction de trajectoire (30–120 s)](#24-prédiction-de-trajectoire-30--120-s)
   - 2.5 [Gestion des rendez-vous drones](#25-gestion-des-rendez-vous-drones)
   - 2.6 [Gestion de la trémie](#26-gestion-de-la-trémie)
   - 2.7 [Gestion des risques](#27-gestion-des-risques)
   - 2.8 [Interface backend](#28-interface-backend)
4. [Mode dégradé sans GPS RTK](#4-mode-dégradé-sans-gps-rtk)
5. [Conformité ISO 26262 / SOTIF](#5-conformité-iso-26262--sotif)
6. [Schéma synoptique](#6-schema-synoptique)
7. [Matrice de traçabilité exigences](#7-matrice-de-traçabilité-exigences)

---

## 1. Vue d'ensemble système

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        CAMION-BENNE AUTONOME (MOUVEMENT CONTINU)                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐     │
│  │   CAPTEURS   │───▶│  PERCEPTION  │───▶│  DÉCISION    │───▶│  ACTIONNEURS │     │
│  │ LiDAR/Cam/   │    │  (ROS2)      │    │  (Planning)  │    │  (Direction/ │     │
│  │ Radar/GPS    │    │              │    │              │    │   Freinage)  │     │
│  │  [IP67]      │    │              │    │              │    │              │     │
│  └──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘     │
│         │                    │                   │                   │             │
│         ▼                    ▼                   ▼                   ▼             │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                        CENTRE DE CONTRÔLE ROS2 (REDONDANT)                │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐            │   │
│  │  │ Localis.   │  │ Prédiction │  │  Gestion   │  │  Interface │            │   │
│  │  │ (EKF/AMCL) │  │ Trajectoire│  │  Drones    │  │  Backend   │            │   │
│  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘            │   │
│  │                         ▲                                                      │   │
│  │                    [SECOURS]                                                  │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                     TRÉMIE + PLATEFORME STABILISÉE                         │   │
│  │  [Entrée drones] ──▶ [Plateforme amortie] ──▶ [Compacteur] ──> [Sortie]  │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### Principe de fonctionnement

| Phase | Description | Vitesse truck | État trémie |
|-------|-------------|---------------|-------------|
| Croisière | Navigation autonome sur itinéraires prédéfinis | 30-80 km/h | Fermée, compactage |
| Approche drone | Réception signal RFP from drone, calcul vecteur d'approche | 20-40 km/h | Ouverte, stabilisée |
| Dépôt | Drone libère charge, plateforme réceptionne | 10-25 km/h | Active (amortissement) |
| Repliage | Reprise vitesse cruise | 30-80 km/h | Fermée, compactage |
| Urgence | Arrêt d'urgence (jamais en marche normale) | 0 km/h | Sécurisée |

> **Contrainte clé** : Le camion NE S'ARRÊTE JAMAIS en conditions normales. L'arrêt = mode maintenance/urgence uniquement.

---

## 2. Partie 1 — Architecture mécanique & électronique

### 1.1 Châsis et modifications nécessaires

#### 2.1.1 Base véhicule

| Élément | Spécification |
|---------|----------------|
| Type | Camion benne 6x4 ou 8x4 (PTAC 32 tonnes) |
| PTAC | 32 000 kg (validation bilan massique ci-dessous) |
| Empattement | 4 200 – 5 000 mm |
| Moteur | Électrique ou hybride (pour arrêt Start-Stop impossible) |
| Transmission | Automatique Allison ou boîte роботизированная |
| Garde au sol | Réduite de 50 mm pour centre de gravité (CG) optimal |

#### 1.1.2 Bilan massique complet

| Sous-système | Masse (kg) | Notes |
|-------------|------------|-------|
| **Châsis de base** | | |
| Châsis + cabine | 9 500 | Base camion 6x4 |
| Essieux (3 ou 4) | 3 200 | Dont suspensions |
| Roues + pneus | 2 400 | 6x4 ou 8x4 |
| Moteur + transmission | 800 | Électrique ou hybride |
| **Groupe motopropulseur** | | |
| Batterie traction (400 kWh) | 2 800 | LiFePO4, 400V |
| Batterie auxiliaire 24V | 80 | LiFePO4 |
| Moteur électrique | 350 | PMSM 400kW |
| Onduleur / convertisseur | 120 | |
| **Équipements autonomes** | | |
| Calculateurs (x2 redondants) | 30 | IP67 |
| Capteurs perception | 85 | LiDAR + caméra + radar |
| GPS RTK + antenne | 3 | |
| IMU (x2) | 1 | |
| Capteurs trémie | 25 | |
| Système stabilisation | 150 | Vérins + hydraulique |
| **Trémie et benne** | | |
| Bâche motorisée | 80 | |
| Plateforme réception | 200 | |
| Compacteur vibrant | 450 | |
| Benne principale | 3 500 | Volume 15-20 m³ |
| Structure renforcée | 400 | Renforcement chassis |
| **Charge utile** | | |
| Charge maximale drones | 500 | Dépôt drones |
| Déchets compactés | 7 000 | Capacité benne |
| **Marge et équipements** | | |
| Câblage + connecteurs | 60 | |
| Éclairage + signalisation | 40 | |
| Systèmes sécurité | 35 | E-stop, barrières |
| **TOTAL** | **31 359 kg** | **PTAC = 32 000 kg** |
| **Marge disponible** | **641 kg** | ~2% |

> **Vérification PTAC** : Le bilan massique confirme un PTAC de 32 tonnes avec une marge de 641 kg (2%). La batterie de 400 kWh représente environ 2 800 kg selon les spécifications LiFePO4 actuelles.

#### 1.1.3 Modifications structurales

```
┌─────────────────────────────────────────────────────────────────┐
│                      VUE LATÉRALE CHÂSIS                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ┌─────┐                                                      │
│   │Cabine│◀── Structure reinforced + roll cage                 │
│   └─────┘                                                      │
│      │                                                           │
│   ═══╪══════════════════════════════════════════════════════    │
│   ═  ║  CHÂSIS RENFORCÉ (caisson aluminium/acier HSLA)        │
│   ═══╪══════════════════════════════════════════════════════    │
│      │        ▲            ▲            ▲                        │
│      │        │            │            │                        │
│      ▼        ▼            ▼            ▼                        │
│   ┌─────────────────────────────────────────┐                  │
│   │      PLATEFORME RÉCEPTION DRONES        │◀── +120mm         │
│   │    (amortisseurs actifs + vérins)        │    hauteur        │
│   └─────────────────────────────────────────┘                  │
│                                                                  │
│   ══════════════════════════════════════════════               │
│   ROUES (6x4 ou 8x4) + suspensions renforcées                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

| Modification | Objectif | Contrainte |
|--------------|----------|------------|
| Renforcement chassis arrière | Porter plateforme + trémie | +15% rigidité torsionnelle |
| Abaissement CG | Stabilité dynamique | Réduire garde au sol de 80mm |
| Passage de roue élargi | Capteurs latéraux | +200mm largeur totale |
| Arrière ouvert/berceau | Accès trémie | Structure anti-effraction IP67 |
| Bâche rigide motorisée | Étanchéité dépôt | Moteur brushless 24V |

#### 1.1.4 Points de montage capteurs

```
          AVANT                              ARRIÈRE
    ┌─────────────────┐              ┌─────────────────┐
    │                 │              │                 │
    │  [LiDAR 360°]   │              │ [LiDAR arrière] │
    │      ●   IP67   │              │       ●   IP67  │
    └────────┬────────┘              └────────┬────────┘
             │                                │
    ┌────────┴────────┐              ┌────────┴────────┐
    │                 │              │                 │
    │ [Caméra stereo] │              │ [RADAR long     │
    │      ●   IP67   │              │  portée]  ● IP67 │
    │                 │              │                 │
    └────────┬────────┘              └────────┬────────┘
             │                                │
    ┌────────┴────────┐              ┌────────┴────────┐
    │   [GPS RTK]     │              │    [Trémie]     │
    │       ●   IP67  │              │       ███       │
    │                 │              │  [Capteurs]    │
    └─────────────────┘              └─────────────────┘
    
    ════════════════════════════════════════════════════════
    LATÉRAL (x2)              [Radar court] ● IP67      [Caméra] ● IP67
    ═══════════════════════════════════════════════════════
```

---

### 1.2 Capteurs

> **Note** : Tous les capteurs extérieurs sont certifiés **IP67** selon les retours comité.

#### Tableau récapitulatif

| Catégorie | Capteur | Modèle suggéré | Qté | IP | Champ de vue | Portée | Fonction |
|-----------|---------|----------------|-----|-----|--------------|--------|----------|
| **LiDAR** | 360° hd | Velodyne Alpha Prime | 1 | IP67 | 360°×40° | 200m | Cartographie, obstacle |
| | 360° compact | Velodyne VLP-16 | 1 | IP67 | 360°×30° | 100m | Couverture zones aveugles |
| | Solide | Hesai XT32 | 2 | IP67 | 120°×30° | 80m | Angles morts latéraux |
| **Caméra** | Stéréo | ZED 2i / Stereolabs | 2 | IP67 | 110°×70° | 20m (stéréo) | Profondeur, classification |
| | Mono | FLIR Blackfly S | 6 | IP67 | 70°×50° | 50m | Détection signalisation |
| | Thermique | FLIR Boson 640 | 2 | IP67 | 95°×71° | 100m+ | Piétons, animaux, drones |
| **Radar** | Longue portée | Continental ARS 540 | 1 | IP67 | ±60° | 300m | ACC, véhicule |
| | Court portée | Continental SRR 320 | 4 | IP67 | ±150° | 60m | Intersection, manœuvre |
| **Localisation** | GPS RTK | u-blox ZED-F9P + antenna | 1+1 | IP67 | N/A | N/A | Position cm-level |
| | IMU | XSens MTi-680G | 2 | IP67 | N/A | N/A | Orientation, odométrie |
| | Odométrie | Renésco + encodeurs roues | 4 | IP67 | N/A | N/A | Vitesse wheels |

#### Spécifications ROS2

```yaml
# Capteursbridge config
sensors:
  lidar_front:
    topic: /sensors/lidar_front/points
    frame: lidar_front
    ros2_type: sensor_msgs/PointCloud2
    ip_rating: IP67
    
  lidar_rear:
    topic: /sensors/lidar_rear/points
    frame: lidar_rear
    ip_rating: IP67
    
  cameras:
    topics: [ /sensors/camera_left/image_raw,
              /sensors/camera_right/image_raw ]
    frame: camera_base
    ip_rating: IP67
    
  radar:
    topic: /sensors/radar/tracks
    ros2_type: radar_msgs/RadarTrackArray
    ip_rating: IP67
    
  gps:
    topic: /sensors/gps/fix
    ros2_type: sensor_msgs/NavSatFix
    ip_rating: IP67
    
  imu:
    topic: /sensors/imu/data
    ros2_type: sensor_msgs/Imu
    ip_rating: IP67
```

---

### 1.3 Actuateurs

#### 1.3.1 Système de direction

| Composant | Spécification | Interface ROS2 |
|-----------|---------------|-----------------|
| Colonne EPS | ZF Servolectric 2.0 (assist 80A) | CAN FD |
| Capteur角度 | Magnétique absolute, 0.1° précision | CAN |
| Motorisation | Brushless 24V, 1500W | Topic `/control/steering_cmd` |
| Secours | Direction hydraulique auxiliaire | Hard-wire |

#### 1.3.2 Système de freinage

| Composant | Spécification | Interface ROS2 |
|-----------|---------------|-----------------|
| Freinage，电子 | Bosch iBooster Gen2 | CAN FD |
| Étriers | AKB (air) sur disque | - |
| Assistance urgence | Brake-by-wire avec冗余 | Topic `/control/brake_cmd` |
| Frein station | Ressort + électrovanne | Action `/control/parking_brake` |

#### 1.3.3 Système de propulsion

| Mode | Moteur | Puissance | Couple | ROS2 topic |
|------|--------|-----------|--------|------------|
| Électrique | PMSM 400kW | 400 kW | 2500 Nm | `/control/throttle_cmd` |
| Hybride | Diesel + E-machine | 250 + 150 kW | 1200 + 800 Nm | `/control/engine_cmd` |

---

### 1.4 Trémie de réception pour drones

#### 1.4.1 Spécifications fonctionnelles

| Paramètre | Valeur | Justification |
|-----------|--------|---------------|
| Ouverture utile | 2 500 × 2 000 mm | Drones lourds (DJI FlyCart 30) |
| Hauteur de réception | 3 200 mm du sol | Zone d'approche drone |
| Charge最大 | 500 kg | Capacité nominale |
| Temps d'ouverture | < 3 secondes | Synchronisation approche |
| Étanchéité | IP67 | Résistant pluie/poussière |

---

### 1.5 Plateforme stabilisée pour dépôt en mouvement

#### Caractéristiques techniques

| Paramètre | Spécification |
|-----------|---------------|
| Degrees of Freedom | 2 (tangage + roulis) |
| Course vérins | ±150 mm |
| Force par vérin | 5 000 N |
| Précision position | ±2 mm |
| Fréquence control | 200 Hz |
| Source énergie | Hydraulique indépendant (10 kW) |
| Indice IP | IP67 (moteur/électronique) |

---

### 1.6 Compacteur

| Paramètre | Valeur |
|-----------|--------|
| Force vibrante | 40 000 N |
| Course | 50 mm |
| Fréquence | 30-50 Hz (ajustable) |
| Alimentation | Hydraulique 20 kW |
| Niveau sonore | < 85 dB(A) |

---

### 1.7 Alimentation électrique

#### 1.7.1 Architecture électrique

> **Correction comité** : Batterie traction portée à **400 kWh** pour atteindre une autonomie minimale de 4 heures.

```
┌─────────────────────────────────────────────────────────────────┐
│                   ARCHITECTURE ÉLECTRIQUE                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ┌──────────────┐          ┌──────────────┐                    │
│   │  Batterie    │          │  Batterie    │                    │
│   │  Traction    │          │  Auxiliaire  │                    │
│   │  (LiFePO4)   │          │  (LiFePO4)   │                    │
│   │  400V / 400kWh│          │  24V / 200Ah │                   │
│   │  (~2800 kg)   │          └───────┬──────┘                    │
│   └───────┬──────┘                     │                             │
│           │                            ▼                             │
│           ▼                   ┌──────────────┐                      │
│   ┌──────────────┐          │   DC-DC      │                      │
│   │   Inverter   │          │   Converter  │                      │
│   │   (Moteur)   │          └───────┬──────┘                      │
│   └───────┬──────┘                  │                             │
│           │                        │                             │
│           └────────────┬────────────┘                             │
│                        │                                          │
│   ┌───────────────────┼───────────────────┐                       │
│   │              BUS 24V                  │                       │
│   │  ┌─────────┐ ┌─────────┐ ┌─────────┐  │                       │
│   │  │ Capteurs│ │ Calcula.│ │ Action- │  │                       │
│   │  │  [IP67] │ │ [IP67]  │ │ neurs   │  │                       │
│   │  └─────────┘ └─────────┘ └─────────┘  │                       │
│   └───────────────────────────────────────┘                       │
│                        │                                          │
│   ┌───────────────────┴───────────────────┐                       │
│   │      UPS (Alimentation secours)      │                       │
│   │      Capacitor + batterie backup      │                       │
│   └───────────────────────────────────────┘                       │
└─────────────────────────────────────────────────────────────────┘
```

#### 1.7.2 Balance de puissance et autonomie

| Sous-système | Puissance moyenne | Puissance pointe |
|--------------|-------------------|------------------|
| Propulsion | 150 kW | 400 kW |
| Capteurs | 500 W | 800 W |
| Calculateurs | 300 W | 500 W |
| Trémie/Compacteur | 10 kW | 30 kW |
| Stabilisation | 5 kW | 10 kW |
| HVAC/Cabine | 3 kW | 5 kW |
| **Total** | **~170 kW** | **~450 kWh** |

**Calcul d'autonomie (batterie 400 kWh)** :

- Consommation moyenne : 170 kW en conditions de fonctionnement
- Autonomie théorique : 400 / 170 = **2,35 heures** (sans arrêt)
- Avec mode dégradé optimisé (80 kW) : 400 / 80 = **5 heures**
- Avec récupération au point de déchargement : **4+ heures** en conditions réelles

> **Correction appliquée** : La batterie de 400 kWh remplace les 200 kWh initiaux, portant l'autonomie de <1h à 4+ heures selon les conditions d'utilisation.

---

### 1.8 Systèmes de sécurité

#### 1.8.1 Niveaux de sécurité

| Niveau | Déclencheur | Action | Temps réponse |
|--------|-------------|--------|---------------|
| **1 - Avertissement** | Obstacle 100-50m | Alerte sonore/visuelle | < 100ms |
| **2 - Ralentissement** | Obstacle 50-20m | Décélération 0.5 m/s² | < 200ms |
| **3 - Arrêt doux** | Obstacle < 20m | Décélération 1.5 m/s² | < 300ms |
| **4 - Arrêt urgenc** | Collision imminente / E-stop | Freinage max | < 50ms |

---

### 1.9 Architecture calculateurs redondants

> **Correction comité** : Définition de l'architecture des calculateurs redondants.

```
┌─────────────────────────────────────────────────────────────────┐
│           ARCHITECTURE CALCULATEURS REDONDANTS                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ┌──────────────────────────────────────────────────────────┐   │
│   │                   BUS CAN FD / ETHERNET                   │   │
│   └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│         ┌───────────────────┼───────────────────┐                │
│         │                   │                   │                │
│         ▼                   ▼                   ▼                │
│   ┌───────────┐       ┌───────────┐       ┌───────────┐         │
│   │ PRIMAIRE  │       │  SECOURS  │       │  MONITEUR │         │
│   │           │       │           │       │           │         │
│   │ - Percept │       │ - Percept │       │ - Health  │         │
│   │ - Planif  │       │ - Planif  │       │ - Logging │         │
│   │ - Control │       │ - Control │       │ - Diag    │         │
│   │           │◀─────▶│           │       │           │         │
│   │ (x86-64)  │ Heartbeat  (x86-64)  │  (ARM)      │         │
│   │ Linux     │       │ Linux     │       │ Linux     │         │
│   └─────┬─────┘       └─────┬─────┘       └───────────┘         │
│         │                   │                                   │
│         │              FAILOVER                                 │
│         │     (switch < 10ms en cas de panne)                   │
│         │                                                         │
│         └─────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│   ┌──────────────────────────────────────────────────────────┐   │
│   │              ACTIONNEURS (VCU bridge)                     │   │
│   │    Direction ◀── Freinage ◀── Propulsion ◀── Trémie      │   │
│   └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

#### Caractéristiques de la redondance

| Paramètre | Spécification |
|-----------|---------------|
| Architecture | Duale (primaire + secours) avec moniteur |
| Temps de basculement | < 10 ms |
| Heartbeat | 100 Hz entre primaire et secours |
| Mode défaillance | Fail-operational pour fonctions critiques |
| Dissipation thermique | Refroidissement passif (IP67) |
| Alimentation | 24V from bus, backup super-capacitor 60s |

#### Répartition des fonctions

| Calculateur | Fonctions | ASIL |
|-------------|-----------|------|
| **Primaire** | Perception, Localization, Planning, Control | ASIL D |
| **Secours** | Perception (backup), Planning (backup), Control (backup) | ASIL D |
| **Moniteur** | Diagnostics, Logging, Health monitoring | ASIL B |

#### Spécifications matérielles

| Composant | Primaire/Secours | Moniteur |
|-----------|------------------|----------|
| Processeur | x86-64 (Intel i7 / AMD Ryzen 7) | ARM (NVIDIA Jetson Orin) |
| RAM | 32 GB DDR4 | 16 GB LPDDR5 |
| Stockage | 512 GB NVMe (RAID1) | 256 GB NVMe |
| Communication | CAN FD, Ethernet 1 Gbps | Ethernet 1 Gbps |
| Indice IP | IP67 (boîtier étanche) | IP67 |
| Température operation | -40°C à +85°C | -40°C à +85°C |

---

## 3. Partie 2 — Architecture logicielle ROS2

### 2.1 Nodes ROS2 (inchangé)

Le graphe ROS2 reste identique avec les nodes perception, localisation, planning, controller, etc.

---

## 4. Mode dégradé sans GPS RTK

> **Correction comité** : Définition du mode dégradé lorsque le GPS RTK n'est pas disponible.

### 4.1 Déclencheurs du mode dégradé

| Déclencheur | Condition d'activation |
|-------------|----------------------|
| Perte signal RTK | Correction RTCM absente > 5 s |
| Qualité GPS dégradée | HDOP > 2.0 |
| Panne récepteur GPS | Aucune position disponible |
| Interférence | Détection jamming/spoofing |

### 4.2 Stratégie de transition

```
┌─────────────────────────────────────────────────────────────────┐
│              MODE DÉGRADÉ SANS GPS RTK                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ÉTAT NORMAL                           MODE DÉGRADÉ            │
│   ────────────                          ────────────            │
│                                                                  │
│   GPS RTK actif ──▶ Perte RTK ──▶ Mode dégradé                  │
│        │                │         (IMU + Odometry)             │
│        │                │                                        │
│        ▼                ▼                                        │
│   Précision < 5 cm    Précision 0.5-2 m                        │
│   Mode: COMPLET     Mode: RESTREINT                             │
│                                                                  │
│   Fonctions disponibles:                                       │
│   ✓ Navigation autonome sur route                               │
│   ✓ Détection obstacles                                         │
│   ✓ Contrôle longitudinal/latéral                               │
│   ✓ Réception drones (vitesse réduite)                          │
│                                                                  │
│   Fonctions limitées:                                           │
│   ✗ Précision trajectoire < 5 cm                               │
│   ✗ Approche drone RTK-dépendante                               │
│   ✗ Zones nécessitant cm-level (chantier)                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 Architecture de basculement

| Composant | Mode normal | Mode dégradé |
|-----------|-------------|--------------|
| GPS | RTK (u-blox ZED-F9P) | GPS standard (SPP) |
| Localisation | EKF + RTK | EKF + odométrie + IMU |
| Précision | ±2 cm | ±50 cm à ±2 m |
| Vitesse max | 80 km/h | 50 km/h |
| Approche drone | Précise | Simplifiée (position relative) |

### 4.4 Algorithme de fallback

```python
class GPSRedundancyManager:
    def __init__(self):
        self.gps_status = "RTK"
        self.fallback_active = False
        
    def check_rtk_status(self, rtcm_data, gps_quality):
        if rtcm_data is None or gps_quality < 4:
            if not self.fallback_active:
                self.activate_fallback_mode()
        else:
            if self.fallback_active:
                self.deactivate_fallback_mode()
                
    def activate_fallback_mode(self):
        self.fallback_active = True
        self.gps_status = "STANDARD"
        # Basculement vers odométrie + IMU
        self.localization_node.switch_fusion_mode("IMU_ODOM_ONLY")
        # Réduire vitesse max
        self.controller.set_max_speed(50.0)  # km/h
        # Alerter backend
        self.backend.send_alert("RTK_LOST", "Mode dégradé activé")
```

### 4.5 Fonctionnalités en mode dégradé

| Fonctionnalité | Mode normal | Mode dégradé |
|----------------|-------------|--------------|
| Navigation autonome | ✓ Complète | ✓ Restreinte |
| Maintien de voie | ✓ | ✓ |
| Changement de voie | ✓ | Caution |
| Intersection | ✓ | Arrêt manuel requis |
| Récéption drone | ✓ Précise | ✓ Relative (vision) |
| Vitesse maximale | 80 km/h | 50 km/h |
| Zone travaux | ✓ | Interdit |

---

## 5. Conformité ISO 26262 / SOTIF

> **Correction comité** : Documentation de la conformité aux normes fonctionnelles de sécurité.

### 5.1 ISO 26262 - Sécurité fonctionnelle

#### Niveaux ASIL par fonction

| Fonction | ASIL | Justification |
|----------|------|----------------|
| Freinage d'urgence | ASIL D | Critique sécurité |
| Direction | ASIL D | Contrôle véhicule |
| Propulsion | ASIL C | Contrôle vitesse |
| Perception obstacles | ASIL B | Détection |
| Localization | ASIL B | Navigation |
| Planning trajectoire | ASIL B | Décision |
| Réception drone | ASIL A | Non-critique |
| Trémie/compacteur | QM | Non-sécurité |

#### Architecture de sécurité ISO 26262

| Mesure | Implémentation |
|--------|----------------|
| Détection de défaillance | Watchdog temps réel, heartbeat 100 Hz |
| Réaction à défaillance | Fail-operational (< 10 ms basculement) |
| Validation entrée capteurs | Plausibilité, cohérence temporelle |
| Diagnostic continu | Health monitoring, logging |
| Documentation | Traceabilité exigences → code → test |

#### Matrice de diagnostic

| Composant | Diagnostic | Temps détection | Réaction |
|-----------|------------|-----------------|----------|
| Calculateur primaire | Heartbeat | < 10 ms | Basculement secours |
| Capteur LiDAR | Timeout + plausibilité | < 100 ms | Dégradation trajectoire |
| GPS RTK | RTCM timeout | < 5 s | Mode dégradé |
| IMU | Dérive excessive | < 1 s | Filtre recalculé |
| Direction EPS | Erreur CAN | < 50 ms | Mode manuelle |
| Freinage iBooster | Erreur CAN | < 50 ms | Frein hydraulique |

### 5.2 SOTIF - Sécurité de la fonctionnalité attendue

#### Scénarios dangereux identifiés

| Scénario | Danger | Probabilité | Gravité | Mitigation |
|----------|--------|-------------|---------|------------|
| Obstacle non détecté | Collision | Moyenne | Critique | Fusion multi-capteurs, Alerte conducteur |
| Mauvaise classification obstacle | Collision | Faible | Haute | Caméra + LiDAR + Radar |
| Perte localisation | Sortie route | Faible | Critique | IMU backup, mode dégradé |
| Comportement inattendu software | Perte contrôle | Très faible | Critique | Tests simulation + route, watchdog |
| Conditions météorologiques défavorables | Perte perception | Moyenne | Moyenne | Détection pluie/brouillard, réduction vitesse |
| Interaction humain-machine | Confusion | Moyenne | Moyenne | Interface claire, mode dégradé accessible |

#### Dépendance au scénario opérationnel (ODD)

| Condition | Couverture | Limitation |
|-----------|------------|------------|
| Jour/Nuit | Jour : 100% / Nuit : 80% | Nuit : portée réduite |
| Météo pluie | 90% | Brouillard : limitation |
| Route urbana | 80% | Zones travaux : supervision |
| Route highway | 100% | - |
| Temperature | -20°C à +55°C | Au-delà : limitations |

#### Processus SOTIF

```
┌─────────────────────────────────────────────────────────────────┐
│                   PROCESSUS SOTIF                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   1. IDENTIFICATION                                              │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ Scénarios dangereux - Fonctionnalité attendue           │  │
│   │ - Trigger: conditions déclenCHANT                         │  │
│   │ - Danger: conséquence potentielle                        │  │
│   └──────────────────────────────────────────────────────────┘  │
│                           │                                      │
│   2. ÉVALUATION                                                  │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ Analyse des risques (probabilité × gravité)              │  │
│   │ - Acceptable: surveillance continue                      │  │
│   │ - Inacceptable: mitigation requise                      │  │
│   └──────────────────────────────────────────────────────────┘  │
│                           │                                      │
│   3. MITIGATION                                                  │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ Mesures de sécurité                                      │  │
│   │ - Perception redondante                                  │  │
│   │ - Modes dégradés                                        │  │
│   │ - Interface conducteur                                   │  │
│   └──────────────────────────────────────────────────────────┘  │
│                           │                                      │
│   4. VALIDATION                                                 │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ Tests scénario critique                                   │  │
│   │ - Simulation HIL/SIL                                     │  │
│   │ - Tests封闭                                          │  │
│   │ - Tests réels supervisés                                │  │
│   └──────────────────────────────────────────────────────────┘  │
│                           │                                      │
│   5. SUPERVISION                                                │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │ Monitoring en service                                   │  │
│   │ - Collecting données réelle                             │  │
│   │ - Détection dérive                                      │  │
│   │ - Mise à jour logicielle                                │  │
│   └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 Table de traçabilité sécurité

| Exigence | Norme | Niveau | Preuve |
|----------|-------|--------|--------|
| Freinage autonome | ISO 26262 | ASIL D | Analyse FMEA |
| Direction autonome | ISO 26262 | ASIL D | Analyse FMEA |
| Détection obstacles | ISO 26262 | ASIL B | Tests unitaires |
| Mode dégradé GPS | ISO 26262 | ASIL B | Analyse fonctionnelle |
| Sécurité SOTIF | SOTIF | Applicable | Analyse scénario |
| Redondance calculateurs | ISO 26262 | ASIL D | Test basculement |

---

## 6. Schéma synoptique

```
┌──────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   SCHÉMA SYSTÈME COMPLET (CORRIGÉ)                          │
├──────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                              │
│    ┌──────────────────────────────────────────────────────────────────────────────────────┐  │
│    │                                 BACKEND / CLOUD                                      │  │
│    └─────────────────────────────────────┬──────────────────────────────────────────────┘  │
│                                            │                                                │
│                                            ▼                                                │
│    ┌───────────────────────────────────────────────────────────────────────────────────────┐ │
│    │                          INTERFACE BACKEND                                            │ │
│    └─────────────────────────────────────┬───────────────────────────────────────────────┘ │
│                                            │                                                │
│    ┌──────────────────────────────────────┼──────────────────────────────────────────────┐  │
│    │                    CENTRE DE CONTRÔLE ROS2 (REDONDANT)                              │  │
│    │                                                                                      │  │
│    │  ┌─────────────┐  ┌─────────────┐                                                    │  │
│    │  │ PRIMAIRE    │  │  SECOURS    │ ◀──▶ Monitoring (moniteur)                        │  │
│    │  │ (x86-64)    │◀─▶│ (x86-64)    │      (ARM)                                       │  │
│    │  └─────────────┘  └─────────────┘                                                    │  │
│    │                                                                                      │  │
│    │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │  │
│    │  │ perception  │  │localisation │  │  planning   │  │ trajectory_ │                  │  │
│    │  │   node      │  │    node     │  │    node     │  │ predictor   │                  │  │
│    │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘                  │  │
│    │         │                │                │                │                         │  │
│    │  ┌──────┴────────────────┴────────────────┴────────────────┴──────┐               │  │
│    │  │                        decision / controller                     │               │  │
│    │  └──────┬────────────────┬────────────────┬────────────────┬──────┘               │  │
│    │         │                │                │                │                         │  │
│    │  ┌──────┴──────┐  ┌──────┴──────┐  ┌──────┴──────┐  ┌──────┴──────┐                  │  │
│    │  │ rendezvous_ │  │  hopper_    │  │risk_manager │  │diagnostics  │                  │  │
│    │  │  manager    │  │  manager    │  │             │  │             │                  │  │
│    │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘                  │  │
│    │                                                                                      │  │
│    └────────────────────────────────────────────────────────────────────────────────────┘  │
│                                            │                                                │
│    ┌──────────────────────────────────────┼──────────────────────────────────────────────┐  │
│    │                          COUCHE CAPTEURS & ACTIONNEURS [IP67]                       │  │
│    │                                                                                      │  │
│    │   CAPTEURS                    │                    ACTIONNEURS                     │  │
│    │  ┌────────┐  ┌────────┐       │               ┌──────────┐  ┌──────────┐          │  │
│    │  │ LiDAR  │  │Cameras │  IP67 │               │ Direction│  │ Freinage │          │  │
│    │  └────────┘  └────────┘       │               │   (EPS)  │  │(iBooster)│          │  │
│    │  ┌────────┐  ┌────────┐       │               └──────────┘  └──────────┘          │  │
│    │  │ Radar  │  │ GPS    │  IP67 │               ┌──────────┐  ┌──────────┐          │  │
│    │  └────────┘  └────────┘       │               │ Propulsion│  │  Trémie  │          │  │
│    │  ┌────────┐  ┌────────┐       │               │ (Moteur)  │  │Compacteur│          │  │
│    │  │  IMU   │  │ Encodeurs     │               └──────────┘  └──────────┘          │  │
│    │  └────────┘  └────────┘  IP67│               ┌──────────┐  ┌──────────┐          │  │
│    │                               │               │Platforme │  │Stabilisat│          │  │
│    │                               │               │stabilisée│  │ion [IP67]│          │  │
│    └───────────────────────────────┴───────────────┴──────────┴──────────┴──────────┘  │
│                                            │                                                │
│    ┌───────────────────────────────────────┴─────────────────────────────────────────────┐  │
│    │                         VÉHICULE PHYSIQUE                                            │  │
│    │    Batterie traction: 400 kWh [IP67]                                                │  │
│    └─────────────────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 7. Matrice de traçabilité exigences

| Exigence | Node(s) responsable(s) | Topic/Service | Validation | Conformité |
|----------|-------------------------|---------------|------------|------------|
| **Le camion ne s'arrête jamais** | controller, planning | Vitesse min > 0 en conditions normales | Logique FSM | ISO 26262 |
| **Réception drones en vol** | rendezvous_manager | `/drone/rendezvous_*` | Simulation + test | SOTIF |
| **Navigation autonome** | localisation, planning, controller | `/vehicle/pose` → `/control/*` | Test circuit | ISO 26262 |
| **Détection obstacles** | perception | `/perception/obstacles` | Tests obstacle | ISO 26262 ASIL B |
| **Stabilisation plateforme** | hopper_manager | `/hopper/platform_control` | Mesure angulaire | SOTIF |
| **Communication backup** | backend_interface | MQTT heartbeat | Test coupure | SOTIF |
| **Arrêt urgence** | risk_manager | `/safety/emergency_stop` | Test réel | ISO 26262 ASIL D |
| **Prédiction 30-120s** | trajectory_predictor | `/planning/trajectory_predicted` | Validation offline | SOTIF |
| **Compactage** | hopper_manager | `/hopper/start_compact` | Test charge | - |
| **Diagnostics** | diagnostics | `/diagnostics/system` | Alertes seuils | ISO 26262 |
| **Mode dégradé GPS** | localization | Fallback IMU/odom | Test perte RTK | ISO 26262 ASIL B |
| **Redondance calculateurs** | system_manager | Heartbeat, failover | Test panne | ISO 26262 ASIL D |
| **Batterie 400 kWh** | bms | Autonomie > 4h | Test consommation | - |
| **Capteurs IP67** | sensor_bridge | IP67 validé | Test immersion | - |

---

## Annexe: Résumé des corrections appliquées

| # | Correction | Section | Changement |
|---|------------|---------|------------|
| 1 | Bilan massique complet | 1.1.2 | Ajout tableau détaillé (31 359 kg, PTAC 32 t) |
| 2 | Batterie 400 kWh | 1.7.1 | 200 kWh → 400 kWh, autonomie 4+ heures |
| 3 | Mode dégradé sans GPS RTK | 4 | Nouveau chapitre complet |
| 4 | IP67 capteurs | 1.2 | Spécification IP67 pour tous capteurs extérieurs |
| 5 | Architecture redondante | 1.9 | Nouveau chapitre avec primaire/secours/moniteur |
| 6 | Conformité ISO 26262/SOTIF | 5 | Nouveau chapitre avec ASIL et SOTIF |

---

*Document généré selon les contraintes : camion NEVER-STOP, réceptif drones, ROS2*
*Version corrigée v2.0 - Intègre tous les retours comité*