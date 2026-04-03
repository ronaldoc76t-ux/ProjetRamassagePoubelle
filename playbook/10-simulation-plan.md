# Plan de simulation

## Story
- En tant que responsable test, je veux un plan de simulation complet pour valider chaque module et cas de failure.
- Critère d’acceptation : simulateur, scénarios, métriques, rendez-vous dynamiques et intégration ROS2.

## Choix simulateur
- Gazebo ROS2 (Ignition)
- Étape de validation: Gazebo -> tests hardware-in-loop -> terrain réel

## Modèles 3D
- Camion-benne autonome
- Drone collecteur
- Sac / bac déchets
- Environnement urbain (routes, intersections, chantiers)

## Scénarios
- Normal (trafic léger/moyen)
- Trafic dense + obstacles mobiles
- Perte GNSS, conditions adverses
- Panne drone, panne de communication

## Rendez-vous dynamiques
- Simulation trajectoire camion + rendez-vous drones
- Latence réseau artificielle et dérive

## Échecs
- collision, dépassement temps, fil complet, échec trémie

## Métriques
- taux succès rendez-vous
- marge temps et position
- consommation énergie
- latence end-to-end

## Intégration ROS2
- launch files, ros2 bag tests, tests CI
