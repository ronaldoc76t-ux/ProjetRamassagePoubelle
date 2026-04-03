# Architecture technologique

## Story
- En tant qu’ingénieur Tech Lead, je veux un choix technologique validé pour aligner l’infrastructure et les outils.
- Critère d’acceptation : stack, protocoles, sécurité, temps réel et monitoring précisés.

## Choix techniques
- ROS2 Humble (LTS, support jusqu'en 2027)
- DDS (FastRTPS / CycloneDDS)
- Simulateur: Gazebo Harmonic (latest LTS)
- Backend: Kubernetes, Kafka, PostgreSQL, Redis
- Mobile: Flutter / React Native
- API: gRPC + REST + MQTT

## Communication
- Camion–Drones: DDS ROS2
- Camion/Drones–Backend: gRPC/MQTT (télémétrie)
- Mobile–Backend: REST/gRPC

## Synchronisation
- Protocole rendez-vous (RendezvousOffer / RendezvousConfirm)
- Prédiction 30-120s (EKF+LSTM)

## Sécurité
- Chiffrement E2E
- mTLS + JWT + ACL
- HSM, IAM

## Temps réel
- Linux PREEMPT_RT embarqué
- QoS ROS2 (reliable, transient_local)

## Logs / télémétrie
- Edge -> cache -> bulk upload
- ELK et Prometheus/Grafana
