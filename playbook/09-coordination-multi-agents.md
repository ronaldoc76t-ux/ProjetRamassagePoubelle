# Coordination multi-agents

## Story
- En tant qu’ingénieur système multi-agent, je veux un design de coordination pour que camion/drone/backend soient synchronisés.
- Critère d’acceptation : architecture d’agents, protocole, gestion échecs et fenêtres rendez-vous.

## Structure
- AgentCamion
- AgentDrone
- AgentOrchestrateur

## Algorithmes
- hybridation Auction-based + MPC
- prédiction position camion (EKF + LSTM)
- assignation mission drones + optimisation fenêtre
- gestion conflit et deadlock

## Protocole de communication
- ROS2 topics dédiés (/rendezvous_offer, /rendezvous_plan)
- REST/gRPC de backup

## Échecs
- detection, replan, reroute, release slot
- rejets multiples -> escalade OP

## Fenêtres de rendez-vous
- granularité 1 s / tolérance ±3 s
- profil margin +/- 2 m
