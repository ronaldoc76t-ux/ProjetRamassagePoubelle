# Playbook opérationnel — Prompt maître

Ce document synthétise l'architecture et la stratégie globale du projet camion-benne autonome + drones collecteurs.

## Story
- En tant que chef de projet, je veux une vue maîtresse du système pour aligner les équipes sur l’architecture globale, les contraintes, et les livrables.
- Critère d’acceptation : document validé, architecture découpée en composants, et plan étape par étape disponible.

## Mission
- Concevoir un système intégré : camion autonome + flotte de drones + backend cloud + application mobile + orchestrateur multi-agents.
- Contraintes : camion en mouvement continu, rendez-vous dynamiques drones/camion, modularité, scalabilité, sécurité, compatibilité ROS2.

## Livrables
1. Architecture haut niveau
2. Architecture détaillée par composant
3. Plan de simulation
4. Plan de développement
5. Analyse des risques et atténuation
6. Diagrammes textuels et Mermaid

## Notes opérationnelles
- Base du travail dans `prompts.md`, transformé en playbook divisé.
- Contenus de section sont répliqués dans des fichiers spécifiques.

## Plan d’action séquentiel (Playbook 01)
1. Vérifier et valider la story et les critères d’acceptation.
2. Coder les artefacts attendus : architecture haut niveau, détaillée, plan simulation, plan dev, risques, diagrammes.
3. Synchroniser avec les équipes : définir responsables de chaque livrable, échéances (sprint 0 à sprint 3 pris en charge).
4. Mettre à jour périodiquement : état d’avancement, blocages, métriques d’intégration et KPIs de mission.
5. Livraison finale : revue inter-équipes, validation métier, mise en production du pilote.
