# Architecture Fonctionnelle Complète

## Système Multi-Agents : Camion-Benne Autonome + Drones Collecteurs

### Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        PLATEFORME CENTRALISÉE                                │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐       │
│  │   BACKEND CLOUD │◄──►│  ORCHESTRATEUR  │◄──►│   BASE DE DONNÉES│       │
│  │   (API REST)    │    │  (Mission Mgmt) │    │   (MongoDB)     │       │
│  └────────┬────────┘    └────────┬────────┘    └─────────────────┘       │
│           │                      │                                         │
│           │         ┌────────────┴────────────┐                            │
│           │         │   MESSAGE BROKER       │                            │
│           │         │   (Redis/RabbitMQ)    │                            │
│           └─────────┼────────────┬───────────┘                            │
│                     │            │                                        │
└─────────────────────┼────────────┼────────────────────────────────────────┘
                      │            │
         ┌────────────┴───┐    ┌───┴────────────┐
         │  APP MOBILE    │    │  CAMION-BENNE  │
         │  (Supervision) │    │  (En mouvement)│
         └────────────────┘    └───────┬────────┘
                                        │
                              ┌─────────┴─────────┐
                              │   FLOTTE DRONES   │
                              │   (N drones)      │
                              └───────────────────┘
```

---

## 1. Cas d'Usage Détaillés

### 1.1 UC-001 : Lancement de Mission

**Description** : L'opérateur initie une mission de collecte depuis le backend ou l'application mobile.

**Acteurs** : Opérateur, Backend, Orchestrateur

**User Story** :
```
En tant qu'opérateur,
Je veux lancer une mission de collecte de déchets,
Afin que le système orchestré commence les opérations de ramassage.
```

**Critères d'Acceptance (GWT)** :

| Condition | Description |
|-----------|-------------|
| **GIVEN** | Le système est opérationnel, GPS valide, tous les composants connectés |
| **WHEN** | L'opérateur appuie sur "Démarrer Mission" avec zone définie |
| **THEN** | Mission créée avec ID unique, status = "PLANIFIEE" |
| **AND** | Orchestrateur notifié, drones assignés, camion notifié du trajet |
| **AND** | Confirmation affichée sur mobile dans les 3 secondes |

**Flux** :
```
Opérateur → [Démarrer Mission] → Backend → Orchestrateur
Orchestrateur → [Assigner Drones] → Flotte
Orchestrateur → [Calculer Trajet] → Camion
Backend → [Retour Confirmation] → App Mobile
```

---

### 1.2 UC-002 : Ramassage Déchets

**Description** : Un drone détecte, approche et collecte un déchet.

**Acteurs** : Drone, Capteurs, Orchestrateur, Backend

**User Story** :
```
En tant que drone,
Je veux détecter et collecter un déchet identifié,
Afin de le transporter vers le point de rendez-vous avec le camion.
```

**Critères d'Acceptance** :

| Condition | Description |
|-----------|-------------|
| **GIVEN** | Drone en vol,mission active, zone de collecte définie |
| **WHEN** | Capteur détecte déchet (vision/thermique), confiance > 85% |
| **THEN** | Déchet tracké, drone approche, bras collecteur activé |
| **AND** | Déchet confirmé collecté, status = "COLLECTE" |
| **AND** | Position enregistrée, NOTIF à l'orchestrateur |

**Machine à états Drone** :
```
┌─────────┐   detection    ┌─────────┐   approche    ┌─────────┐
│  EN_VOL │ ─────────────► │ DETECTE │ ────────────► │COLLECTE │
└─────────┘                └─────────┘                └────┬────┘
      │                                               │
      │ échoue                                       │ déposé
      │                                    ┌─────────┴─────────┐
      └───────────────────────────────────►│   EN_ROUTE_RV   │
                                            └──────────────────┘
```

---

### 1.3 UC-003 : Rendez-Vous Dynamique (Critical)

**Description** : Le drone exécute un rendez-vous dynamique avec le camion en mouvement.

**Acteurs** : Drone, Camion, Orchestrateur

**Contrainte** : Le camion NE S'ARRÊTE JAMAIS. Le drone doit calculer une interception.

**User Story** :
```
En tant que drone,
Je veux effectuer un rendu de déchet au camion sans qu'il s'arrête,
Afin de maintenir la continuité opérationnelle du système.
```

**Critères d'Acceptance** :

| Condition | Description |
|-----------|-------------|
| **GIVEN** | Drone a un déchet, camion en mouvement, connexion établie |
| **WHEN** | Distance < 50m et vitesse relative compatible |
| **THEN** | Calcul interception en temps réel (prediction trajectoire) |
| **AND** | Drone同步 à la vitesse du camion |
| **AND** | Dépôt sécurisé, confirmation timestampée |
| **AND** | Camion accuse réception sans arrêter |

**Algorithme de Rendez-Vous** :
```
┌────────────────────────────────────────────────────────────────┐
│                   CALCUL INTERCEPTION                          │
│                                                                │
│  Entrées:                                                     │
│    - PositionCamion(t), VecteurVitesseCamion                   │
│    - PositionDrone(t), VecteurVitesseDrone                     │
│    - HauteurDrop = 5m au-dessus du camion                      │
│                                                                │
│  Process:                                                      │
│    1. Prédire PositionCamion(t+Δt) avec Kalman Filter          │
│    2. Calculer point intercept à hauteur h=5m                  │
│    3. Générer trajectoire lissée (spline cubique)              │
│    4. Vérifier accessibilité et sécurité                       │
│    5. Ajuster en temps réel si dérive > 2m                     │
│                                                                │
│  Sortie: Trajectoire validée ou fallback (report)             │
└────────────────────────────────────────────────────────────────┘
```

---

### 1.4 UC-004 : Retour et Recharge Drone

**Description** : Après dépôt, le drone retourne à sa base ou au point de recharge.

**Acteurs** : Drone, Station/Recharge, Orchestrateur

**User Story** :
```
En tant que drone,
Je veux retourner à ma station de recharge après une série de collecte,
Afin d'être prêt pour une prochaine mission.
```

**Critères d'Acceptance** :

| Condition | Description |
|-----------|-------------|
| **GIVEN** | Drone a déposé tous déchets, batterie < 40% ou mission terminée |
| **WHEN** | Orchestrateur notifie "RETOUR_BASE" |
| **THEN** | Calcul trajet optimal vers station la plus proche |
| **AND** | Vol de retour, atterrissage automatique |
| **AND** | Connexion chargeur, début recharge |
| **AND** | Status = "RECHARGING", NOTIF periodicique batterie |

---

### 1.5 UC-005 : Supervision Mobile

**Description** : L'opérateur supervise en temps réel depuis l'application mobile.

**Acteurs** : Opérateur, App Mobile, Backend

**User Story** :
```
En tant qu'opérateur,
Je veux voir l'état de tous les composants en temps réel,
Afin de superviser les opérations et intervenir si nécessaire.
```

**Critères d'Acceptance** :

| Condition | Description |
|-----------|-------------|
| **GIVEN** | App connectée, session auth validée |
| **WHEN** | Dashboard chargé |
| **THEN** | Carte affiche position camion + tous drones |
| **AND** | Indicateurs statut (vert/orange/rouge) par composant |
| **AND** | Alerts en temps réel si anomalie (< 1s latence) |

---

## 2. Flux Opérationnels

### 2.1 Flux Principal : Cycle de Collection Complet

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│                          FLUX OPÉRATIONNEL PRINCIPAL                              │
└──────────────────────────────────────────────────────────────────────────────────┘

   ┌─────────┐      ┌──────────────┐      ┌─────────────┐      ┌─────────────┐
   │ BACKEND │      │ ORCHESTRATEUR │      │    DRONE    │      │   CAMION    │
   └────┬────┘      └───────┬───────┘      └──────┬──────┘      └──────┬──────┘
        │                  │                      │                   │
        │ 1. CREATE_MISSION │                      │                   │
        │─────────────────►│                      │                   │
        │                  │                      │                   │
        │                  │ 2. PLANIFY_MISSION   │                   │
        │                  │─────────────────────►│                   │
        │                  │                      │                   │
        │                  │    3. ACK + STATUS   │                   │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │                  │ 4. SEND_ROUTE        │                   │
        │                  │────────────────────────────────────────►│
        │                  │                      │                   │
        │                  │                  5. MOVE_TO_ZONE       │
        │                  │◄────────────────────────────────────────│
        │                  │                      │                   │
        │                  │ 6. DRONE_TO_COLLECT  │                   │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │                  │              7. DETECT + COLLECT        │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │                  │ 8. REQUEST_RENDEZVOUS│                   │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │                  │ 9. CALCUL_INTERCEPT  │                   │
        │                  │─────────────────────►│                   │
        │                  │                      │                   │
        │                  │              10. INTERCEPT_MOVE         │
        │                  │◄────────────────────────────────────────│
        │                  │                      │                   │
        │                  │           11. DEPOSIT_WASTE             │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │                  │     12. ACK_DEPOSIT  │                   │
        │                  │────────────────────────────────────────►│
        │                  │                      │                   │
        │                  │          13. REPEAT OR RETURN           │
        │                  │◄─────────────────────│                   │
        │                  │                      │                   │
        │    14. STATUS_UPDATE                    │                   │
        │◄─────────────────│                      │                   │
        │                  │                      │                   │
```

### 2.2 Flux Décisionnel : Gestion des Rendez-Vous

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     FLUX DÉCISIONNEL RENDEZ-VOUS                            │
└─────────────────────────────────────────────────────────────────────────────┘

    DRONE                    ORCHESTRATEUR                   CAMION
      │                          │                              │
      │  REQ_RENDEZVOUS          │                              │
      │─────────────────────────►│                              │
      │                          │                              │
      │                    ┌─────┴─────┐                       │
      │                    │ Calcul    │                       │
      │                    │ Trajectoire│                      │
      │                    │ Interception│                     │
      │                    └─────┬─────┘                       │
      │                          │                              │
      │                          │  GET_POSITION_PROJECTED     │
      │                          │─────────────────────────────►│
      │                          │                              │
      │                          │  POSITION_UPDATE            │
      │                          │◄─────────────────────────────│
      │                          │                              │
      │                   ┌──────┴──────┐                       │
      │                   │ Validate    │                       │
      │                   │ Interception│                       │
      │                   └──────┬──────┘                       │
      │                          │                              │
      │              ┌────────────┴────────────┐                │
      │              │                         │                │
      │     ┌────────┴────────┐      ┌─────────┴────────┐      │
      │     │ Valide (OK)     │      │ Échec (RETRY)    │      │
      │     └────────┬────────┘      └────────┬────────┘      │
      │              │                         │               │
      │    ┌─────────▼─────────┐    ┌──────────▼──────────┐    │
      │    │ SEND_FLIGHT_PLAN  │    │ RESCHEDULE_RENDEZVOUS│   │
      │    │   (Drone)         │    │  (Nouvelle tentative)│   │
      │    └───────────────────┘    └─────────────────────┘    │
      │              │                         │                 │
```

---

## 3. Interactions Entre Acteurs

### 3.1 Matrice des Interactions

| De \ Vers | Backend | Orchestrateur | Camion | Drone | Mobile |
|-----------|---------|---------------|--------|-------|--------|
| **Backend** | - | CRUD Mission | Config | - | Read Status |
| **Orchestrateur** | Status Update | - | Position Sync | Mission Ctrl | - |
| **Camion** | Telemetry | Position Stream | - | Intercept | - |
| **Drone** | Telemetry | Status/State | Handoff | - | - |
| **Mobile** | Commands | - | - | - | - |

### 3.2 Protocoles de Communication

#### 3.2.1 Drone ↔ Orchestrateur

```
Topic:  drone/{drone_id}/state
Payload: {
  "drone_id": "drone_001",
  "status": "EN_VOL|COLLECTE|EN_ROUTE_RV|DEPOSE|RECHARGING",
  "position": {"lat": 45.501, "lng": -73.567, "alt": 50},
  "battery_pct": 78,
  "current_mission_id": "mission_042",
  "timestamp": "2024-01-15T10:30:00Z"
}

Topic:  orchestrator/{drone_id}/command
Payload: {
  "command": "START_COLLECTION|RETURN_BASE|CANCEL",
  "params": {...},
  "priority": "NORMAL|URGENT"
}
```

#### 3.2.2 Camion ↔ Orchestrateur

```
Topic:  truck/{truck_id}/telemetry
Payload: {
  "truck_id": "truck_001",
  "position": {"lat": 45.501, "lng": -73.567},
  "speed_kmh": 45,
  "heading_deg": 270,
  "next_stop": "intersection_042",
  "projected_positions": [
    {"lat": 45.502, "lng": -73.568, "eta_seconds": 30},
    {"lat": 45.503, "lng": -73.569, "eta_seconds": 60}
  ],
  "capacity_kg": 1200,
  "battery_pct": 65
}

Topic:  orchestrator/truck/{truck_id}/update
Payload: {
  "mission_id": "mission_042",
  "drones_assigned": ["drone_001", "drone_002"],
  "rendezvous_points": [...]
}
```

---

## 4. Séquences Temporelles

### 4.1 Séquence Complète d'une Mission

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        SÉQUENCE TEMPORELLE - MISSION COMPLETE                            │
└────────────────────────────────────────────────────────────────────────────────────────┘

TEMPS    BACKEND           ORCHESTRATEUR         CAMION              DRONE
│        │                    │                   │                   │
T0       │ CREATE_MISSION     │                    │                   │
├────────┼──────────────────►│                    │                   │
T0+100ms │                    │ PLANIFY            │                   │
├────────┼──────────────────►│ ────────────────►  │                   │
T0+200ms │                    │                    │ TRAJECTORY_UPDATE │
├────────┼──────────────────►│                    │ ◄────────────────  │
T0+500ms │                    │                    │ MOVE               │
├────────┼──────────────────►│                    │ ═══════════════►   │
         │                    │                    │                   │
         │                    │ DRONE_ASSIGN       │                   │
├────────┼──────────────────►│ ────────────────►  │                   │
T0+1s    │                    │                    │ START_COLLECTION  │
├────────┼──────────────────►│                    │ ◄────────────────  │
         │                    │                    │                   │
T0+5s    │                    │                    │ FLY_TO_ZONE       │
├────────┼──────────────────►│                    │ ◄────────────────  │
         │                    │                    │ DETECT_OBJECT     │
T0+10s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ APPROACH           │
T0+15s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ COLLECT            │
T0+18s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │ REQ_RENDEZVOUS      │                   │
T0+20s   │                    │ ◄─────────────────│                    │
├────────┼──────────────────►│                    │                   │
         │                    │ CALC_INTERCEPT      │                   │
         │                    │ ────────────────►  │                   │
T0+21s   │                    │                    │ INTERCEPT_PLAN    │
├────────┼──────────────────►│                    │ ◄────────────────  │
         │                    │                    │                   │
         │                    │                 GET_POS (every 100ms)│
T0+22s   │                    │ ◄─────────────────│ ◄────────────────  │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ FLY_INTERCEPT     │
T0+25s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ SYNC_SPEED        │
T0+28s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │                    │ ALIGN_ABOVE        │
T0+30s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │                    │ DROP               │
T0+32s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │              TRUCK_RECEIVE            │
T0+33s   │                    │ ◄─────────────────│ ═══════════════►   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │ ACK_DEPOSIT        │                   │
T0+34s   │                    │ ────────────────►  │                    │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ CONFIRM_DEPOSIT   │
T0+35s   │                    │ ◄─────────────────│ ◄────────────────  │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │ CHECK_MISSION      │                   │
T0+36s   │                    │ ────────────────►  │                   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │                   │
         │                    │ [Plus d'objets?]   │                   │
         │                    │    └── OUI         │                   │
         │                    │                    │                   │
         │                    │ NEXT_COLLECTION    │                   │
T0+37s   │                    │ ────────────────►  │                   │
├────────┼──────────────────►│                    │                   │
         │                    │                    │ NEXT_OBJECT       │
T0+38s   │                    │ ◄─────────────────│ ◄────────────────  │
├────────┼──────────────────►│                    │                   │
```

### 4.2 Points de Synchronisation Critiques

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                    POINTS DE SYNCHRONISATION CRITIQUES                         │
└────────────────────────────────────────────────────────────────────────────────┘

┌────────────┬──────────────────────────────────────────────────────────────────┐
│  POINT     │  DESCRIPTION                                                      │
├────────────┼──────────────────────────────────────────────────────────────────┤
│  PS-1      │  Mission Start: Backend → Orchestrateur (100ms max)               │
│  PS-2      │  Route Update: Orchestrateur → Camion (500ms max)                 │
│  PS-3      │  Drone Assign: Orchestrateur → Drone (200ms max)                 │
│  PS-4      │  Intercept Calculation: Temps réel (< 50ms par itération)        │
│  PS-5      │  Position Sync: Camion → Orchestrateur (100ms, heartbeat)         │
│  PS-6      │  Deposit Confirm: Camion → Orchestrateur → Backend (< 200ms)      │
│  PS-7      │  Status Update: Backend → Mobile (< 1s)                           │
└────────────┴──────────────────────────────────────────────────────────────────┘
```

---

## 5. Préconditions et Postconditions

### 5.1 Matrice des Conditions

| Cas d'Usage | Préconditions | Postconditions |
|-------------|---------------|----------------|
| **UC-001** | - Backend en ligne<br>- Orchestrateur connecté<br>- Au moins 1 drone dispo<br>- Camion en service<br>- GPS valide | - Mission.status = "PLANIFIEE"<br>- Drones assignés<br>- Trajet camion calculé<br>- Notification envoyée |
| **UC-002** | - Drone.status = "EN_VOL"<br>- Zone de collecte définie<br>- Capteurs opérationnels<br>- Batterie > 30% | - Déchet collecté<br>- Déchet.position enregistrée<br>- Drone.status = "EN_ROUTE_RV" |
| **UC-003** | - Drone a déchet<br>- Camion en mouvement<br>- Connexion établie<br>- Distance < 500m | - Déchet déposé dans benne<br>- Camion.acknowledged<br>- Drone.status mis à jour |
| **UC-004** | - Drone.status = "COLLECTE" ou "EN_VOL"<br>- Batterie < 40% OU mission terminée<br>- Station reachable | - Drone.atterri<br>- Drone.status = "RECHARGING"<br>- Chargement actif |
| **UC-005** | - Session utilisateur valide<br>- App ouverte<br>- Connexion internet | - Dashboard affiché<br>- Positions en temps réel<br>- Alerts actives |

### 5.2 Conditions de Succès Globales

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CONDITIONS DE SUCCÈS SYSTÈME                          │
└─────────────────────────────────────────────────────────────────────────────┘

CRITÉRE                    DÉFINITION                         SEUIL MIN
─────────────────────────────────────────────────────────────────────────────
Disponibilité système     % temps où système opérationnel      > 99.5%
Temps de réponse RV        Latence calcul interception         < 100ms
Précision interception    Écart position réelle vs prédite    < 1.5m
Taux collecte              Déchets collectés / proposés       > 90%
Batterie min               Seuil critique avant retour forcé   > 15%
Latence mobile             Status → interface utilisateur      < 2s
```

---

## 6. Schéma d'État Global

### 6.1 Machine à États du Système

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     MACHINE À ÉTATS SYSTÈME                                 │
└─────────────────────────────────────────────────────────────────────────────┘

                           ┌─────────────────┐
                           │    INITIAL      │
                           │   (Booting)     │
                           └────────┬────────┘
                                    │
                           ┌────────▼────────┐
                           │   OPERATIONAL   │
                           │  (Tous services │
                           │    alive)       │
                           └────────┬────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              │                     │                     │
     ┌────────▼────────┐   ┌────────▼────────┐   ┌────────▼────────┐
     │  MISSION_ACTIVE │   │  DEGRADED_MODE  │   │  MAINTENANCE    │
     │  (Collecte en    │   │  (1+ composants │   │  (Update/      │
     │   cours)        │   │   indispos)     │   │   Réparation)  │
     └────────┬────────┘   └────────┬────────┘   └────────┬────────┘
              │                     │                     │
              │            ┌────────▼────────┐            │
              │            │  RECOVERY      │            │
              │            │  Auto ou manuel│            │
              │            └────────┬────────┘            │
              │                     │                     │
              └─────────────────────┴─────────────────────┘
                                    │
                           ┌────────▼────────┐
                           │    SHUTDOWN      │
                           │  (Arrêt contrôlé)│
                           └─────────────────┘
```

### 6.2 Diagramme de Déploiement

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        DÉPLOIEMENT COMPOSANTS                                  │
└─────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────┐                ┌─────────────────────┐
│   CLIENT MOBILE     │                │      AWS/GCP        │
│                     │                │                     │
│  ┌───────────────┐  │   HTTPS/WSS    │  ┌───────────────┐  │
│  │  React Native │  │◄──────────────►│  │   API Gateway │  │
│  │  / Flutter    │  │                │  └───────┬───────┘  │
│  └───────────────┘  │                │          │          │
└─────────────────────┘                │  ┌───────┴───────┐  │
                                      │  │  Orchestrator │  │
                                      │  │   Service     │  │
                                      │  └───────┬───────┘  │
                                      │          │          │
                                      │  ┌───────┴───────┐  │
                                      │  │  Redis/Rabbit │  │
                                      │  │     MQ        │  │
                                      │  └───────┬───────┘  │
                                      │          │          │
                                      │  ┌───────┴───────┐  │
                                      │  │  PostgreSQL   │  │
                                      │  │  + TimescaleDB│  │
                                      │  └───────────────┘  │
                                      └──────────┬──────────┘
                                                 │
                    ┌────────────────────────────┼────────────────────────────┐
                    │                            │                            │
        ┌───────────▼───────────┐    ┌───────────▼───────────┐    ┌────────▼────────┐
        │       CAMION          │    │        DRONE(S)       │    │    STATION      │
        │   (Edge Computing)    │    │    (Edge Computing)   │    │   RECHARGE      │
        │                      │    │                       │    │                 │
        │  ┌────────────────┐  │    │  ┌────────────────┐   │    │  ┌───────────┐  │
        │  │  ROS2 Node    │  │    │  │  ROS2 Node    │   │    │  │  IoT Hub  │  │
        │  │  - Navigation │  │    │  │  - Flight Ctrl │   │    │  └───────────┘  │
        │  │  - Telemetry  │  │    │  │  - Perception │   │    │                 │
        │  │  - Position   │  │    │  │  - Collection │   │    └─────────────────┘
        │  └────────────────┘  │    │  └────────────────┘   │                      │
        │                     │    │                       │                      │
        │  MQTT Bridge        │    │  MQTT Bridge          │                      │
        │  to Cloud           │    │  to Cloud             │                      │
        └─────────────────────┘    └───────────────────────┘                      │
```

---

## 7. Gestion des Risques et Fallbacks

### 7.1 Scénarios de Dégradation

| Scénario | Déclencheur | Action |
|----------|-------------|--------|
| **Échec RV** | Calcul interception échoue 3x | Drone retourne à station, report mission |
| **Perte com** | Timeout > 5s | Drone hover, mode dégradé, tentative reconnexion |
| **Batterie critique** | < 15% | Atterrissage immédiat sécurisé |
| **Camion arrêté** | Speed = 0 > 10s | Annulation RV, reprogrammation |
| **Drone indisponible** | Hors service | Réassignation automatique autre drone |

---

## 8. Protocole de Réservation de RV (Anti-Deadlock)

### 8.1 Problématique

Plusieurs drones peuvent demander simultanément un rendez-vous avec le même camion. Sans protocole de réservation, un deadlock peut survenir :
- **Inter-blocage** : Deux drones attendent chacun que l'autre libère le couloir d'approche
- **Starvation** : Un drone en attente constante peut ne jamais exécuter son RV
- **Collision** : Deux drones tentent d'intercepter simultanément

### 8.2 Protocole de Réservation

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  PROTOCOLE DE RÉSERVATION DE RV                            │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  ÉTAT GLOBAL: TABLE DE RÉSERVATION (Orchestrateur)                          │
├──────────────────────────────────────────────────────────────────────────────┤
│  {                                                                           │
│    "truck_001": {                                                            │
│      "slot_reserved": {                                                      │
│        "start_time": "2024-01-15T10:35:00Z",                                │
│        "end_time": "2024-01-15T10:36:00Z",                                  │
│        "drone_id": "drone_001",                                             │
│        "status": "LOCKED"                                                    │
│      },                                                                      │
│      "queue": ["drone_002", "drone_003"]                                    │
│    }                                                                         │
│  }                                                                           │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 8.3 Algorithme de Réservation

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ALGORITHME DE RÉSERVATION                                 │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  DRONE                                                                      │
│  ──────                                                                     │
│  1. Requête RV                                                              │
│     POST /api/v1/rendezvous/reserve                                         │
│     { "drone_id": "drone_001", "truck_id": "truck_001" }                    │
│                                                                              │
│  2. Réponse Orchestrateur                                                   │
│     - SI slot disponible: { "status": "GRANTED", "time_slot": "10:35:00" } │
│     - SI slot occupé:    { "status": "QUEUED", "position": 2 }             │
│                                                                              │
│  3. Confirmation (avant T-30s)                                             │
│     POST /api/v1/rendezvous/confirm                                         │
│     { "drone_id": "drone_001", "truck_id": "truck_001" }                   │
│                                                                              │
│  4. Timeout (30s avant slot) non confirmé → slot libéré                    │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  ORCHESTRATEUR                                                              │
│  ─────────────                                                              │
│  1. Réception requête                                                      │
│     - Vérifier disponibilité slot (±5min fenêtre)                          │
│     - Verrouiller avec TTL (60s)                                           │
│                                                                              │
│  2. Allocation slot                                                        │
│     - Premier entré, premier servi (FIFO)                                  │
│     - Fenêtre glissante de 5 minutes entre RV                              │
│                                                                              │
│  3. Release slot                                                            │
│     - Après ACK_DEPOSIT reçu                                               │
│     - Ou après timeout (2 min)                                             │
│     - Notifier prochain drone en queue                                      │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 8.4 Diagramme de Séquence Réservation

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    SÉQUENCE RÉSERVATION RV                                   │
└──────────────────────────────────────────────────────────────────────────────┘

TEMPS    DRONE_001          ORCHESTRATEUR           DRONE_002          CAMION
│        │                      │                      │               │
│        │ RESERVE_RV            │                      │               │
│        │─────────────────────►│                      │               │
│        │                      │                      │               │
│        │ GRANTED (slot 10:35) │                      │               │
│        │◄─────────────────────│                      │               │
│        │                      │                      │               │
│        │                      │              RESERVE_RV              │
│        │                      │─────────────────────►│               │
│        │                      │                      │               │
│        │                      │ QUEUED (position: 1) │               │
│        │                      │◄─────────────────────│               │
│        │                      │                      │               │
│        │ CONFIRM (T-30s)      │                      │               │
│        │─────────────────────►│                      │               │
│        │                      │                      │               │
│        │                      │             SLOT_RELEASED            │
│        │                      │◄───────────────────────────────────────│
│        │                      │                      │               │
│        │                      │ GRANTED (slot 10:40) │               │
│        │                      │─────────────────────►│               │
│        │                      │                      │               │
│        │                      │              CONFIRM                │
│        │                      │─────────────────────►│               │
│        │                      │                      │               │
```

### 8.5 Gestion des Deadlocks et Timeouts

| Scénario | Détection | Action |
|----------|-----------|--------|
| **Drone ne confirme pas** | Timeout 30s | Slot libéré, next drone notifié |
| **Drone en queue trop long** | > 5 min | Priorité boostée, alerter ops |
| **Camion改变的trajectoire** | Position dérive > 50m | Annuler tous slots, recalcul |
| **Drone en approche trop lent** | Vitesse < 5 km/h | Timeout 60s, cancel RV |
| **Collision imminente** | Distance < 10m | Hover forcé, slot reporté |

---

## 9. Messages d'Erreur pour l'Opérateur

### 9.1 Catalogue des Erreurs

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CATALOGUE ERREURS OPÉRATEUR                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌────────────────────┬────────────────────────────────────────────────────────┐
│    CODE ERREUR     │                  DESCRIPTION                           │
├────────────────────┼────────────────────────────────────────────────────────┤
│ ERR_001            │ Connexion perdue avec le drone                          │
│ ERR_002            │ Batterie drone critique (< 15%)                        │
│ ERR_003            │ Échec calcul interception - aucun trajectoire valide    │
│ ERR_004            │ Timeout réponse drone (pas de heartbeat 10s)           │
│ ERR_005            │ Capteur de détection défaillant                         │
│ ERR_006            │ Camion arrêté - RV impossible                           │
│ ERR_007            │ Réservation RV refusée - slot occupé                    │
│ ERR_008            │ Perte connexion camion                                  │
│ ERR_009            │ Capacité benne camion atteinte                          │
│ ERR_010            │ Station de recharge inaccessible                       │
│ ERR_011            │ GPS invalide - position incertaine                      │
│ ERR_012            │ Moteur de vol défaillant                               │
│ ERR_013            │ Bras collecteur bloqué                                 │
│ ERR_014            │ Collision détectée avec obstacle                       │
│ ERR_015            │ Orchestrateur indisponible (SPOF?)                     │
└────────────────────┴────────────────────────────────────────────────────────┘
```

### 9.2 Messages Détaillés par Catégorie

#### 9.2.1 Erreurs Drone

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ERREURS DRONE                                         │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_001: CONNEXION DRONE PERDUE                                             │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🔴 CRITIQUE                                                        │
│ Message: "Perte de connexion avec drone {drone_id}. Tentative de          │
│          reconnexion en cours..."                                           │
│ Action Opérateur: Surveiller - reconnexion auto dans 30s. Si échec,        │
│                   местоположение dernière position connue.                 │
│ Action Système: Drone passe en mode dégradé, hover sur place.             │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_002: BATTERIE CRITIQUE                                                   │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🔴 CRITIQUE                                                        │
│ Message: "Drone {drone_id}: Batterie à {battery_pct}%. Retour immédiat      │
│          à la station de recharge déclenché."                                │
│ Action Opérateur: Surveiller le retour du drone. Intervenir si nécessaire. │
│ Action Système: Retour autonome immédiat, priorité maximale.                │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_012: DÉFAILLANCE MOTEUR VOL                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🔴 CRITIQUE                                                        │
│ Message: "Drone {drone_id}: Défaillance moteur détectée. Atterrissage     │
│          d'urgence en cours à la position actuelle."                        │
│ Action Opérateur: Alerte immédiatement - envoyer équipe technique.         │
│ Action Système: Atterrissage contrôlé, signal MAYDAY émis.                │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_014: COLLISION DETECTÉE                                                 │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🔴 CRITIQUE                                                        │
│ Message: "ALERTE COLLISION - Drone {drone_id} a détecté une collision.     │
│          Statut: {status}. Position: {lat}, {lng}"                          │
│ Action Opérateur: Intervention immédiate requise. Évaluer dommages.       │
│ Action Système: Arrêt moteurs, enregistrement black-box, alerter services.│
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 9.2.2 Erreurs Système / Orchestrateur

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ERREURS SYSTÈME / ORCHESTRATEUR                          │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_003: ÉCHEC CALCUL INTERCEPTION                                          │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🟠 MAJEUR                                                          │
│ Message: "Impossible de calculer une trajectoire d'interception pour      │
│          drone {drone_id} vers camion {truck_id}. Cause: {reason}"          │
│ Cause Possible: Camion trop rapide, obstacles, zone inaccessible          │
│ Action Opérateur: Valider manuellement ou reporter le RV.                  │
│ Action Système: Drone retourne en zone d'attente, nouvelle tentative 30s   │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_015: ORCHESTRATEUR INDISPONIBLE                                         │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🔴 CRITIQUE                                                        │
│ Message: "Orchestrateur {instance_id} temporairement indisponible.         │
│          Failover en cours vers instance secondaire..."                    │
│ Action Opérateur: Attendre automatiquement. Aucune action requise.         │
│ Action Système: Basculement automatique sur instance redondante.          │
└──────────────────────────────────────────────────────────────────────────────┘
```

#### 9.2.3 Erreurs Camion

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ERREURS CAMION                                       │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_006: CAMION ARRÊTÉ                                                      │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🟠 MAJEUR                                                          │
│ Message: "Camion {truck_id} arrêté depuis {duration}s. Les RV planifiés   │
│          sont annulés. Reason: {reason}"                                   │
│ Reason: Traffic, obstacle, défaillance technique                            │
│ Action Opérateur: Évaluer situation, décider de reprendre ou annuler.       │
│ Action Système: Annuler RV en cours, reprogrammer après redémarrage.       │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_008: CONNEXION CAMION PERDUE                                            │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🟠 MAJEUR                                                          │
│ Message: "Perte de connexion avec camion {truck_id}. Dernière position    │
│          connue: {lat}, {lng}. Suivi manuel requis."                        │
│ Action Opérateur: Contacter chauffeur si possible. Suivre par autre moyen. │
│ Action Système: Drone continue en mode autonome, sans sync position.       │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│ ERR_009: CAPACITÉ BENNE ATTEINTE                                            │
├──────────────────────────────────────────────────────────────────────────────┤
│ Sévérité: 🟡 AVERTISSEMENT                                                  │
│ Message: "Camion {truck_id}: Capacité maximale ({capacity_kg}kg) atteinte. │
│          Vidage requis avant continuation."                                 │
│ Action Opérateur: Diriger camion vers point de vidage.                     │
│ Action Système: Drones en attente, pas de nouveau RV jusqu'à vidage.       │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 Interface Opérateur - Format Notifications

```
┌─────────────────────────────────────────────────────────────────────────────┐
│               FORMAT NOTIFICATIONS DASHBOARD                              │
└─────────────────────────────────────────────────────────────────────────────┘

{
  "notification_id": "notif_2024-01-15_001",
  "timestamp": "2024-01-15T10:35:00Z",
  "type": "ERROR",
  "severity": "CRITICAL|MAJOR|WARNING",
  "code": "ERR_001",
  "message": "Perte de connexion avec drone drone_001",
  "affected_components": [
    {
      "type": "DRONE",
      "id": "drone_001",
      "last_known_position": {"lat": 45.501, "lng": -73.567}
    }
  ],
  "recommended_actions": [
    "Surveiller reconnexion automatique",
    "Préparer intervention terrain si échec dans 60s"
  ],
  "auto_actions_taken": [
    "Tentative reconnexion",
    "Drone passé en mode dégradé"
  ],
  "ack_required": true,
  "sla_timer_seconds": 300
}
```

---

## 10. Stratégie de Redondance de l'Orchestrateur (Pas de SPOF)

### 10.1 Architecture Actif-Passif

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              ARCHITECTURE REDONDANTE ORCHESTRATEUR                         │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│                          INFRASTRUCTURE CLOUD                               │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                     LOAD BALANCER (HAProxy/AWS ALB)                │    │
│  │              (Santé Check toutes les 5 secondes)                  │    │
│  └─────────────────────────────┬───────────────────────────────────────┘    │
│                                │                                             │
│         ┌──────────────────────┴──────────────────────┐                      │
│         │                                             │                      │
│  ┌──────▼──────┐                            ┌────────▼────────┐            │
│  │ ORCHESTR-1  │                            │ ORCHESTR-2      │            │
│  │  (ACTIF)    │◄─────heartbeat─────►      │  (STANDBY)      │            │
│  │             │     (toutes les 1s)        │                 │            │
│  │ - Mission   │                            │ - Sync état     │            │
│  │ - Planning │                            │ - En veille    │            │
│  │ -调度       │                            │ - Prêt à prise │            │
│  └────────────┘                            └────────┬────────┘            │
│         │                                           │                      │
│         │              ┌────────────────────────────┴────────┐             │
│         │              │         MESSAGE BROKER              │             │
│         │              │    (Cluster Redis/RabbitMQ)          │             │
│         │              │    - Pub/Sub avec persistence       │             │
│         │              │    - Réplication Maître-Esclave     │             │
│         │              └─────────────────────────────────────┘             │
└─────────┼─────────────────────────────────────────────────┼────────────────┘
          │                                                 │
          │              TOPIC SYNC ÉTAT                    │
          │              ─────────────────                  │
          │  ORCHESTR-1 ───────────────► ORCHESTR-2        │
          │  (state_update)   every 500ms                   │
          │                                                 │
          │         FAILOVER TRIGGER                        │
          │         ─────────────────                       │
          │  1. Heartbeat timeout > 5s                      │
          │  2. Health check HTTP 5xx                       │
          │  3. Exception non gérée critique                │
```

### 10.2 Mécanisme de Failover

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      PROCESSUS FAILOVER                                      │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  1. DÉTECTION ÉCHEC                                                        │
│     - Heartbeat: pas de signal pendant 5s                                   │
│     - Health Check: /health retourne 503 ou timeout                        │
│     - Exception: crash ou exception non gérée                              │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  2. ÉLECTION NOUVEAU ACTIF                                                 │
│     - SI Load Balancer: bascule automatique vers standby                    │
│     - SI Election (Raft/Paxos): le noeud standby devient actif              │
│     - TTL: < 10 secondes                                                    │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  3. RESTAURATION ÉTAT                                                       │
│     - Lire dernier état depuis DB (mission active, positions drones)      │
│     - Recharger messages en attente depuis MQ                               │
│     - Rebuild in-memory state depuis TimescaleDB                           │
│     - Durée estimée: < 30 secondes                                          │
└──────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  4. REPRISE OPÉRATIONNELLE                                                 │
│     - Reprendre planification mission                                       │
│     - Rétablir flux normal drones                                          │
│     -Notifier backend et mobile                                             │
│     - Alerter opératur: "Failover effectué, service opérationnel"          │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 10.3 Données à Synchroniser

| Donnée | Fréquence | Persistence | RPO |
|--------|-----------|-------------|-----|
| Missions actives | Temps réel | DB + MQ | < 1s |
| Positions drones | 100ms | In-memory + TimescaleDB | < 500ms |
| Réservations RV | Temps réel | DB | < 1s |
| État système global | 500ms | DB | < 5s |
| Telemetry camion | 100ms | TimescaleDB | < 1s |

**RPO (Recovery Point Objective)** : Perte de données max acceptable = 5 secondes

**RTO (Recovery Time Objective)** : Temps de reprise = < 30 secondes

### 10.4 Monitoring et Alerting

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MONITORING ORCHESTRATEUR                                 │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  MÉTRIQUES CLÉS (Dashboard Grafana)                                         │
├──────────────────────────────────────────────────────────────────────────────┤
│  • Uptime: > 99.95% par instance                                            │
│  • Latence moyenne réponse: < 50ms                                          │
│  • Nombre de missions actives: [0-N]                                        │
│  • Queue depth (RabbitMQ): < 1000 messages                                  │
│  • CPU: < 70% en moyenne                                                    │
│  • Memory: < 80%                                                            │
│  • Failover count: counter par mois                                         │
└──────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────────┐
│  ALERTES                                                                     │
├──────────────────────────────────────────────────────────────────────────────┤
│  CRITICAL:                                                                   │
│    - Orchestrateur unreachable > 30s                                        │
│    - Toutes les instances down                                              │
│    - Perte de données > RPO                                                  │
│                                                                              │
│  WARNING:                                                                    │
│    - Latence > 200ms                                                         │
│    - Queue depth > 5000                                                      │
│    - CPU > 85% pendant > 5 min                                               │
│    - Failover次数 > 3/jour                                                   │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 11. Exigences Quantifiées (KPIs)

### 11.1 Exigences de Performance

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXIGENCES DE PERFORMANCE                                 │
└─────────────────────────────────────────────────────────────────────────────┘

┌────────────────────────────┬───────────────────┬─────────────────────────────┐
│          MÉTRIQUE         │      EXIGENCE     │          JUSTIFICATION       │
├────────────────────────────┼───────────────────┼─────────────────────────────┤
│ Latence création mission  │ < 500ms           │ Réactivité opérateur         │
│ Latence affectation drone │ < 1s              │ Temps de décision planning  │
│ Latence calcul RV         │ < 100ms           │ Interception temps réel     │
│ Latence position sync     │ < 200ms           │ Précision positionnement    │
│ Latence UI mobile         │ < 2s              │ Expérience utilisateur      │
│ Temps failover            │ < 30s             │ Continuité service          │
│ Temps reconnect drone     │ < 10s             │ Reprise rapide              │
└────────────────────────────┴───────────────────┴─────────────────────────────┘
```

### 11.2 Exigences de Précision

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXIGENCES DE PRÉCISION                                   │
└─────────────────────────────────────────────────────────────────────────────┘

┌────────────────────────────┬───────────────────┬─────────────────────────────┐
│          MÉTRIQUE         │      EXIGENCE     │          JUSTIFICATION       │
├────────────────────────────┼───────────────────┼─────────────────────────────┤
│ Précision interception    │ < 1.5m            │ Dépôt sécurisé benne        │
│ Précision position drones │ < 0.5m (RTK GPS)  │ Sécurité vol                │
│ Précision détection déchet│ > 85% confiance   │ Taux de fausse détection   │
│ Précision prédiction cam. │ < 2m à 30s ahead │ Trajectoire interception    │
│ Précision batterie        │ ± 5%              │ Planification recharge     │
└────────────────────────────┴───────────────────┴─────────────────────────────┘
```

### 11.3 Exigences de Disponibilité

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXIGENCES DE DISPONIBILITÉ                               │
└─────────────────────────────────────────────────────────────────────────────┘

┌────────────────────────────┬───────────────────┬─────────────────────────────┐
│          MÉTRIQUE         │      EXIGENCE     │          JUSTIFICATION       │
├────────────────────────────┼───────────────────┼─────────────────────────────┤
│ Disponibilité système      │ > 99.5%           │ Opérations 24/7             │
│ Disponibilité orchestrateur│ > 99.95% (cluster)│ Pas de SPOF                │
│ MTBF composants critiques │ > 10,000 heures   │ Fiabilité équipements       │
│ MTTR système              │ < 30 minutes      │ Reprise rapide              │
│ Max downtime mission      │ < 5 minutes       │ Continuité collecte         │
│ Rate limitation drones    │ < 3 par an        │ Disponibilité flotte        │
└────────────────────────────┴───────────────────┴─────────────────────────────┘
```

### 11.4 Exigences Opérationnelles

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXIGENCES OPÉRATIONNELLES                                │
└─────────────────────────────────────────────────────────────────────────────┘

┌────────────────────────────┬───────────────────┬─────────────────────────────┐
│          MÉTRIQUE         │      EXIGENCE     │          JUSTIFICATION       │
├────────────────────────────┼───────────────────┼─────────────────────────────┤
│ Taux collecte              │ > 90%             │ Efficacité opérationnelle   │
│ Capacité benne             │ > 1000 kg         │ Autonomie quotidienne       │
│ Autonomie drone            │ > 45 minutes      | Duration vol                │
│ Temps recharge drone       │ < 60 minutes      | Rotation rapide             │
│ Zone de couverture         │ < 5 km rayon      | Rayon opérationnel          │
│ Nombre drones simultanés   | ≤ 10 drones       | Capacité orchestration      │
└────────────────────────────┴───────────────────┴─────────────────────────────┘
```

### 11.5 Tableau Récapitulatif

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              TABLEAU RÉCAPITULATIF - KPI SYSTÈME                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────┬──────────────┬──────────────┬──────────────┐
│     CATÉGORIE   │    CRITIQUE │    MAJEUR    │   MODÉRÉ    │
├──────────────────┼──────────────┼──────────────┼──────────────┤
│ LATENCE         │ < 100ms RV   │ < 500ms mise │ < 2s UI     │
│ PRÉCISION       │ < 1.5m interc│ < 0.5m GPS  │ > 85% détect│
│ DISPONIBILITÉ   │ > 99.95% orch│ > 99.5% syst │ > 95% drones│
│ CAPACITÉ        │ 10 drones    │ 1000kg benne │ 45min autonomy│
└──────────────────┴──────────────┴──────────────┴──────────────┘
```

---

## 12. Résumé des Livrables

| Élément | Section | Status |
|---------|---------|--------|
| ✅ 5 Cas d'usage détaillés | Section 1 | Complet |
| ✅ Flux opérationnels | Section 2 | Complet |
| ✅ Interactions acteurs | Section 3 | Complet |
| ✅ Séquences temporelles | Section 4 | Complet |
| ✅ Pré/Postconditions | Section 5 | Complet |
| ✅ Schémas architecturaux | Section 6 | Complet |
| ✅ Gestion risques | Section 7 | Complet |
| ✅ Protocole réservation RV | Section 8 | **NOUVEAU** |
| ✅ Messages erreur opérateur | Section 9 | **NOUVEAU** |
| ✅ Redondance orchestrateur | Section 10 | **NOUVEAU** |
| ✅ Exigences quantifiées | Section 11 | **NOUVEAU** |

---

*Document généré pour le playbook 03 - Architecture Fonctionnelle*
*Version : 1.1 (Corrigée selon retours comité validation)*
*Date : 2026-03-31*

**Corrections appliquées :**
1. ✅ Protocole de réservation RV (anti-deadlock) - Section 8
2. ✅ Messages d'erreur pour l'opérateur - Section 9
3. ✅ Stratégie de redondance orchestrateur (pas de SPOF) - Section 10
4. ✅ Exigences quantifiées (latence, précision, disponibilité) - Section 11