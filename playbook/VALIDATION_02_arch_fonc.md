# Validation - Architecture Fonctionnelle
## Délibération du Comité d'Experts

**Document évalué :** `03-architecture-fonctionnelle.md`  
**Date :** 2026-03-31  
**Version évaluée :** 1.0

---

## Résumé Exécutif

Le comité a évalué l'architecture fonctionnelle du système multi-agents (Camion-Benne + Drones Collecteurs). Le document présente une structure complète avec 5 cas d'usage, des diagrammes de flux, et une architecture claire. Cependant, des améliorations significatives sont nécessaires sur les aspects ergonomie, coordination distribuée, et traçabilité des exigences.

**Verdict global : CONDITIONNELLEMENT ACCEPTÉ**  
Le document est accepté sous réserve de la prise en compte des recommandations critiques ci-dessous.

---

## Expert 1 : Spécialiste UX/Flux Utilisateurs

### Points Forts

| # | Point Fort | Justification |
|---|------------|----------------|
| 1 | **Format GWT systématique** | Les critères d'acceptation Given/When/Then sont cohérents et facilitent la validation utilisateur |
| 2 | **Visualisation diagrams** | Les ASCII art (architecture, machines à états, flux) sont lisibles et informatifs |
| 3 | **UC-005 Supervision Mobile** | Le cas d'usage de supervision temps réel avec seuils de latence (< 1s) montre une compréhension des besoins opérationnels |
| 4 | **Indicateurs visuels** | Mention des couleurs statut (vert/orange/rouge) pour l'état des composants |

### Points Faibles

| # | Point Faible | Impact | Priorité |
|---|--------------|--------|----------|
| 1 | **Pas de spécification des messages d'erreur** | L'opérateur ne sait pas comment le système communique les échecs (ex: "Mission échouée" sans cause) | HAUTE |
| 2 | **Application mobile passive** | UC-005 ne permet que la visualisation ; pas de contrôle direct (arrêt d'urgence, reconfiguration) | HAUTE |
| 3 | **Absence d'onboarding/aide** | Pas de guide utilisateur, pas de tooltips, pas de documentation in-app | MOYENNE |
| 4 | **Alertes non priorisées** | "< 1s latence" mentionné mais pas de gestion de priorité ou de filtrage des alertes | MOYENNE |
| 5 | **Pas d'accessibilité** | Aucune mention WCAG, contraste, navigation clavier, ou lecteur d'écran | BASSE |

### Recommandations UX

1. **Créer un catalogue de messages d'erreur** avec codes erreurs, descriptions utilisateur, et actions recommandées
2. **Enrichir l'app mobile** avec contrôles d'urgence (bouton STOP, reprise manuelle de drone)
3. **Ajouter un système d'aide contextuelle** avec guides paso à paso pour les procédures critiques
4. **Implémenter une分层 d'alertes** : Critique / Warning / Info avec options de filtrage
5. **Documenter les choix d'accessibilité** dans une spécification UI separate

---

## Expert 2 : Architecte Systèmes Multi-Agents

### Points Forts

| # | Point Fort | Justification |
|---|------------|----------------|
| 1 | **Architecture découplée par message broker** | Utilisation de Redis/RabbitMQ permet une bonne scalabilité et résistance aux pannes |
| 2 | **Machine à états bien définie** | Les diagrammes d'état (drone, système) sont clairs et facilitent le débogage |
| 3 | **Points de synchronisation identifiés** | PS-1 à PS-7 définissent les temps max pour chaque synchronisation critique |
| 4 | **Scénarios de dégradation documentés** | Le tableau section 7 couvre les principaux scénarios d'échec |

### Points Faibles

| # | Point Faible | Impact | Priorité |
|---|--------------|--------|----------|
| 1 | **Risque de deadlock sur RV simultanés** | Que se passe-t-il si 2+ drones demandent un RV simultanément avec le même camion ? Pas de mécanisme de priorité | CRITIQUE |
| 2 | **Pas de stratégie de backoff exponentiel** | En cas d'échec de calcul d'interception (section 7), juste "report mission" sans détail | HAUTE |
| 3 | **Single Point of Failure implicite** | L'orchestrateur centralisé : si FAIL, tout le système s'arrête | HAUTE |
| 4 | **Pas de mécanisme de consensus** | Pour les décisions critiques (dépôt déchet), pas de validation croisée drone-camion | MOYENNE |
| 5 | **Heartbeat timeout non standardisé** | "Timeout > 5s" pour perte com, mais pas de jitter, retry count, ou backoff | MOYENNE |
| 6 | **Pas de gestion de race condition** | Drone-truck position sync : aucune verrou ouAtomicité décrite | MOYENNE |

### Recommandations Architecture

1. **Implémenter un protocole de reservation de RV** : Le drone demande une "fenêtre" de RV avant de commencer l'interception
2. **Ajouter un orchestrateur redondant** (mode active-passive ou distribué) pour eliminer le SPOF
3. **Définir un protocole de consensus** : Le camion accuse réception (ACK) doit être cryptographiquement signé
4. **Specifier les paramètres de retry** : backoff exponentiel avec jitter, max retries, fallback sur station
5. **Ajouter un "lock" distribue** sur les ressources partagés (camion, zone de RV)

---

## Expert 3 : Expert en Ingénierie des Exigences

### Points Forts

| # | Point Fort | Justification |
|---|------------|----------------|
| 1 | **Numérotation cohérente des UC** | UC-001 à UC-005 permet une référence simple et traçabilité |
| 2 | **Matrice pré/post conditions** | La section 5.1 lie clairement conditions à chaque cas d'usage |
| 3 | **Critères de succès quantifiables** | Section 5.2 définit des seuils mesurables (> 90% taux collecte, > 99.5% disponibilité) |
| 4 | **User Stories structurées** | Format "En tant que... Je veux... Afin que..." respecté |

### Points Faibles

| # | Point Faible | Impact | Priorité |
|---|--------------|--------|----------|
| 1 | **Exigences non fonctionnelles incomplètes** | "GPS valide" sans précision (précision < 5m ?), "batterie > 30%" sans justification | HAUTE |
| 2 | **Pas de matrice de traçabilité** | Aucune liaison entre exigences de UC et tests de validation | HAUTE |
| 3 | **Absence de gestion de versions** | Le document n'a pas de système de gestion des changements (REQ-001.1, etc.) | MOYENNE |
| 4 | **Exigences implicites non documentées** | Ex: récupération après crash, audit trail, conformité réglementaire | MOYENNE |
| 5 | **Seuils arbitraires** | Pourquoi 85% confiance (UC-002) ? Pourquoi 40% batterie (UC-004) ? Pas de données de justification | MOYENNE |
| 6 | **Pas de cas limites pour UC-003** | Que se passe-t-il si le camion change de direction pendant l'interception ? | BASSE |

### Recommandations Exigences

1. **Créer une matrice de traçabilité** Requirements → UC → Tests → Validation
2. **Ajouter une section "Justification des seuils"** avec données de test ou références scientifiques
3. **Introduire un versionnage sémantique** des exigences (ex: REQ-UC001-001)
4. **Compléter les exigences non fonctionnelles** : latence réseau, bande passante,、法规 conformité
5. **Documenter les cas limites** (edge cases) pour chaque UC, particulièrement UC-003 (rendez-vous dynamique)

---

## Synthèse des Points Critiques

| Priorité | Point | Expert | Action Requise |
|----------|-------|--------|----------------|
| CRITIQUE | Risque deadlock RV simultanés | #2 | Conception protocole réservation |
| HAUTE | Messages d'erreur non specifiés | #1 | Catalogue erreurs + codes |
| HAUTE | App mobile passive | #1 | Ajouter contrôles d'urgence |
| HAUTE | SPOF Orchestrateur | #2 | Architecture redondante |
| HAUTE | Exigences non quantifiées | #3 | Spécifier seuils avec justification |
| HAUTE | Pas de matrice traçabilité | #3 | Créer matrice Requirements-Tests |

---

## Verdict Final

### Décision : **CONDITIONNELLEMENT ACCEPTÉ**

Le document est accepté pour advancement vers la phase suivante, **sous réserve** que les actions correctives suivantes soient implémentées avant la validation finale :

#### Actions Bloquantes (à résoudre avant PASSAGE)
- [ ] **Expert #2** : Résoudre le risque de deadlock sur les RV simultanés (protocole de réservation)
- [ ] **Expert #2** : Documenter la stratégie de redondance de l'orchestrateur

#### Actions Requises (pour validation finale)
- [ ] **Expert #1** : Ajouter un catalogue de messages d'erreur pour l'opérateur
- [ ] **Expert #1** : Enrichir l'app mobile avec contrôles d'urgence
- [ ] **Expert #3** : Créer la matrice de traçabilité exigences → tests
- [ ] **Expert #3** : Justifier les seuils critiques (85%, 40%, 15% batterie, etc.)

#### Suggestions d'Amélioration (optionnel)
- Accessibilité WCAG
- Guide utilisateur / onboarding in-app
- Versionnage sémantique des exigences

---

## Signatures du Comité

| Rôle | Expert | Date |
|------|--------|------|
| Spécialiste UX/Flux utilisateurs | Expert 1 | 2026-03-31 |
| Architecte systèmes multi-agents | Expert 2 | 2026-03-31 |
| Expert ingénierie des exigences | Expert 3 | 2026-03-31 |

---

*Document de délibération généré pour le playbook OpenClaw*  
*Prochaine étape : VALIDATION_03_* (à définir selon playbook)