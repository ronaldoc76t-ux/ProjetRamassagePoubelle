# Comité Coordination Multi-Agents & Simulation - Délibération

## Date: 2026-04-01

## Documents revus
- 09-coordination-multi-agents.md (811 octets)
- 10-simulation-plan.md (1039 octets)
- 12-mermaid-diagram.md (706 octets)

---

## Points validés ✅

### Document 09
- Architecture 3 agents (Camion, Drone, Orchestrateur)
- Algorithme hybridation Auction-based + MPC pertinent
- Protocole de communication ROS2 défini
- Gestion des échecs présente

### Document 10
- Choix Gazebo pertinent pour simulation ROS2
- Scénarios de test complets (normal, dense, panne)
- Métriques pertinentes (taux succès, latence, énergie)

### Document 12
- Diagrammes Mermaid présents et cohérents avec l'architecture

---

## Points à corriger ⚠️

### Document 09
1. **Détails MPC manquants** - "MPC" mentionné mais pas de horizon, cost function
2. **Auction-based vague** - Comment fonctionne l'enchère? Qui enchérit sur quoi?
3. **Fenêtre rendez-vous tordue** - "granularité 1s / tolérance ±3s" mais quelle fenêtre totale?
4. **Pas de détails protocole** - Format des messages ROS2?
5. **Gestion deadlock manquante** - Comment on détecte et résout?

### Document 10
1. **Version Gazebo non spécifiée** - Fortress? Garden? Harmonic?
2. **Modèles 3D non listés** - "Camion-benne autonome" etc mais pas de sources
3. **Pas de CI/CD** - "tests CI" mentionné mais pipeline non détaillé
4. **Benchmark targets manquants** - Latence cible? Taux succès minimum?

### Document 12
1. **Un seul diagramme** - Devrait avoir diagramme de séquence, architecture déploiement
2. **Légendes manquantes** - Symboles non expliqués

---

## Décisions

| Point | Décision | Action |
|-------|-----------|--------|
| 09: Ajouter détails MPC | Définir horizon (10-30s), cost function | Corriger 09 |
| 10: Spécifier version | Gazebo Harmonic (latest LTS) | Corriger 10 |
| 10: Ajouter seuils | Latence <100ms, succès >95% | Corriger 10 |
| 12: Enrichir diagrammes | Ajouter séquence + déploiement | Corriger 12 |

---

## Résumé
- **Validé avec corrections** pour 09, 10, 12
- Documents corrects mais manquent de précision technique