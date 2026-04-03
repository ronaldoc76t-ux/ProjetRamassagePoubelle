# Plan de Simulation - Système Camion-Benne Autonome + Drones Collecteurs

> **Version corrigée** selon retours comité  
> Date: 2026-03-31  
> Contenu additionnel: Oracles de test, reproductibilité, Digital Twin, fidelity, métriques performance simulation

---

## 1. Choix du Simulateur

### Recommandation : **Gazebo Sim (Ignition Fortress ou Harmonic)**

| Critère | Gazebo Sim | Webots | Isaac Sim |
|---------|------------|--------|-----------|
| Support ROS2 natif | ✅ Excellent | ✅ Bon | ✅ Excellent |
| Modèles 3D UAV/UGV | ✅ large communauté | ✅ Moyen | ✅ (NVIDIA) |
| Physique multi-rotors | ✅ Adapté | ✅ Adapté | ✅ (physX) |
| Coût | Libre | Libre | Propriétaire |
| Intégration outdoor | ✅ Excellent | ⚠️ Limité | ✅ Excellent |
| **Déterminisme** | ✅ Configurable | ⚠️ Limité | ✅ Excellent |
| **Fidelity plugins** | ✅ Fine-grained | ⚠️ Moyen | ✅ Excellent |

### Justification

**Gazebo Sim** est choisi pour :
- **ROS2 intégré natif** : ros_gz_bridge, ignition-transport, SDFormat
- **Grande bibliothèque** : modèles URDF/SDF pour drones (PX4, ArduPilot) et véhicules
- **Outdoor robuste** : heightmap, illumination dynamique, météo
- **Communauté active** : maintenance continue, documentation abondante
- **Hardware-in-loop** : compatibilité avec Pixhawk, CAN, ESC
- **Contrôle déterministe** : possibilité de configurer step, seeds, physics solver

**Version recommandée** : Gazebo Harmonic (ou Fortress pour stability)

---

## 2. Modèles 3D Nécessaires

### 2.1 Camion-benne Autonome

| Élément | Description | Format | Fidelity |
|---------|-------------|--------|----------|
| Châassis | Modèle générique benne 6-8m | URDF + meshes STL/DAE | Medium (visuel) → High (physics) |
| Roues | 4-6 roues avec physics | SDF joints | **High**: friction, slip, suspension |
| Benne | Trémie basculante, capacité 2-4m³ | Link articulé | **High**: dynamique, inertia |
| Capteurs | LiDAR, caméra, GNSS, IMU | Plugins Gazebo | **High**: bruit, latence configurable |
| Capot radar |隐藏式 radar360° | Visual only | Low |

**Plugins requis** :
- `ros_gz_sim` pour synchronisation temps réel
- `gz-sensors` : LiDAR, cameras, GPS, IMU
- `gz-transport` : topics ROS2

### 2.2 Drone Collecteur

| Élément | Description | Fidelity |
|---------|-------------|----------|
| Frame | Quadrirotor 400-600mm | **Medium-High**: masse, CG |
| Moteurs + ESC | Modèle physique avec thrust curves | **High**: PWM→thrust lookup |
| Batterie | Modèle énergétique (capacity, discharge) | **Medium**: tension dynamique |
| Capteurs | GPS, IMU, caméra downwards, LiDAR | **High**: bruit, drift |
| Bras de préhension | Mechanism servo-controlled | **High**: cinématique |
| Lumière | LED status (battery, link) | Low |

**Modèles de référence** :
- Iris+ (PX4) : `/usr/share/gz-sim/models/iris`
- Custom SDF basé sur rotor_s

### 2.3 Objets et Environnement

| Catégorie | Objets | Fidelity |
|-----------|--------|----------|
| Déchets | Sacs poubelle, bacs roll, conteneurs | **Low-Medium**: collision only |
| Infrastructure | Routes, intersections, panneaux | **Medium**: visuel |
| Obstacles | Véhicules, piétons (personnes), cônes | **Medium**: dynamique |
| Terrain | Heightmap, routes asphaltées | **High**: friction map |

**Environnement** :
- Ville simulée (quartier industriel)
- Routes avec marquages
- Zones de dépôt désignées

---

## 3. Spécification de Fidelity par Composant

### 3.1 Matrice de Fidelity

| Composant | Low | Medium | High | Notre Cible |
|-----------|-----|--------|------|-------------|
| **Physique véhicule** | Kinématique seul | Masse+inertie | Friction/slip/suspension | **High** |
| **Physique drone** | Masse constante | Thrust曲线 | Aerodynamics drag | **High** |
| **LiDAR** | Pas de bruit | Bruit Gaussien | Bruit + occlusion | **High** |
| **Camera** | Pas de 处理 | Distortion | Noise + motion blur | **Medium** |
| **IMU** | Parfait | Bruit Gaussien | Bias drift | **High** |
| **GNSS** | Parfait | Noise UTM | Multi-path, outage | **High** |
| **Communication** | Parfaite | Packet loss | Latence+jitter+loss | **High** |
| **Météo** | Désactivée | Facteur unique | Multi-paramètre | **Medium** |

### 3.2 Configuration par Scénario

```yaml
# config/fidelity/rdv_nominal.yaml
fidelity:
  physics: high
  sensors:
    lidar:
      noise_type: gaussian
      noise_stddev: 0.02
      update_rate: 10  # Hz
    imu:
      bias_stochastic: true
      gyro_noise: 0.002  # rad/s
      accel_noise: 0.05  # m/s^2
    gnss:
      enabled: true
      jitter: 0.5  # m
      outage_probability: 0.01
  communication:
    latency_mean: 50  # ms
    latency_stddev: 10  # ms
    packet_loss: 0.001
  environment:
    wind_enabled: true
    wind_speed: 0-5  # m/s
    visibility: good  # good/moderate/poor
```

---

## 4. Reproductibilité et Déterminisme

### 4.1 Gestion des Seeds

```yaml
# config/simulation_seeds.yaml
seeds:
  # Seeds globaux pour reproductibilité
  global_seed: 42
  
  # Seeds par composant
  physics_engine:
    seed: 1001
    solver_iterations: 50
  
  sensors:
    lidar_noise: 2001
    imu_noise: 2002
    gnss_noise: 2003
    
  environment:
    wind_seed: 3001
    traffic_seed: 3002
    
  random_events:
    fault_injection: 4001
    comm_outage: 4002
```

### 4.2 Configuration Déterministe

```python
# simulation_deterministic.py
import random
import numpy as np

class DeterministicSimulation:
    def __init__(self, global_seed: int):
        self.global_seed = global_seed
        self._setup_seeds()
        
    def _setup_seeds(self):
        """Configure tous les générateurs pour déterminisme"""
        random.seed(self.global_seed)
        np.random.seed(self.global_seed)
        
        # Gazebo physics
        gz_params = {
            'physics.seed': self.global_seed,
            'physics.solver.iterations': 50,
            'physics.solver.type': 'PGS',  # Projected Gauss-Seidel
            'physics.solver.precontact': True,
        }
        
        # ROS2 timer determinism
        rclpy.init args = ['--ros-args', '-p', f'sim_global_seed:={self.global_seed}']
        
    def reset(self):
        """Reset complet pour replay"""
        self._setup_seeds()
        # Reset Gazebo
        # Reset ROS2 state
        # Reset fault injector
```

### 4.3 Scénarios Répétables

```bash
# Lancement avec seed explicite
ros2 launch rdv_bringup simulation.launch.py \
    global_seed:=42 \
    scenario:=T-01 \
    fault_injection:=false

# Vérification reproductibilité
ros2 param get /gz_sim physics.seed  # Doit retourner 42
ros2 param get /simulation_manager global_seed  # Doit retourner 42
```

### 4.4 Enregistrement et Replay

| Élément | Description | Stockage |
|---------|-------------|----------|
| Seed configuration | YAML avec tous les seeds | `seeds/run_XXX.yaml` |
| Gazebo state | SDF snapshot initial | `states/init_XXX.sdf` |
| Events timeline | JSON avec événements | `events/run_XXX.json` |
| ROS2 bag | Topics complets | `bags/run_XXX/` |

```bash
# Enregistrement
ros2 bag record -a -o /data/simulation_$(date +%Y%m%d_%H%M%S)

# Replay déterministe
ros2 bag play /data/simulation_run_001/ \
    --clock \
    --rate 1.0 \
    --start-offset 0
```

---

## 5. Oracles de Test et Critères de Validation

### 5.1 Définition des Oracles

Un **oracle de test** est une règle permettant de déterminer automatiquement si un test passe ou échoue.

| Oracle ID | Description | Critère de Passage |
|-----------|-------------|-------------------|
| **OR-RDV-01** | Le drone atteint la zone de RDV | `distance(drone, truck) < 0.5m` pendant ≥ 3s |
| **OR-RDV-02** | Le RDV se complète dans le temps imparti | `t_rdv < t_max = 60s` |
| **OR-RDV-03** | Pas de collision durant le RDV | `collision_count == 0` |
| **OR-NAV-01** | Le camion suit le chemin planifié | `deviation < 0.3m` en permanence |
| **OR-NAV-02** | Le drone évite les obstacles | `min_obstacle_distance > 0.3m` |
| **OR-COMM-01** | Communication fonctionnelle | `latence < 100ms`, `packet_loss < 5%` |
| **OR-BATT-01** | Batterie suffisante pour mission | `battery_end > 20%` |
| **OR-FAIL-01** | Détection d'échec correcte | `failure_detected == true` dans 5s |
| **OR-FAIL-02** | Récupération après échecs | `return_to_nominal == true` dans 30s |

### 5.2 Oracles de Performance

| Oracle ID | Métrique | Seuil Warning | Seuil Critique |
|-----------|----------|---------------|----------------|
| **OR-PERF-01** | FPS simulation | < 45 | < 30 |
| **OR-PERF-02** | Latence step sim | > 50ms | > 100ms |
| **OR-PERF-03** | Latence ROS2 | > 20ms | > 50ms |
| **OR-PERF-04** | Mémoire utilisée | > 80% RAM | > 95% RAM |

### 5.3 Implémentation Oracles

```python
# oracles/test_oracle.py
from dataclasses import dataclass
from enum import Enum
from typing import Callable, Any

class OracleResult(Enum):
    PASS = "pass"
    WARNING = "warning"
    FAIL = "fail"
    SKIP = "skip"

@dataclass
class Oracle:
    id: str
    name: str
    check_fn: Callable[[dict], OracleResult]
    description: str
    severity: str  # critical, major, minor

# Example oracle implementation
def oracle_rdv_success(state: dict) -> OracleResult:
    """
    OR-RDV-01: Le drone atteint la zone de RDV
    Critère: distance < 0.5m pendant >= 3s
    """
    distance = state['distance_drone_truck']
    duration = state['rdv_approach_duration']
    
    if distance < 0.5 and duration >= 3.0:
        return OracleResult.PASS
    elif distance < 1.0:
        return OracleResult.WARNING
    else:
        return OracleResult.FAIL

RDV_SUCCESS_ORACLE = Oracle(
    id="OR-RDV-01",
    name="RDV Position Achieved",
    check_fn=oracle_rdv_success,
    description="Drone reaches RDV zone within tolerance",
    severity="critical"
)
```

### 5.4 Test Runner avec Oracles

```python
# oracles/test_runner.py
import json
from pathlib import Path
from oracles import ORACLE_REGISTRY

class SimulationTestRunner:
    def __init__(self, scenario: str, seeds: dict):
        self.scenario = scenario
        self.seeds = seeds
        self.results = []
        
    def run(self) -> dict:
        """Exécute un scénario et évalue tous les oracles"""
        self._setup_simulation()
        self._execute_scenario()
        self._evaluate_oracles()
        return self._generate_report()
    
    def _evaluate_oracles(self):
        """Évalue chaque oracle enregistré"""
        final_state = self._get_final_state()
        
        for oracle in ORACLE_REGISTRY:
            result = oracle.check_fn(final_state)
            self.results.append({
                'oracle_id': oracle.id,
                'result': result.value,
                'details': final_state
            })
    
    def _generate_report(self) -> dict:
        """Génère le rapport de test"""
        passed = sum(1 for r in self.results if r['result'] == 'pass')
        failed = sum(1 for r in self.results if r['result'] == 'fail')
        
        return {
            'scenario': self.scenario,
            'seeds': self.seeds,
            'total_oracles': len(self.results),
            'passed': passed,
            'failed': failed,
            'pass_rate': passed / len(self.results),
            'details': self.results
        }
```

---

## 6. Use Cases Digital Twin

### 6.1 Définition Digital Twin

Le **Digital Twin** dans notre contexte est une réplique virtuelle synchronisée avec le système réel permettant :
- Monitoring en temps réel
- Simulation prédictive
- Test de scénarios sans risque
- Entraînement ML/RL

### 6.2 Use Case 1: Monitoring Temps Réel

```
┌─────────────────────────────────────────────────────────────┐
│  USE CASE 1: Monitoring Temps Réel                          │
├─────────────────────────────────────────────────────────────┤
│  SCÉNARIO: Surveillance opérationnelle du système réel      │
│                                                             │
│  COMPOSANTS:                                                 │
│  ├── Camion réel (physique)                                 │
│  ├── Drones réels (physique)                                │
│  ├── Digital Twin (simulation Gazebo)                        │
│  └── Pont de données (ros_gz_bridge)                        │
│                                                             │
│  FLUX:                                                       │
│  1. Capteurs réels → topics ROS2                            │
│  2. ros_gz_bridge → topics Gazebo                           │
│  3. Digital Twin update state                                │
│  4. Visualisation / Alerting                                │
│                                                             │
│  FRÉQUENCE: 10-50 Hz                                        │
│  LATENCE MAX: 100ms                                         │
│  FIDÉLITÉ: Medium (visualisation seule)                     │
└─────────────────────────────────────────────────────────────┘
```

### 6.3 Use Case 2: Simulation Prédictive

```
┌─────────────────────────────────────────────────────────────┐
│  USE CASE 2: Simulation Prédictive                          │
├─────────────────────────────────────────────────────────────┤
│  SCÉNARIO: Prédiction de trajectoire et planification        │
│                                                             │
│  COMPOSANTS:                                                 │
│  ├── État actuel (Digital Twin)                             │
│  ├── Modèle prédictif (simulation rapide)                   │
│  └── Interface planification                                │
│                                                             │
│  FLUX:                                                       │
│  1. État courant → input simulation                         │
│  2. Simulation rapide (100x réel)                           │
│  3. Prédiction trajectoire future                           │
│  4. Décision path planning                                  │
│                                                             │
│  OBJECTIF: Prédire碰撞/ééchec RDV avant occurrence          │
│  HORIZON: 30-60 secondes                                    │
│  FIDÉLITÉ: Medium-High                                      │
└─────────────────────────────────────────────────────────────┘
```

### 6.4 Use Case 3: What-If Analysis

```
┌─────────────────────────────────────────────────────────────┐
│  USE CASE 3: What-If Analysis                               │
├─────────────────────────────────────────────────────────────┤
│  SCÉNARIO: Évaluation de scénarios hypothétiques           │
│                                                             │
│  QUESTIONS TYPiques:                                        │
│  ├── "Que se passe-t-il si le GNSS échoue à 500m du RDV?"  │
│  ├── "Quelle trajectoire alternative si obstacle détecté?" │
│  ├── "Comment le système réagit-il à une perte de com?"    │
│                                                             │
│  FLUX:                                                       │
│  1. Snapshot état actuel                                   │
│  2. Injection scénario what-if                             │
│  3. Simulation déterministe                                 │
│  4. Analyse résultats                                      │
│                                                             │
│  UTILISATEUR: Opérateur, Ingénieur, ML                     │
│  FIDÉLITÉ: High                                             │
└─────────────────────────────────────────────────────────────┘
```

### 6.5 Use Case 4: Entraînement et Validation

```
┌─────────────────────────────────────────────────────────────┐
│  USE CASE 4: Entraînement / Validation                      │
├─────────────────────────────────────────────────────────────┤
│  SCÉNARIO: Entraînement RL/ML et validation algorithmes    │
│                                                             │
│  FLUX:                                                       │
│  ├── Entraînement: Simulation → Modèle RL → Reward          │
│  ├── Validation: Scénarios réels → Digital Twin → Benchmark │
│  └── Regression:新旧 algorithmes comparés                  │
│                                                             │
│  DONNÉES:                                                   │
│  ├──Millions de steps simulation (rapide)                  │
│  ├──数百 de scénarios réels (validation)                    │
│  └──Métriques de performance comparées                      │
│                                                             │
│  FIDÉLITÉ: Variable (basse pour entraînement, haute pour validation)│
└─────────────────────────────────────────────────────────────┘
```

### 6.6 Tableau Comparatif Digital Twin

| Use Case | Mode Sync | Latence | Fidelity | Application |
|----------|-----------|---------|----------|-------------|
| Monitoring | Real-time | < 100ms | Low-Medium | Dashboard |
| Prédictif | Periodic | < 1s | Medium-High | Path planning |
| What-If | On-demand | < 10s | High | Analyse |
| Entraînement | Batch | N/A | Low-High | ML/RL |

---

## 7. Métriques de Performance Simulation

### 7.1 Métriques FPS et Latence

| Métrique | Description | Cible | Warning | Critique |
|----------|-------------|-------|---------|----------|
| **Simulation FPS** | Images/seconde simu | ≥ 60 | < 45 | < 30 |
| **Simulation Time** | Temps simulé / temps réel | 1.0x | < 0.8x | < 0.5x |
| **Step Duration** | Durée d'un pas de simulation | < 10ms | > 20ms | > 50ms |
| **Physics Step** | Durée pas physique | < 5ms | > 10ms | > 20ms |
| **Render Time** | Durée renduframe | < 15ms | > 25ms | > 40ms |

### 7.2 Métriques ROS2

| Métrique | Description | Cible | Warning | Critique |
|----------|-------------|-------|---------|----------|
| **Topic Latency** | Latence end-to-end | < 10ms | > 20ms | > 50ms |
| **Node Latency** | Latence traitement node | < 5ms | > 10ms | > 25ms |
| **Callback Duration** | Durée callbacks | < 2ms | > 5ms | > 10ms |
| **Subscription Depth** | Profondeur queue souscriptions | < 10 | > 50 | > 100 |

### 7.3 Métriques Réseau (Communication)

| Métrique | Description | Cible | Warning | Critique |
|----------|-------------|-------|---------|----------|
| **Packet Loss** | Paquets perdus | < 1% | > 3% | > 10% |
| **Jitter** | Variance latence | < 5ms | > 15ms | > 30ms |
| **Bandwidth** | Bande passante utilisée | < 10 Mbps | > 50 Mbps | > 100 Mbps |
| **Reconnection Time** | Temps reconnexion | < 1s | > 3s | > 10s |

### 7.4 Métriques Système

| Métrique | Description | Cible | Warning | Critique |
|----------|-------------|-------|---------|----------|
| **CPU Usage** | Utilisation CPU | < 70% | > 80% | > 95% |
| **RAM Usage** | Utilisation mémoire | < 60% | > 80% | > 95% |
| **GPU Usage** | Utilisation GPU | < 80% | > 90% | > 98% |
| **Disk I/O** | Lecture/écriture disk | < 50 MB/s | > 100 MB/s | > 200 MB/s |

### 7.5 Collecte et Affichage

```python
# metrics/simulation_metrics.py
import time
import psutil
from collections import deque
from rclpy.node import Node

class SimulationMetrics(Node):
    def __init__(self):
        super().__init__('simulation_metrics')
        
        # Publishers
        self.perf_pub = self.create_publisher PerfMetrics
        self.sys_pub = self.create_publisher SystemMetrics
        
        # Timers
        self.create_timer(1.0, self._collect_performance)
        self.create_timer(5.0, self._collect_system)
        
        # Buffers for sliding averages
        self.fps_buffer = deque(maxlen=60)
        self.step_buffer = deque(maxlen=60)
        
    def _collect_performance(self):
        """Collecte métriques performance simulation"""
        msg = PerfMetrics()
        
        # Simulation FPS (via Gazebo)
        msg.fps = self.get_gz_fps()
        msg.sim_time_ratio = self.get_sim_time_ratio()
        msg.step_duration_ms = self.get_step_duration()
        
        # ROS2 latencies
        msg.topic_latency_ms = self.measure_topic_latency('/truck/pose')
        msg.node_latency_ms = self.measure_node_latency('/rdv_coordinator')
        
        self.perf_pub.publish(msg)
        
    def _collect_system(self):
        """Collecte métriques système"""
        msg = SystemMetrics()
        
        msg.cpu_percent = psutil.cpu_percent()
        msg.ram_percent = psutil.virtual_memory().percent
        msg.gpu_percent = self.get_gpu_usage()  # GPUtil or similar
        msg.disk_io_mbps = psutil.disk_io_counters()._asdict()
        
        self.sys_pub.publish(msg)
        
        # Check thresholds and alert
        self._check_thresholds(msg)
```

```yaml
# Configuration seuils métriques
metrics_thresholds:
  performance:
    simulation_fps:
      warning: 45
      critical: 30
    step_duration_ms:
      warning: 20
      critical: 50
  system:
    cpu_percent:
      warning: 80
      critical: 95
    ram_percent:
      warning: 80
      critical: 95
```

---

## 8. Scénarios de Test

### 8.1 Scénarios Nominaux

| ID | Scénario | Objectif | Oracles Associés |
|----|----------|----------|------------------|
| T-01 | Trafic léger | Validation navigation nominale | OR-NAV-01, OR-RDV-01 |
| T-02 | Trafic moyen | Comportement multi-agents | OR-NAV-01, OR-NAV-02 |
| T-03 | Trafic dense + obstacles | Navigation réactive | OR-NAV-02, OR-RDV-01 |

### 8.2 Scénarios Degrés

| ID | Scénario | Condition adverses | Oracles Associés |
|----|----------|-------------------|------------------|
| D-01 | Perte GNSS | GPS denied, localisation estimée | OR-NAV-01, OR-FAIL-01 |
| D-02 | Météo adverse | Pluie, visibilité réduite | OR-NAV-02, OR-RDV-02 |
| D-03 | Nuit / faible luminosité | Éclairage artificiel | OR-NAV-02 |
| D-04 | Panne moteur drone | Atterrissage d'urgence | OR-FAIL-01, OR-FAIL-02 |

### 8.3 Scénarios Communication

| ID | Scénario | Mode | Oracles Associés |
|----|----------|------|------------------|
| C-01 | Perte link radio | Timeout > 5s | OR-COMM-01, OR-FAIL-01 |
| C-02 | Latence réseau | Delay 100-500ms simulé | OR-COMM-01, OR-RDV-02 |
| C-03 | Interférence | Noise injection | OR-COMM-01 |

### 8.4 Scénarios Digital Twin

| ID | Use Case | Description |
|----|----------|-------------|
| DT-01 | Monitoring | Synchronisation réel→simulé temps réel |
| DT-02 | Prédictif | Simulation 100x pour prédiction collision |
| DT-03 | What-If | Test scénarios sans impact système réel |
| DT-04 | Entraînement | Batch simulation pour RL |

---

## 9. Simulation des Rendez-vous Dynamiques

### 9.1 Concept

Le **rendez-vous dynamique** (RDV) est la manœuvre où un drone rejoint le camion en mouvement pour :
- Déposer les déchets collectés
- Reprendre une nouvelle mission
- Recharger (optionnel)

### 9.2 Modélisation

```
┌─────────────────────────────────────────────────────────────┐
│  Processus RDV Dynamique                                   │
├─────────────────────────────────────────────────────────────┤
│  1. Camion → broadcast position (topic: /truck/pose)       │
│  2. Drone → calcul trajectoire intercept                    │
│  3. Accord RDV : Truck.speed ≈ Drone.speed                 │
│  4. Approche : distance < 3m                                │
│  5. Raccord : bras préhension → benne                      │
│  6. Dépôt / Reprise                                         │
│  7. Séparation                                              │
└─────────────────────────────────────────────────────────────┘
```

### 9.3 Paramètres de Simulation

| Paramètre | Valeur typique | Plage test |
|-----------|----------------|------------|
| Vitesse camion | 10-30 km/h | 5-50 km/h |
| Vitesse drone | 5-15 m/s | 2-20 m/s |
| Altitude RDV | 3-5 m | 2-10 m |
| Distance latérale | 0-2 m | 0-5 m |
| Durée manœuvre | 10-30 s | 5-60 s |

### 9.4 Défauts Injectés

- **Dérive temporelle** : clock skew entre nodes
- **Perte de positionnement** : drone utilise position dead-reckoning
- **Marge d'erreur** : tolérance position < 0.5m

---

## 10. Simulation des Échecs

### 10.1 Types d'Échec

| Catégorie | Échec | Déclencheur | Comportement |
|-----------|-------|-------------|--------------|
| Communication | Perte signal | topicmute, network cutoff | Drone land on last known position |
| Batterie | Seuil critique | battery < 15% | Return to home, abandon RDV |
| Collision | Détection obstacle | LiDAR proximity < 0.3m | Brake + hover / land |
| Timing | Timeout RDV | t_rdv > t_max (60s) | Abort, retry later |
| Mécanique | Échec trémie | Benne jam sensor | Warning + skip |

### 10.2 Scénarios d'Échec Injectés

| ID | Échec | Sévérité | Oracle Impacté |
|----|-------|----------|-----------------|
| F-01 | Perte radio 30s | Critique | OR-RDV-01, OR-FAIL-01 |
| F-02 | Batterie drone 10% | Majeur | OR-BATT-01 |
| F-03 | Collision évitement | Majeur | OR-NAV-02 |
| F-04 | Trémie bloquée | Mineur | OR-RDV-02 |

### 10.3 Injection de Défauts (Fault Injection)

```python
# Exemple: Fault injection node ROS2
import rclpy
from rclpy.node import Node

class FaultInjector(Node):
    def __init__(self, seed: int = None):
        super().__init__('fault_injector')
        self.seed = seed or int(time.time())
        self.rng = random.Random(self.seed)
        self.create_timer(1.0, self.inject_faults)
    
    def inject_faults(self):
        # Simuler perte signal
        if self.fault_mode == 'signal_loss':
            self.publish_nothing()  # Silence
        # Simuler batterie faible
        elif self.fault_mode == 'low_battery':
            self.publish_battery(10)  # 10%
```

---

## 11. Métriques de Performance (Métiers)

### 11.1 Métriques Principales

| Métrique | Description | Seuil succès |
|----------|-------------|--------------|
| **Taux succès RDV** | % RDV réussis / total | ≥ 95% |
| **Marge position RDV** | Erreur position | < 0.5 m |
| **Marge temporelle RDV** | Délai vs estimé | < 5 s |
| **Temps mission moyen** | Durée cycle complet | Référence -10% |
| **Consommation énergie** | Wh/km (drone) | < référence |

### 11.2 Métriques Secondaires

| Métrique | Description |
|----------|-------------|
| Latence communication | End-to-end /truck → /drone |
| Nombre de collisions | Évitements manqués |
| Temps de récupération | Retour à état nominal |
| Disponibilité système | Uptime / mission |

---

## 12. Intégration ROS2

### 12.1 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ROS2 SIMULATION STACK                   │
├─────────────────────────────────────────────────────────────┤
│  Nodes:                                                     │
│  ├── /truck_controller     (cmd_vel, feedback)              │
│  ├── /drone_navigator      (local planner)                  │
│  ├── /rdv_coordinator      (state machine)                 │
│  ├── /fault_injector       (simulation fault)               │
│  ├── /metrics_collector    (logging)                        │
│  └── /oracle_evaluator     (test validation)                │
│                                                             │
│  Topics:                                                    │
│  ├── /truck/pose            (tf, odometry)                  │
│  ├── /drone/pose           (tf, odometry)                   │
│  ├── /rdv/command          (GoTo, Approach, Dock)          │
│  ├── /rdv/status           (Ready, InProgress, Success)     │
│  ├── /metrics/performance  (FPS, latence)                  │
│  ├── /metrics/oracles      (oracle results)                │
│  └── /digital_twin/sync    (state sync)                    │
└─────────────────────────────────────────────────────────────┘
```

### 12.2 Packages ROS2

| Package | Rôle |
|---------|------|
| `truck_bringup` | Launch truck, sensors, controllers |
| `drone_bringup` | Launch drone, navigator |
| `rdv_coordinator` | State machine, RDV logic |
| `simulation_utils` | Gazebo plugins, fault injection |
| `metrics_logger` | ros2 bag, prometheus exporter |
| `test_oracles` | Oracle definitions and evaluation |

### 12.3 Fichiers de Launch avec Seeds

```python
# rdv_simulation.launch.py
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    # Arguments configurables
    global_seed_arg = DeclareLaunchArgument(
        'global_seed',
        default_value='42',
        description='Global seed for deterministic simulation'
    )
    scenario_arg = DeclareLaunchArgument(
        'scenario',
        default_value='T-01',
        description='Test scenario to run'
    )
    fidelity_arg = DeclareLaunchArgument(
        'fidelity',
        default_value='high',
        description='Fidelity level: low, medium, high'
    )
    
    return LaunchDescription([
        global_seed_arg,
        scenario_arg,
        fidelity_arg,
        
        # Gazebo avec seeds
        IncludeLaunchDescription(
            GazeboSimulationLaunch(),
            launch_arguments={
                'gz_args': '-r empty.sdf',
                'physics.seed': LaunchConfiguration('global_seed')
            }.items()
        ),
        
        # Paramètres globaux
        Node(
            package='simulation_utils',
            executable='set_seeds',
            parameters=[{
                'global_seed': LaunchConfiguration('global_seed'),
                'physics_seed': LaunchConfiguration('global_seed') + 1000,
                'sensors_seed': LaunchConfiguration('global_seed') + 2000,
                'environment_seed': LaunchConfiguration('global_seed') + 3000,
            }]
        ),
        
        # Truck
        Node(
            package='truck_bringup',
            executable='truck_node',
            output='screen'
        ),
        
        # Drone
        Node(
            package='drone_bringup',
            executable='drone_node',
            output='screen'
        ),
        
        # RDV Coordinator
        Node(
            package='rdv_coordinator',
            executable='rdv_node',
            output='screen'
        ),
        
        # Oracle Evaluator
        Node(
            package='test_oracles',
            executable='oracle_evaluator',
            output='screen',
            parameters=[{
                'scenario': LaunchConfiguration('scenario'),
                'enabled_oracles': ['OR-RDV-01', 'OR-RDV-02', 'OR-NAV-01']
            }]
        ),
        
        # Metrics
        Node(
            package='metrics_logger',
            executable='metrics_node',
            output='screen'
        )
    ])
```

### 12.4 Tests CI/CD avec Oracles

```yaml
# .github/workflows/simulation.yml
name: Simulation Tests

jobs:
  simulation-test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        scenario: ['T-01', 'T-02', 'T-03', 'D-01', 'D-02', 'C-01', 'F-01']
        seed: [42, 43, 44]
    steps:
      - uses: actions/checkout@v3
      
      - name: Run simulation test
        run: |
          ros2 launch rdv_bringup rdv_simulation.launch.py \
            scenario:=${{ matrix.scenario }} \
            global_seed:=${{ matrix.seed }} \
            fidelity:=high
          
      - name: Collect metrics
        run: python3 scripts/analyze_metrics.py
        
      - name: Verify oracles
        run: python3 scripts/verify_oracles.py
        
      - name: Publish results
        run: |
          echo "## Results - ${{ matrix.scenario }} (seed ${{ matrix.seed }})" >> $GITHUB_STEP_SUMMARY
          cat results/oracle_report.json >> $GITHUB_STEP_SUMMARY
```

---

## 13. Plan de Validation

| Phase | Description | Durée | Oracles Cible |
|-------|-------------|-------|---------------|
| Phase 1 | Simulation unitaire (truck seul, drone seul) | 1 semaine | OR-NAV-01 |
| Phase 2 | RDV nominal (T-01) | 1 semaine | OR-RDV-01, OR-RDV-02 |
| Phase 3 | Scénarios dégradés (D-01, C-01) | 1 semaine | OR-FAIL-01 |
| Phase 4 | Échecs et récupération (F-01 à F-04) | 1 semaine | OR-FAIL-01, OR-FAIL-02 |
| Phase 5 | Trafic dense (T-03) + intégration | 1 semaine | OR-NAV-02 |
| Phase 6 | Digital Twin validation (DT-01 à DT-04) | 1 semaine | Tous |

---

## 14. Références

- [Gazebo Sim Documentation](https://gazebosim.org/docs)
- [ROS2 Navigation](https://navigation.ros.org/)
- [PX4 Simulation](https://docs.px4.io/main/en/simulation/)
- Ignition Transport : Topics et services
- SDFormat : Modèles robotiques

---

*Document généré pour validation système camion-benne autonome + flotte drones*

**Corrections appliquées**:
1. ✅ Oracles de test documentés (Section 5)
2. ✅ Reproductibilité améliorée avec seeds (Section 4)
3. ✅ Use cases Digital Twin définis (Section 6)
4. ✅ Fidelity spécifiée par composant (Section 3)
5. ✅ Métriques de performance simulation ajoutées (Section 7)