# VALIDATION_04_drone — Délibération du Comité d'Experts

**Document évalué:** `06-drone.md` — Architecture drones collecteurs  
**Date:** 2026-03-31  
**Comité:** 3 experts indépendants

---

## Résumé Exécutif

| Expert | Domaine | Verdict |
|--------|---------|---------|
| Expert 1 | Ingénieur aérospatial/mécanique | **APPROBATION CONDITIONNELLE** |
| Expert 2 | Spécialiste vol autonome | **APPROBATION CONDITIONNELLE** |
| Expert 3 | Expert IA embarquée | **REQUÉRANCE MAJEURE** |

**Verdict global: APPROBATION CONDITIONNELLE** — Le document présente une base solide mais nécessite des corrections majeures avant implémentation.

---

## Expert 1 — Ingénieur Aérospatial / Mécanique

### Points Forts

1. **Configuration octocopter appropriée**
   - La redondance à 8 rotors offre une tolerance aux pannes acceptable pour une mission de collecte urbaine (perte de 2 moteurs survivable)
   - Le choix du direct drive simplifie la mécanique et réduit le poids

2. **Calcul de masse cohérent**
   - MTOW de 15-20 kg avec charge utile de 5-10 kg est réaliste
   - La répartition masse (frame ~2.5kg, batterie ~3kg, moteur+ESC ~1.6kg) suit les bonnes pratiques

3. **Choix battery LiPo 8S**
   - Tension de 29.6V adaptée aux moteurs 100-150 kV
   - Capacité 15-20kWh provides ~500-600 Wh, correct pour l'autonomie visée

4. **Système de préhension avec charge cells**
   - Capteur de présence et détection de résistance montrent une conception pensée sécurité

### Points Faibles

1. **Autonomie surestimée**
   - 25-30 min sans charge pour un octocopter de 9.3kg vide est **optimiste**
   - Calcul: 3200W disponibles / tension 29.6V ≈ 108A
   - Avec 500Wh et consommation ~100A en vol stationnaire = **~5 min seulement !**
   - Réel: ~12-18 min sans charge, ~8-12 min avec 10kg

2. **Moteurs sous-dimensionnés**
   - 8×400W nominal pour un drone de 15-20kg MTOW = ratio thrust/weight de ~1.7:1
   - Ce ratio est insuffisant pour une mission robuste (minimum 2:1 recommandé)
   - Les moteurs 100-150 kV à 800-1200W en pointe sont corrects en puissance unitaire, mais le total est limite

3. **Structure bras pliables non détaillée**
   - Le document mentionne "bras pliables" sans spécification du mécanisme
   - Impact sur la rigidure structurelle et les vibrations en vol non évalué

4. **Pas de calcul thermique**
   - Les moteurs brushless à 400W nominal vont générer beaucoup de chaleur
   - Pas de simulation thermique ou dissipation prévue

### Recommandations Expert 1

| Priorité | Recommandation |
|----------|----------------|
| **Majeure** | Réviser les estimateurs d'autonomie avec des mesures empiriques真实的 |
| **Majeure** | Augmenter le ratio thrust/weight à 2:1 minimum (moteurs plus puissants ou moins de rotors) |
| **Mineure** | Ajouter un système de dissipation thermique passive sur les moteurs |
| **Mineure** | Spécifier le mécanisme de bras pliables avec tolérance structurelle |

---

## Expert 2 — Spécialiste Vol Autonome

### Points Forts

1. **Architecture ROS2 cohérente**
   - Découpage en nœuds logiques bienpensé (perception, planification, contrôle)
   - Topics/Services/Actions bien définis pour une intégration propre

2. **Gestion failsafe complète**
   - Machine à états failsafe bien structurée
   - Détection des principales pannes (GPS, RC, communication, batterie)
   - Présence parachute comme dernière recurso est un +

3. **Planification trajectoire multiphase**
   - Distinction Global/Local/Approche est bonnes pratique
   - Algorithmes appropriés (A*, RRT*, DWA, MPC)

4. **Synchronisation camion intelligente**
   - Utilisation de Kalman filter pour prédiction de position
   - Concept de convergence de vitesse relative est correct

### Points Faibles

1. **Fréquence de contrôle insuffisante**
   - 100 Hz pour le contrôle est acceptable mais optimal serait 200-400 Hz
   - Pour un octocopter avec contrôle en couple, cette fréquence peut créer des oscillations

2. **Pas de mention du contrôleur de vol spécifique**
   - Le document mentionne "PX4 / ArduPilot" sans choix arrêté
   - Ces deux contrôleurs ont des différences importantes en matière de vol autonome
   - Pas de paramétrage recommandé (EKF, gains PID)

3. **Prédiction de position camion fragile**
   - Horizon de 5-10 secondes avec erreur < 50cm est **très ambitieux**
   - En milieu urbain avec traffique, la prediction est tres complexe
   - Pas de mention d'incertitude de prédiction dans la trajectoire drone

4. **Mode de vol en intérieur absent**
   - Le document mentionne Optical Flow mais sans détails d'intégration
   - Transition GPS → Optical Flow non spécifiée

5. **Docking sur camion en mouvement non détaillé assez**
   - Le document décrit le concept mais pas les marges de sécurité
   - "Précision < 10cm" est irréaliste pour un drone en mouvement
   - Pas de test de convergence ou de protocole de Abort

### Recommandations Expert 2

| Priorité | Recommandation |
|----------|----------------|
| **Majeure** | Choisir définitivement PX4 ou ArduPilot et documenter le paramétrage |
| **Majeure** | Ajouter une phase de validation en simulation avec Gazebo |
| **Majeure** | Réduire les attentes de précision de docking (< 30cm plus réaliste) |
| **Mineure** | Augmenter la fréquence de contrôle à 200 Hz |
| **Mineure** | Ajouter un mode "Hold" robuste avec estimation d'incertitude |

---

## Expert 3 — Expert en Détection / IA Embarquée

### Points Forts

1. **Choix matériel performant**
   - Jetson Orin Nano est appropriate pour YOLOv8 en temps réel
   - TensorRT permet d'atteindre les 100 FPS visés

2. **Pipeline de détection cohérent**
   - Chaîne Debayering → Normalisation → YOLO → NMS → Tracking est standard et correct
   - Utilisation de ByteTrack pour le tracking est un bon choix

3. **Fusion multi-capteurs**
   - Caméra + LiDAR + GPS RTK offre une perception complète
   - Architecture permettant la localisation 3D des objets détectés

4. **Architecture de communication**
   - Bridge 5G/WiFi/LoRa/RC montre une conception redondante
   - Protocoles appropriés (UDP, MQTT, ROS2 bridge)

### Points Faibles

1. **Consommation énergétique irréaliste**
   - Jetson Orin Nano en inference continue: ~15-40W
   - LiDAR: ~5-10W
   - Caméra 4K: ~2-5W
   - Total perception: **~25-55W** — cela représente 5-10% de la capacité batterai seulement pour la perception!
   - Le document ne tient pas compte de cette consommation dans ses calculs d'autonomie

2. **Traitement sur le drone vs Edge**
   - Tout le traitement (YOLO, tracking, fusion) est fait onboard
   - En conditions réelles, la latence et la chaleur vont créer des problèmes
   - Pas de mention de "Edge computing" déporté ou de collaboration swarm

3. **Modèle non spécifié**
   - "YOLOv8l-obb entraîné sur Sacs/déchets" est mentionné mais non fourni
   - Pas de dataset, pas de métriques de performance (mAP, recall)
   - La détection de sacs hétérogènes est complexe — le modèle doit être très robuste

4. **Profondeur et localisation 3D**
   - "Depth + LiDAR fusion" est mentionné mais non détaillé
   - Pas de calibration extrinsèque entre caméra et LiDAR
   - Erreur de localisation 3D non quantifiée

5. **Gestion thermique du compute**
   - Jetson Orin Nano a besoin de refroidissement actif
   - Pas de dissipateur, pas de ventilateur mentionné
   - En plein soleil, le GPU va throttle et réduire les performances

6. **Efficacité énergétique du modèle**
   - Pas de mention de quantification (INT8 vs FP16)
   - Pas de optimisation pour mobile/embedded
   - Un modèle full precision va consommer bien plus

### Recommandations Expert 3

| Priorité | Recommandation |
|----------|----------------|
| **Majeure** | Refaire les calculs d'autonomie EN INTÉGRANT la consommation perception (~50W) |
| **Majeure** | Quantifier le modèle en INT8 pour réduire consommation et chaleur |
| **Majeure** | Développer et tester le modèle de détection avec dataset réel |
| **Mineure** | Ajouter un système de refroidissement passif/actif pour le Jetson |
| **Mineure** | Prévoir une option de offload partiel vers la base via 5G (traitement lourd) |
| **Mineure** | Documenter la calibration extrinsèque caméra-LiDAR |

---

## Synthèse des Recommandations Critiques

### Actions Bloquantes (avant implémentation)

1. **Corriger les calculs d'autonomie** — ils sont faux et dangerously optimistes
   - Avec perception (~50W), batterie de 500Wh donne ~10 min max en vol actif
   - Solutions: batterie plus grande, réduction consommation, ou mission plus courte

2. **Augmenter le ratio thrust/weight** — 1.7:1 est insuffisant
   - Passer à 2:1 minimum pour marge de sécurité

3. **Valider le modèle de détection** — il n'existe pas actuellement
   - Entraîner et tester sur des données réelles de sacs/déchets

4. **Choisir et paramétrer le contrôleur de vol** — PX4 ou ArduPilot
   - Documenter la configuration complète

### Actions Importantes

5. Ajouter un système de refroidissement pour le Jetson Orin
6. Réduire les attentes de précision de docking (< 30cm plus réaliste)
7. Détailler le mécanisme de bras pliables
8. Ajouter une phase de validation en simulation

---

## Verdict Final

**APPROBATION CONDITIONNELLE**

Le document présente une **architecture conceptuelle solide** avec des fondations correctes (ROS2, redondance, failsafe). Cependant, les **erreurs de calcul critiques** (autonomie, consommation) et les **lacunes de détail** (modèle IA, paramétrage FC) rendent une implémentation directe risquée.

### Score par critère

| Critère | Score (1-5) |
|---------|-------------|
| Cohérence globale | 4/5 |
| Réalisme des spécifications | 2/5 |
| Complétude technique | 3/5 |
| Détection d'erreurs | 2/5 |
| Prêt pour implémentation | 2/5 |

### Prochaine étape recommandée

Retour aux auteurs pour correction des points bloquants, puis nouvelle revue.

---

*Document délibéré par le comité d'experts — ValidATION_04_drone.md*