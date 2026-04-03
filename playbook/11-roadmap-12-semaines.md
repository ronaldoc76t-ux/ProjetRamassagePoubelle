# Roadmap 12 semaines

## Story
- En tant que planificateur projet, je veux une roadmap détaillée de 12 semaines pour organiser les sprints.
- Critère d’acceptation : phases, jalons, risques, ressources et livrables définis.

## Phases
1. **S1-S2** : design détaillé, spécifications, architecture.
   - Livrables: specs complètes, ADR décisions
   - Stories: "En tant qu'archi, je veux valider la stack"
2. **S3-S4** : PoC Camion ROS2 et prédicteur trajectoire.
   - Livrables: nav_node fonctionnel, trajectory predictor
   - Stories: "En tant que truck, je peux naviguer en continu"
3. **S5-S6** : PoC Drone ROS2 et contrôle docking.
   - Livrables: drone nodes, docking controller
   - Stories: "En tant que drone, je peux détecter et collecter"
4. **S7** : Backend minimal + API.
   - Livrables: REST API, PostgreSQL schema, JWT auth
   - Stories: "En tant qu'API, je expose les données fleet"
5. **S8** : Orchestrateur et premiers tests intégrés.
   - Livrables: mission orchestrator, Kafka topics
   - Stories: "En tant qu'orchestrateur, j assigne les missions"
6. **S9** : Simulation complète + optimisation.
   - Livrables: Gazebo world, tests RV dynamiques
   - Stories: "En tant que système, je validé le RV à 50km/h"
7. **S10** : Application mobile MVP.
   - Livrables: MVP mobile (dashboard + map)
   - Stories: "En tant qu'user, je vois la position des drones"
8. **S11** : Tests fiabilité, sécurité, performance.
   - Livrables: Test report, pen test, benchmark
   - Stories: "En tant que QA, je valide les perfs"
9. **S12** : Documentation, validation finale, déploiement pré-prod.
   - Livrables: Doc technique, démo M4, go-live pré-prod

## Jalons
- M1 : architecture validée
- M2 : prototype Camion + Drone fonctionnel
- M3 : orchestration et orchestrateur en place
- M4 : démo end-to-end

## Risques & dépendances
- réseau -> tests multiréseaux
- capteurs -> redondance
- évolutions réglementaires -> veille

## Ressources
- équipe robotique, software ROS2, cloud, sécurité, dev mobile

## Équipe (5-8 personnes)
| Rôle |Nb| Responsibilities |
|------|---|------------------|
| Tech Lead / PM | 1 | Coordination, roadmap, stakeholders |
| Robotic Engineer | 2 | Camion + Drone ROS2 |
| Backend Dev | 2 | API, Kafka, orchestration |
| Mobile Dev | 1 | App iOS/Android |
| DevOps | 1 | K8s, CI/CD, monitoring |
| QA/Test | 1 | Tests, simulation |

## Budget Estimé
- MVP: €80,000 - €120,000
- Include: hardware drones/camion, cloud, licenses,外包

## Definition of Done (DoD)
- Code review passé
- Tests unitaires > 80% coverage
- Documentation technique
- CI/CD pipeline vert
- Démo fonctionnelle validée par PO

## Stratégie de Redondance Orchestrateur
- Mode **active-passive** avec heartbeat
- Si primary fail → secondary prend en charge en < 5s
- DNS switch ou IP failover automatique
