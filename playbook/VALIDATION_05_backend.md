# Validation Architecture Backend - Comité d'Évaluation

**Date:** 2026-03-31  
**Document source:** `07-backend.md`  
**Type:** Comité d'architecture multi-experts

---

## Résumé Exécutif

Architecture microservices complète pour un système de camion-benne autonome et drones collecteurs. Le design couvre l'orchestration des missions, la gestion de flotte, la prédiction de trajectoire et la télémétrie temps réel. L'architecture propose une stack moderne avec PostgreSQL/TimescaleDB, Kafka, Redis et Kubernetes.

**Verdict global: APPROUVÉ AVEC RESERVES**  
Score: 7.5/10

---

## Points de Vue des Experts

### 🔷 EXPERT 1: Architecte Données (PostgreSQL, TimescaleDB, Scalabilité)

#### Points Forts

| Aspect | Évaluation |
|--------|------------|
| **TimescaleDB** | Excellent choix pour la télémétrie temps réel. Les hypertables avec chunk_interval=1min pour telemetry et 1h pour positions permettent un compromis optimal entre granularité et performance. |
| **Schéma PostgreSQL** | Conception DDD mature avec types ENUM explicites, index GIN pour arrays, et jointures LATERA optimisées pour les vues temps réel. |
| **Rétention** | Stratégie de chunks claire permettant compaction/retention policy efficace. |
| **Event sourcing** | Table events bien conçue pour audit trail et replay avec index composite approprié. |
| **Modélisation géospatiale** | PostGIS + coordonnées DECIMAL avec précision suffisante (10,8 / 11,8). |

#### Points Faibles

| Aspect | Problème | Gravité |
|--------|----------|---------|
| **Index manquants** | Pas d'index sur `missions.created_by`, ni sur `mission_steps.step_type` | Moyenne |
| ** foreign keys incomplètes** | `assigned_drone_ids` est un UUID[] sans FK - intégrité référentielle non garantie | Élevée |
| **JSONB surutilisé** | Colonnes comme `metadata` everywhere - fragmentation et performances dégradées | Moyenne |
| **Vue v_drone_current_status** | LATERAL subqueries = N+1 queries latentes.时应考虑 materialized views. | Moyenne |
| **Partitionnement** | Pas de logique de partitionnement explicite par région/date pour les hypertables à l'échelle 10TB | Élevée |
| **TimescaleDB compress** | Compression non activée - coût stockage 3-10x supérieur au nécessaire | Moyenne |

#### Recommandations

1. **Activer TimescaleDB compression** avec `chunk_time_interval` plus long post-compression:
   ```sql
   ALTER TABLE drone_telemetry SET (
     timescaledb.compress,
     timescaledb.compress_segmentby = 'drone_id'
   );
   SELECT add_compression_policy('drone_telemetry', INTERVAL '7 days');
   ```

2. **Materialiser les vues de statut** avec refresh toutes les 30 secondes via cron:
   ```sql
   CREATE MATERIALIZED VIEW drone_current_status AS
   SELECT d.id, d.serial_number, ...
   FROM drones d LEFT JOIN LATERAL (...) dp ON true
   WITH DATA;
   ```

3. **Ajouter partitionsgeo** par région si multi-sites:
   ```sql
   SELECT add_retention_policy('drone_positions', INTERVAL '90 days');
   ```

4. **Corriger intégrité référentielle** - utiliser table de liaison `mission_drones`:
   ```sql
   CREATE TABLE mission_drones (
     mission_id UUID REFERENCES missions(id),
     drone_id UUID REFERENCES drones(id),
     PRIMARY KEY (mission_id, drone_id)
   );
   ```

---

### 🔷 EXPERT 2: Spécialiste DevOps/Cloud (Kubernetes, Helm, CI/CD)

#### Points Forts

| Aspect | Évaluation |
|--------|------------|
| **HPA bien conçu** | Métriques composites (CPU + queue depth) pour mission-service - bonne approche. |
| **Multi-région** | Architecture DR avec PostgreSQL streaming replication, Redis Cluster, Kafka MirrorMaker2. |
| **Probes configurées** | liveness + readiness avec seuils appropriés (30s/10s initial). |
| **Ressources définies** | Requests/limits explicites, bon point de départ pour scheduler. |
| **Vault intégration** | Secret management externe prévu - bonnes pratiques. |
| **Logging structuré** | Format JSON avec trace_id, span_id -compatible OpenTelemetry. |

#### Points Faibles

| Aspect | Problème | Gravité |
|--------|----------|---------|
| **Helm non détaillé** | Only raw K8s manifests - pas de Charts structurés, pas de values.yaml | Moyenne |
| **CI/CD absent** | Pas de pipeline défini (GitLab CI, GitHub Actions, ArgoCD?) | Élevée |
| **Istio seulement** | mTLS policy détaillée mais pas de service mesh complet - comment injection, routing? | Moyenne |
| **Monitoring partiel** | Prometheus rules définies mais pas d'AlertManager config, ni de PodDisruptionBudget | Moyenne |
| **Database ops** | Pas de StatefulSet pour PostgreSQL, pas de Operator déployé | Élevée |
| **Backup/Restore** | Pas de stratégie documentée (pg_dump, wal-g, Velero?) | Élevée |
| **Ingress** | Pas de configuration Ingress/ Gateway API | Moyenne |

#### Recommandations

1. **Structurer en Helm Charts**:
   ```bash
   # Structure recommandée
   charts/
   ├── backend/
   │   ├── Chart.yaml
   │   ├── values.yaml
   │   ├── templates/
   │   │   ├── deployment.yaml
   │   │   ├── service.yaml
   │   │   ├── hpa.yaml
   │   │   └── servicemonitor.yaml
   │   └── values-prod.yaml
   ├── postgres/
   │   └── (utiliser cloudnative-pg ou timescaledb-operator)
   └── kafka/
       └── (utiliser strimzi-kafka-operator)
   ```

2. **Définir pipeline CI/CD** (exemple GitLab CI):
   ```yaml
   stages:
     - build
     - test
     - scan
     - build-image
     - deploy
     
   build-image:
     script:
       - docker build -t $IMAGE:$VERSION
       - docker push $IMAGE:$VERSION
       
   deploy:
     stage: deploy
     script:
       - helm upgrade --install release charts/backend -f charts/backend/values-$ENV.yaml
     when: manual
   ```

3. **Ajouter PodDisruptionBudget** pour services critiques:
   ```yaml
   apiVersion: policy/v1
   kind: PodDisruptionBudget
   metadata:
     name: mission-service-pdb
   spec:
     minAvailable: 2
     selector:
       matchLabels:
         app: mission-service
   ```

4. **Implémenter ArgoCD** pour GitOps:
   ```yaml
   apiVersion: argoproj.io/v1alpha1
   kind: Application
   metadata:
     name: backend-prod
   spec:
     source:
       repoURL: https://github.com/org/backend
       path: charts/backend
       targetRevision: main
     destination:
       server: https://kubernetes.default.svc
       namespace: production
   ```

---

### 🔷 EXPERT 3: Expert Sécurité (Chiffrement, Authentification, Conformité RGPD)

#### Points Forts

| Aspect | Évaluation |
|--------|------------|
| **TLS 1.3** | Standard moderne, bonne choix. |
| **JWT bien implémenté** | Tokens séparés (access/refresh), expiration courte (15min), JTI pour révocation. |
| **mTLS prévu** | Istio PeerAuthentication STRICT - excellent pour zero-trust. |
| **Vault** | Secrets management centralisé, rotation prévue. |
| **WAF** | Couche filtrage SQLi, XSS, CSRF documentée. |
| **Audit trail** | Table events avec metadata JSONB - traçabilité OK. |

#### Points Faibles

| Aspect | Problème | Gravité |
|--------|----------|---------|
| **Hash mots de passe** | Algorithme non spécifié - devrait être bcrypt/argon2, pas de salt visible | Élevée |
| **RGPD: rétention** | Pas de politique de purge utilisateurs, pas de "droit à l'oubli" implémenté | Élevée |
| **RGPD: consentement** | Pas de gestion consentements (opt-in/out pour données collectées) | Élevée |
| **RGPD:DPO** | Pas de mention DPO, pas de registre traitements | Moyenne |
| **Chiffrement BDD** | pgcrypto mentionné mais pas de colonnes chiffrées explicitement (emails, localisations) | Moyenne |
| **Rate limiting** | Limite par minute (100) insuffisante pour API publique - facilement bypassable | Moyenne |
| **Secrets dans code** | Pas de mention de _external secrets operator_ pour injection Vault | Moyenne |
| **Logging données sensibles** | Pas de scrubber pour logs - risk de fuite PII dans ELK | Moyenne |

#### Recommandations

1. **Renforcer protocole auth**:
   ```python
   # Utiliser Argon2id pour hash mots de passe
   from argon2 import PasswordHasher
   ph = PasswordHasher(
       time_cost=3,
       memory_cost=64 * 1024,
       parallelism=4,
       hash_len=32,
       type=Type.ID
   )
   ```

2. **Implémenter conformité RGPD**:

   a) Droit à l'effacement (Article 17):
   ```sql
   CREATE OR REPLACE FUNCTION delete_user_data(user_id UUID)
   RETURNS VOID AS $$
   BEGIN
     -- Anonymiser données personnelles
     UPDATE users 
     SET email = 'deleted-' || id || '@anonymized.local',
         first_name = 'DELETED',
         last_name = 'DELETED',
         phone = NULL,
         avatar_url = NULL,
         metadata = '{}'
     WHERE id = user_id;
     
     -- Supprimer données location
     DELETE FROM drone_positions WHERE drone_id IN (
       SELECT id FROM drones WHERE owner_id = user_id
     );
   END;
   $$ LANGUAGE plpgsql SECURITY DEFINER;
   ```

   b) Registre des traitements (Article 30):
   ```yaml
   # treatments registry
   treatments:
     - id: user_management
       data_controller: OpsTeam
       purpose: Account management
       legal_basis: consent
       data_categories: [email, name, phone]
       retention: 5 years after deletion
       
     - id: fleet_telemetry
       data_controller: OpsTeam
       purpose: Fleet monitoring & safety
       legal_basis: legitimate_interest
       data_categories: [gps, telemetry, video]
       retention: 90 days then aggregated
   ```

3. **Configurer external secrets**:
   ```yaml
   apiVersion: external-secrets.io/v1beta1
   kind: ClusterExternalSecret
   metadata:
     name: vault-secrets
   spec:
     storeRef:
       kind: SecretStore
       name: vault-backend
     secretStoreRef:
       name: hashicorp-vault
       kind: SecretStore
     refreshInterval: 1h
     template:
       data:
         DATABASE_URL: "{{ .DATABASE_URL }}"
         JWT_SECRET: "{{ .JWT_SECRET }}"
   ```

4. **Masquer PII dans logs**:
   ```python
   import logging
   import re
   
   class PIIFilter(logging.Filter):
       PATTERNS = [
           (r'\b[\w.-]+@[\w.-]+\.\w+\b', '[EMAIL]'),
           (r'\b\d{3}[-.]?\d{3}[-.]?\d{4}\b', '[PHONE]'),
           (r'"latitude":\s*-?\d+\.?\d+', '"latitude": [REDACTED]'),
       ]
       
       def filter(self, record):
           record.msg = self._redact(str(record.msg))
           return True
   ```

---

## Tableau Récapitulatif

| Critère | Expert 1 (Data) | Expert 2 (DevOps) | Expert 3 (Security) |
|---------|-----------------|-------------------|---------------------|
| **Score** | 7/10 | 6.5/10 | 6/10 |
| **Verdict** | Approouvé | Approouvé avec réserves | Approouvé avec réserves |
| **Bloquant** | Partitionnement 10TB absent | CI/CD absent | Hash password non sécurisé |
| **Majeur** | Intégrité référentielle | Backup/Restore | Conformité RGPD |
| **Mineur** | Compression TimescaleDB | Helm Charts | Rate limiting |

---

## Verdict Final

### APPROUVÉ AVEC RESERVES

L'architecture backend est **solide et bien pensée**, démontrant une compréhension approfondie des patterns microservices, de la gestion temps réel et de la scalabilité. La combinaison PostgreSQL+TimescaleDB est particulièrement pertinente pour le cas d'usage (télémétrie haute fréquence).

### Conditions d'approbation

Avant mise en production, les corrections suivantes sont **obligatoires**:

1. 🔴 **Sécurité**: Implémenter hash Argon2bcrypt pour mots de passe
2. 🔴 **RGPD**: Ajouter pipeline "droit à l'effacement" et registre traitements
3. 🔴 **Data**: Corriger intégrité référentielle sur missions ↔ drones
4. 🟠 **DevOps**: Implémenter pipeline CI/CD avec scanning sécurité
5. 🟠 **DevOps**: Définir stratégie backup/restore pour PostgreSQL et Kafka

### Recommandations post-validation

- **Court terme** (Sprint 1-2): Corrections bloquantes ci-dessus
- **Moyen terme** (Sprint 3-4): TimescaleDB compression, Helm Charts production-ready
- **Long terme** (Trimestre 2): Migration vers Operator PostgreSQL, Service Mesh complet avec observabilité

---

*Document généré par comité d'experts - Architecture Review Board*