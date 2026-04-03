# VALIDATION_08 - Délibération Plan de Simulation

## Projet : Système Camion-Benne Autonome + Drones Collecteurs

**Document source** : `10-simulation-plan.md`  
**Date évaluation** : 2026-03-31  
**Type** : Revue multi-experts

---

## Résumé Exécutif

Le plan de simulation proposé présente une base solide pour la validation du système autonome camion-benne + drones. Cependant, des lacunes significatives existent concernant la fidelity du Digital Twin, la reproductibilité des tests, et certains aspects techniques de l'intégration ROS2/Gazebo. Le verdict global est **CONDITIONNEL AVEC RECOMMANDATIONS**.

---

## Analyse par Expert

### 🧪 EXPERT 1 : Utilisateur Gazebo/Webots

**Domaine** : Fidelity, ROS2 Integration, Performance

#### Points Forts

| Aspect | Évaluation |
|--------|-------------|
| **Choix simulateur** | Gazebo Sim est pertinent pour ROS2 natif et scénario outdoor |
| **Modèles 3D** | Approche URDF/SDF bien définie, références aux modèles PX4 |
| **Plugins** | Liste complète : ros_gz_bridge, gz-sensors, gz-transport |
| **Architecture ROS2** | Structure de nodes claire et cohérente |

#### Points Faibles

| Aspect | Problème | Impact |
|--------|----------|--------|
| **Version Gazebo** | "Harmonic (ou Fortress)" — indécision | Risque compatibilité |
| **Performance** | Pas de benchmark, pas d'optimisation temps réel | Incertitude sur réalisme |
| **Fidelity physique** | Pas de mention modèle de friction roues, thrust curves détaillées | Simulation may be too idéalisée |
| **Capteurs** | Juste "LiDAR, caméra" — pas de spécification modèle réel (Velodyne, RealSense?) | Gap hardware réel |
| **Outdoor** | "Heightmap, météo" — pas de configuration détaillée | Environment trop simple |

#### Recommandations

1. **Verrouiller version Gazebo** : Privilégier Fortress (stable) ou Harmonic avec test de compatibilité ros_gz
2. **Ajouter profil de performance** : Spécifier target FPS (30-60), latency simulation
3. **Détailler modèles capteurs** : Préciser marques/modèles réels pour améliorer fidelity
4. **Configuration météo** : Définir paramètres pluie/visibilité pour scénarios D-02
5. **Hardware-in-loop** : Documenter compatibilité Pixhawk/CAN dès le départ

---

### 📊 EXPERT 2 : Spécialiste Validation/Test

**Domaine** : Couverture, Métriques, Reproductibilité

#### Points Forts

| Aspect | Évaluation |
|--------|-------------|
| **Couverture scénarios** | Bonne liste : nominaux (T-01 à T-03), dégradés (D-01 à D-04), communication (C-01 à C-03), échecs (F-01 à F-04) |
| **Métriques principales** | Taux succès RDV, marge position/temporelle, consommation — alignés avec objectifs |
| **Infrastructure test** | CI/CD YAML, ros2 bag — bonnes pratiques |
| **Phases validation** | Plan structuré en 5 phases sur 5 semaines |

#### Points Faibles

| Aspect | Problème | Impact |
|--------|----------|--------|
| **Reproductibilité** | Pas de graine aléatoire (seed), pas de déterminisme | Tests non reproductibles |
| **Couverture边界** | Pas de test aux limites (vitesse extreme, distance RDV max) | Zones grises non testées |
| **Métriques secondaire** | "Latence communication" mentionné mais pas de seuil | Validation subjective |
| **Oracles de test** | Pas de définition claire de "succès" vs "échec" pour chaque scénario | Ambiguïté interprétation |
| **Statistical significance** | Nombre d'itérations par scénario non défini | Résultats non statistiquement valides |
| **Données de référence** | Pas de baseline comparaison | Pas de mesure d'amélioration |

#### Recommandations

1. **Déterminisme** : Utiliser `--seed` dans Gazebo, fixer random seed pour chaque test
2. **Oracle de test** : Définir critères de pass/fail explicites pour chaque scénario (ex: RDV succès = position error < 0.5m ET time < 60s)
3. **Itérations** : Minimum 30 runs par scénario nominal pour significance statistique
4. **Seuil latence** : Définir latence max acceptable (ex: < 100ms)
5. **Baseline** : Exécuter tests sans code nouveau pour établir référence
6. **Couverture boundary** : Ajouter tests aux limites (vitesse 50 km/h, altitude 10m, distance 5m)

---

### 🔗 EXPERT 3 : Expert Digital Twin

**Domaine** : Sync Réel/Simulé, Fidelity, Use Cases

#### Points Forts

| Aspect | Évaluation |
|--------|-------------|
| **Concept RDV** | Modélisation processus dynamique bien détaillée |
| **State machine** | Structure coordinator avec états définis |
| **Intégration ROS2** | Topics bien définis pour sync |

#### Points Faibles

| Aspect | Problème | Impact |
|--------|----------|--------|
| **Digital Twin fidelity** | ZERO mention de calibrage avec système réel | Twin ne reflète pas réalité |
| **Sync mechanism** | Pas de stratégie real-time sync (clock, data) | Drift non géré |
| **Use cases** | "Use case" non définis pour le Digital Twin | Pas de valeur métier |
| **Twin architecture** | Pas de mention cloud/edge, juste simulation locale | Limite utility |
| **Data exchange** | Pas de processus pour importer données réelles dans sim | Cycle broken |
| **Validation against reality** | Aucune méthode de comparaison sim vs réel | Twin non validé |

#### Recommandations

1. **Calibration pipeline** : Documenter comment les paramètres réels (masse camion, thrust drone) sont intégrés dans la sim
2. **Sync strategy** : Implémenter NTP/PTP pour clock sync, ros_gz_bridge en mode heartbeat
3. **Define use cases** :
   - UC1: Validation logicielle avant déploiement
   - UC2: Entraînement opérateurs
   - UC3: Analyse incidents (replay données réelles)
4. **Twin architecture** : Définir si cloud-hosted ou local, avec API d'échange
5. **Validation sim-réel** : Méthode pour comparer comportement simulé vs mesuré (ex: distance RDV réelle vs simulée)
6. **Digital Twin KPIs** :
   - Sync latency < 100ms
   - Fidelity score > 90% (comparaison positions)
   - Uptime twin > 99%

---

## Tableau Synthétique

| Critère | Expert 1 | Expert 2 | Expert 3 |
|---------|----------|----------|----------|
| **Fidelity** | ⚠️ Modérée | ❌ Non traitée | ❌ Insuffisante |
| **ROS2 Integration** | ✅ Bonne | N/A | ⚠️ Partielle |
| **Couverture tests** | N/A | ⚠️ Bonne avec lacunes | N/A |
| **Métriques** | ⚠️ Partielles | ✅ Bonnes | ⚠️ Non alignées DT |
| **Reproductibilité** | ❌ Non mentionnée | ❌ Insuffisante | N/A |
| **Use Cases** | N/A | N/A | ❌ Manquants |

---

## Verdict Final

### 🟡 **CONDITIONNEL — REQUIERT CORRECTIFS**

#### Justification

Le plan est techniquement solide pour une première itération de simulation, mais présente des lacunes critiques qui compromettent :
1. La **reproductibilité** des résultats (Expert 2)
2. La **fidelity** du Digital Twin (Expert 3)
3. La **performance** documentée du simulateur (Expert 1)

### Priorités de Correction

| Priorité | Correctif | Expert |
|----------|-----------|--------|
| 🔴 P1 | Définir oracle de test et critères pass/fail | Exp 2 |
| 🔴 P2 | Ajouter seed aléatoire et déterminisme | Exp 2 |
| 🔴 P3 | Définir use cases Digital Twin | Exp 3 |
| 🟡 P4 | Verrouiller version Gazebo + benchmark performance | Exp 1 |
| 🟡 P5 | Ajouter calibration pipeline sim→réel | Exp 3 |
| 🟢 P6 | Configurations météo détaillées | Exp 1 |
| 🟢 P7 | Métriques secondaires avec seuils | Exp 2 |

### Prochaine Étape

Avant validation finale, le plan doit intégrer :
- **Annexe A** : Oracle de test avec seuils quantifiés
- **Annexe B** : Use cases Digital Twin avec architecture
- **Annexe C** : Pipeline de calibration et validation sim-réel

---

*Document préparé par comité d'experts simulation robotique*