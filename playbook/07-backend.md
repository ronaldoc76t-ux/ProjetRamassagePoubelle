# Architecture backend cloud

## Story
- En tant qu'architecte backend, je veux un plan de service pour construire l'orchestrateur, API et ingestion télémétrie.
- Critère d'acceptation : architecture microservices, DB, sécurité, scalabilité et logs inclues.

## 1. Composants

### API Gateway
- Kong ou Traefik
- Rate limiting: 1000 req/min par client
- SSL/TLS termination
- Routes: /api/v1/* → microservices

### Auth (OAuth2/JWT)
- Providers: Google, Apple, email/password
- JWT tokens: access (15min) + refresh (7 jours)
- 2FA: TOTP (optionnel pour admin) - biométrie COMME second facteur, pas alternatif
- RBAC: admin, operator, viewer
- **Hash mots de passe**: Argon2id (pas bcrypt)

### Microservices
| Service | Ports | Responsabilité |
|---------|-------|----------------|
| mission-service | 8001 | CRUD missions, assignation |
| fleet-service | 8002 | Gestion flotte drones/camions |
| trajectory-service | 8003 | Prédiction EKF + LSTM |
| telemetry-service | 8004 | Ingestion métriques |
| coordination-service | 8005 | Orchestration rendez-vous |
| user-service | 8006 | Profils, auth, permissions |
| notification-service | 8007 | Push notifications |

## 2. Base de données

### PostgreSQL (primary)
```sql
-- Tables principales
users (id, email, password_hash, role, created_at)
drones (id, name, status, battery, last_position)
trucks (id, name, status, position, route_id)
missions (id, drone_id, truck_id, status, started_at, completed_at)
rendezvous (id, mission_id, target_time, position, status)
telemetry (time, drone_id, truck_id, lat, lng, speed, battery)
```

### TimescaleDB (timeseries)
- Extension de PostgreSQL pour séries temporelles
- Rétention: 30 jours données brutes, 1 an agrégées

### Redis (cache)
- Cache tokens JWT (TTL: 15min)
- Session user
- Rate limiting counters
- Pub/Sub pour notifications temps réel

## 3. Message Bus (Kafka)

### Topics
| Topic | Partitions | Retention | Description |
|-------|------------|-----------|-------------|
| telemetry.campaign | 6 | 7 jours | Métriques 10Hz |
| telemetry.drone | 3 | 7 jours | Données drone |
| telemetry.truck | 3 | 7 jours | Données camion |
| mission.events | 9 | 30 jours | Lifecycle missions |
| rendezvous.planning | 12 | 14 jours | Planification RV |
| alerts.critical | 6 | 90 jours | Alertes sécurité |

## 4. Orchestrateur et gestion

### Mission Planning
- assignation: drone disponible + plus proche zone
- slot rendez-vous: calcul fenêtre ±3s, marge ±2m
- ré-optimisation: toutes les 30s si nécessaire

### Prédicteur Trajectoire
- Input: position actuelle, vitesse, heading, carte
- Modèle: EKF (filtre classique) + LSTM (apprentissage)
- Output: prédiction 30-120s ahead
- Update rate: 1Hz

### Gestion Pannes
- Drone down → replanification automatique
- Perte signal → mode dégradé avec buffer local 5min
- Truck down → alert + intervention humaine

## 5. Sécurité & Scalabilité

### IAM
- RBAC: admin, operator, viewer
- Permissions granulaires par ressource
- Audit log toutes les actions admin

### Réseau
- VPC avec sous-réseaux isolés
- WAF: protection SQLi, XSS, DDoS
- mTLS entre services (Istio)

### Déploiement
- Kubernetes (GKE/EKS)
- HPA: CPU >70% → scale up
- Ressources: 2-4 vCPU, 4-8GB RAM par pod

## 6. Logs & Monitoring

### Stack
- Logs: Kafka → ELK (Elasticsearch, Logstash, Kibana)
- Métriques: Prometheus + Grafana
- Traces: Jaeger

### Dashboards
- Mission success rate
- Latence API p99
- Drone fleet status
- Alerts critique

### Alerts
- Slack/Discord pour équipe ops
- PagerDuty pour critique

## 7. RGPD & Conformité

### Droit à l'effacement (Article 17)
- Fonction `delete_user_data(user_id)` pour anonymisation
- Données supprimées: email, phone, localisation, historique

### Registre des Traitements
| Traitement | Base légale | Données | Rétention |
|------------|------------|----------|-----------|
| user_management | Consentement | email, nom, phone | 5 ans post-suppression |
| fleet_telemetry | Intérêt légitime | GPS, télémétrie | 90 jours puis agrégé |
| video_surveillance | Obligation légale | Vidéo | 30 jours |

## 8. CI/CD & DevOps

### Pipeline (GitLab CI / GitHub Actions)
```yaml
stages: build, test, scan, build-image, deploy
- build: compilation Go/Node
- test: unit tests + coverage >80%
- scan: SAST, dependency check
- build-image: Docker build + push
- deploy: Helm upgrade via ArgoCD
```

### Backup/Restore
- pg_dump quotidien + WAL-G pour archive
- Backup Kafka: MirrorMaker2 vers secondary
- RTO: 4h, RPO: 1h

### PodDisruptionBudget
```yaml
minAvailable: 2 pour services critiques
```