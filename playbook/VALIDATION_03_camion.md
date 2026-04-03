# VALIDATION_03 — Camion-Benne Autonome

## Délibération du Comité d'Experts

**Document source :** `05-camion.md`  
**Date de délibération :** 2026-03-31  
**Contexte :** Architecture détaillée du camion-benne autonome à mouvement continu avec réception de drones en vol

---

## Résumé Exécutif

Le présent document évalue l'architecture technique du camion-benne autonome conçu pour opérer en mouvement continu tout en réceptionnant des drones en vol. Cette évaluation mobilise trois expertises complémentaires : ingénierie mécanique véVICulaire, électronique embarquée, et navigation autonome.

L'analyse révèle une conception ambitieuse et globalement cohérente, avec quelques points de vigilance nécessitant des clarifications ou améliorations avant validation finale.

---

## Vue d'Ensemble des Points Clés

| Dimension | Score Global | Verdict |
|-----------|-------------|---------|
| Mécanique & Transmission | **8/10** | ✓ Validé sous conditions |
| Électronique Embarquée | **7.5/10** | ✓ Validé avec réserves |
| Navigation Autonome | **8.5/10** | ✓ Validated |

---

# Expert 1 — Ingénieur Mécanique VéVICulaire

## Domaines évalués : Châsis, Transmission, Fiabilité, Intégration Mécanique

### Points Forts

1. **Choix de la base véhicule bien adapté**
   - Le choix d'un camion 6x4 ou 8x4 de 26-32 tonnes offre une capacité de charge utile suffisante (≈ 10-15 tonnes) pour intégrer les systèmes additionnels tout en maintenant une charge utile commerciale acceptable.
   - L'empattement de 4 200 à 5 000 mm procure une stabilité longitudinale adecuada pour la réception de charges suspendues.

2. **Approche de renforcement结构el cohérente**
   - L'objectif de +15% de rigidité torsionnelle est réaliste et atteignable par un caisson aluminium/acier HSLA arrière.
   - La réduction de garde au sol de 80 mm (plutôt que les 50 mm mentionnés) contribue significativement à l'abaissement du centre de gravité, critique pour la stabilité dynamique.

3. **Système de plateforme stabilisée bien conçu**
   - L'architecture à 2 degrés de liberté (tangage + roulis) avec vérins hydrauliques est appropriée.
   - Les spécifications de précision (±0.5°) et de temps de réponse (< 50ms) sont cohérentes avec les besoins de réception de charges en mouvement.
   - La fréquence de contrôle de 200 Hz assure une bonne réactivité.

4. **Transmission adaptée au mode never-stop**
   - Le choix d'une boîte automatique Allison ou robotisée est pertinent pour éviter l'embrayage et permettre une conduite continue.
   - La motorisation électrique ou hybride permet le Start-Stop impossible — c'est une excellente décision architecturale.

5. **Système de compactage correctement dimensionné**
   - Le compacteur vibrant de 40 000 N est adequat pour une benne de 15-20 m³.
   - L'ajustement de fréquence (30-50 Hz) permet une adaptation au type de déchets.

### Points Faibles et Préoccupations

1. **Réduction excessive de garde au sol**
   - Une réduction de 80 mm peut compromettre l'angle d'approche et de thérapeutage,limitant l'accès à certains sites de chargement.
   - **Recommandation :** Maintenir une garde au sol minimale de 250 mm et compenser par un lestage adapté ou des suspensions actives à faible hauteur.

2. **Intégration de la trémie arrière**
   - L'arrière "ouvert/berceau" avec structure anti-effraction IP67 crée un défi d'étanchéité significatif lors de la circulation sur voies publiques (poussière, eau).
   - **Recommandation :** Prévoir des joints lèvres dynamiques et un système de drainage actif.

3. **Charge additionnelle non quantifiée**
   - Le document mentionne +120 mm de hauteur pour la plateforme mais ne cuantifie pas le poids total ajouté (capteurs, vérins, structure, batteris).
   - **Risque :** Dépassement du PTAC (Poids Total Autorisé en Charge) pourrait réduire la charge utile commerciale sous le seuil économique.
   - **Recommandation :** Réaliser un bilan massique détaillé avec marge de 10%.

4. **Système hydraulique de plateforme stabilisée**
   - Une alimentation hydraulique indépendante de 10 kW représente une source de défaillance additionnelle.
   - **Recommandation :** Prévoir une pompe électrique de secours ou un accumulateur nitrogen permettant au moins 5 cycles de stabilisation après perte de puissance.

5. **Transmission — manque de détail sur le rapport de pont**
   - Pour une vitesse de croisière de 30-80 km/h, le rapport de pont doit être optimisé pour minimiser la consommation énergétique tout en maintenant la capacité de accélération.
   - **Recommandation :** Specifier les rapports de pont et vérifier la compatibilité avec les profils d'autonomie.

### Recommandations Techniques

| # | Recommandation | Priorité |
|---|----------------|----------|
| R1.1 | Réaliser un bilan massique complet avec marge de 10% | Haute |
| R1.2 | Maintenir garde au sol ≥ 250 mm et compenser par suspensions | Haute |
| R1.3 | Ajouter accumulateur de secours pour stabilisation hydraulique | Moyenne |
| R1.4 | Specifier rapports de pont pour optimisation énergétique | Moyenne |
| R1.5 | Concevoir système de drainage actif pour trémie arrière | Moyenne |

---

# Expert 2 — Spécialiste Électronique Embarquée

## Domaines évalués : Capteurs, ECU, Contraintes Automotive, Architecture Électrique

### Points Forts

1. **Architecture capteurs complète et redondante**
   - La combinaison LiDAR 360° (Alpha Prime) + LiDAR compact + LiDARs solides latéraux offre une couverture360° avec redondance.
   - Le radar longue portée (Continental ARS 540) est particulièrement pertinent pour l'ACC et la détection à longue distance.
   - L'inclusion de caméras thermiques (FLIR Boson) est excellente pour la détection de piétons/animations et la vérification de température des déchets.

2. **Couverture CAN FD cohérente**
   - L'architecture avec VCU comme nóeud central CAN FD est standard et facilitant l'intégration.
   - Les interfaces topic `/control/*` vers ROS2 sont bien définies.

3. **Gestion BMS correctement paramétrée**
   - Les seuils SOC (20% avertissement, 10% critique) et température (60°C) sont appropriés pour des batteries LiFePO4.
   - La tension min/max (320V/420V) est cohérente avec un pack 400V.

4. **Architecture électrique avec backup**
   - La présence d'un UPS avec backup capacitaire + batterie est essentielle pour la sécurité.
   - La séparation batteries traction (400V) et auxiliaire (24V) est une bonne pratique.

5. **Redondance des systèmes critiques**
   - Direction et freinage disposent de systèmes de secours (hydraulique).
   - La double borniere E-Stop (cabine + extérieur) est appropriée.

### Points Faibles et Préocupações

1. **Consommation électrique élevée**
   - Le total de ~170 kW moyen et 450 kW en pointe est très élevé pour un véhicule électrique/hybride.
   - Avec 200 kWh de batteris de traction, l'autonomie serait de seulement ~1h à puissance moyenne.
   - **Recommandation :** Augmenter la capacité de batteri tractio à minimum 400 kWh pour une autonomie de 4-6 heures.

2. **Exposition des capteurs aux environnements difficiles**
   - Les capteurs montée sur le toit (LiDAR 360°, caméra) seront exposés aux éléments (pluie, poussière, soleil direct).
   - Les spécifications IP ne sont pas mentionnées pour les capteurs.
   - **Recommandation :** Specifier IP67 minimum pour tout composant extérieur et prévoir un système de nettoyage automatique (wiper, air comprimé).

3. **Architecture de calculateur non détaillée**
   - Le document mentionne "Calculateur Linux x86-64" mais ne detaille pas :
     - Nombre d'unités et leur rôle
     - Mode de défaillance (single point of failure?)
     - Température de fonctionnement
   - **Recommandation :** Définir une architecture avec calculateurs redondants ou fail-over, et spécifier les contraintes temperature (-40°C à +85°C automotive).

4. **Câblage et EMV**
   - Le câblage d'un véhicule avec ~15 capteurs + actionneurs + batteris est complexe et non détaillé.
   - **Risque :** Perturbations EMV entre les différents sous-systèmes (particulièrement entre RF (radar) et CAN).
   - **Recommandation :** Prévoir un harnais blindé et une stratégie de mise à la masse.

5. **Communication drone en environnement EMV**
   - Les linkes de communication drone (probablement 2.4 GHz ou 5.8 GHz) peuvent être perturbées par les systèmes électroniques du camion.
   - **Recommandation :** Positionner l'antenne de communication drone en hauteur et à l'écart des sources d'interférence.

6. **Intégration ROS2 sur plateforme automotive**
   - Les messages ROS2 ne sont pas nativement temps réel.
   - **Recommandation :** Prévoir une couche middleware temps réel (RTOS ou Xenomai) pour les boucles de contrôle critiques (50-200 Hz).

### Recommandations Techniques

| # | Recommandation | Priorité |
|---|----------------|----------|
| R2.1 | Augmenter capacité batteri traction à 400 kWh | Haute |
| R2.2 | Specifier IP67 pour tous capteurs extérieurs | Haute |
| R2.3 | Définir architecture calculateurs redondants | Haute |
| R2.4 | Prévoir système nettoyage capteurs automatiques | Moyenne |
| R2.5 | Concevoir câblage blindé avec stratégie EMV | Moyenne |
| R2.6 | Vérifier compatibilité ROS2 avec contraintes temps réel | Moyenne |

---

# Expert 3 — Expert Navigation Autonome

## Domaines évalués : Localisation, SLAM, Planification, Sécurité, Gestion des Risques

### Points Forts

1. **Architecture ROS2 bien structurée**
   - Le graphe de nodes est cohérent avec une separation claire des responsabilités.
   - Les topics/services/actions sont correctement définis avec les fréquences appropriées.
   - L'utilisation de `robot_localization` pour EKF est un bon choix industriel.

2. **Stratégie de navigation never-stop cohérente**
   - L'approche de vitesse variable (80 km/h → 20-40 km/h → 10-25 km/h) est bien pensée pour maintenir le mouvement tout en permettant la réception drone.
   - La machine à états est correctement définie.

3. **Prédiction de trajectoire pertinente**
   - L'horizon de 30-120 secondes avec résolution 1s est approprie pour la communication drone.
   - La combinaison physics-based + ML (LSTM) est un bon compromis entre robustesse et précision.

4. **Gestion des risques multi-niveaux**
   - Les 4 niveaux de réponse (avertissement → ralentissement → arrêt doux → urgence) sont bien gradués.
   - Les temps de réponse (< 50ms à < 300ms) sont cohérents avec les exigences de sécurité.

5. **Protocole de rendez-vous drone bien conçu**
   - L'échange de trajectories avant l'approche est essentiels.
   - Les états machine (`IDLE` → `APPROACHING` → `SYNCING` → `DEPOSITING`) sont logiques.

### Points Faibles et Préoccupations

1. **Dépendance au GPS RTK**
   - Le document repose fortement sur GPS RTK pour la localisation centimétrique.
   - **Risque :** Perte de signal GPS (tunnel, ville dense, interference) = perte de localisation précise.
   - **Recommandation :** Préciser le mode de fonctionnement en mode dégradé (estimation odométrique uniquement) et ses performances attendues.

2. **Fusion de capteurs non détaillée**
   - Le document mentionne "perception" mais ne detaille pas l'algorithme de fusion (simple addition? fusion probabiliste? MBF?).
   - **Recommandation :** Définir explicitement l'architecture de fusion (par exemple : LiDAR + Caméra + Radar → Objets trackés via MOT).

3. **SLAM non-addressé**
   - Le document suppose une carte HD préexistante ("/map").
   - **Question :** Comment le camion gère-t-il les routes non cartographiées ou les modifications de tracé?
   - **Recommandation :** Prévoir un mode SLAM en ligne ou un téléchargement de cartes depuis le backend.

4. **Validation de la sécurité fonctionnelle**
   - Le document ne fait pas référence à des normes de sécurité (ISO 26262, ISO 13849, SOTIF).
   - Le concept de "fail operational" vs "fail safe" n'est pas explicité.
   - **Recommandation :** Définir le SIL (Safety Integrity Level) cible pour chaque fonction critique et documenter l'architecture de sécurité.

5. **Trajectoire prédite non validée**
   - L'algorithme de prédiction n'est pas décrit en détail.
   - **Risque :** Une prédiction inexacte pourrait causer des collisions drone-camion.
   - **Recommandation :** Prévoir une validation expérimentale et des marges de sécurité dans le calcul des zones de rendez-vous.

6. **Communication avec le drone**
   - Le protocole est bien défini mais ne mentionne pas la latence tolerable.
   - **Question :** Quel est le plan si la communication échoue à moins de 20m?
   - **Recommandation :** Définir un protocole de repli (drone retourne à base, truck continue).

### Recommandations Techniques

| # | Recommandation | Priorité |
|---|----------------|----------|
| R3.1 | Définir mode dégradé GPS (odométrie seule) et performances associées | Haute |
| R3.2 | Documenter architecture de fusion multi-capteurs | Haute |
| R3.3 | Ajouter capacités SLAM en ligne ou téléchargement cartes | Moyenne |
| R3.4 | Référencer normes ISO 26262/SOTIF et définir SIL cibles | Haute |
| R3.5 | Valider expérimentalement l'algorithme de prédiction | Haute |
| R3.6 | Définir protocole de repli en cas de perte communication drone | Moyenne |

---

# Synthèse et Verdict Final

## Tableau Récapitulatif des Points Forts

| Expert | Point Fort Majeur |
|--------|-------------------|
| Mécanique | Architecture plateforme stabilisée cohérente et bien dimensionnée |
| Électronique | Couverture capteurs complète avec redondance |
| Navigation | Architecture ROS2 mature et gestion des risques bien gradée |

## Tableau Récapitulatif des Points Faibles

| Expert | Point Faible Majeur | Impact |
|--------|---------------------|--------|
| Mécanique | Bilan massique non réalisé | Charge utile insuffisante |
| Électronique | Autonomie batteri insuffisante (200 kWh) | Temps de fonctionnement < 1h |
| Navigation | Dépendance GPS RTK sans mode dégradé | Perte de localisation |

## Verdict

### **APPROUVÉ SOUS CONDITIONS**

Le comité d'experts approuve l'architecture générale du camion-benne autonome sous réserve de la mise en oeuvre des corrections suivantes avant la phase d'implémentation :

**Conditions blockantes (à résoudre avant-projet) :**

1. **R1.1** — Réaliser le bilan massique complet et vérifier le PTAC
2. **R2.1** — Augmenter la capacité batteri traction à minimum 400 kWh
3. **R3.1** — Définir et documenter le mode dégradé sans GPS

**Conditions recommandées (à intégrer au projet) :**

4. **R2.2** — Spécifier IP67 pour tous capteurs extérieurs + système nettoyage
5. **R2.3** — Définir architecture calculateurs redondants
6. **R3.4** — Documenter la conformité ISO 26262/SOTIF et définir SIL

### Score Final

| Critère | Score |
|---------|-------|
| Complétude technique | 8/10 |
| Cohérence architecturale | 8/10 |
| Fiabilité et sécurité | 7/10 |
| Faisabilité économique | 6.5/10 |
| **Note finale** | **7.5/10** |

---

## Signatures du Comité

- **Expert 1** — [_Signature_]  
- **Expert 2** — [_Signature_]  
- **Expert 3** — [_Signature_]

---

*Document généré le 2026-03-31*