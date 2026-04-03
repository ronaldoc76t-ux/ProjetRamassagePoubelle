# Diagramme Mermaid haut niveau

## Story
- En tant que stakeholder, je veux un diagramme visuel synthétique du système pour comprendre rapidement l’architecture.
- Critère d’acceptation : diagramme Mermaid complet avec flux entre camion, drones, backend et mobile.

```mermaid
graph TD
  subgraph Opérations
    Camion[Camion autonome] -->|DDS/5G| Drones[Flotte drones collecteurs]
    Drones -->|DDS/5G| Camion
  end
  subgraph Cloud
    Backend[Backend Cloud]
    Orchestrator[Orchestrateur Multi-Agents]
    Backend --> Orchestrator
    Drones -->|gRPC/MQTT| Backend
    Camion -->|gRPC/MQTT| Backend
  end
  subgraph Mobile
    App[Application mobile]
    App -->|REST/gRPC| Backend
  end
```
