# Architecture backend cloud - Système de Camion-Benne Autonome et Drones Collecteurs

## Vue d'ensemble du Système

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                                    SYSTEM ARCHITECTURE                                   │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│    ┌──────────────┐                              ┌──────────────┐                      │
│    │  Mobile App  │                              │   Web App    │                      │
│    │   (iOS/      │                              │  (Dashboard │                      │
│    │   Android)   │                              │   Admin)     │                      │
│    └──────┬───────┘                              └──────┬───────┘                      │
│           │                                             │                               │
│           └─────────────────┬───────────────────────────┘                               │
│                             │                                                           │
│                      ┌──────▼───────┐                                                   │
│                      │  API Gateway │                                                   │
│                      │   (Kong/     │                                                   │
│                      │   Traefik)   │                                                   │
│                      └──────┬───────┘                                                   │
│                             │                                                           │
│           ┌─────────────────┼─────────────────┐                                        │
│           │                 │                 │                                           │
│    ┌──────▼──────┐  ┌───────▼───────┐  ┌──────▼──────┐                                  │
│    │  Auth Svc   │  │ Mission Svc   │  │ Telemetry   │                                  │
│    │  (OAuth2/   │  │ (Orchestra-   │  │   Svc       │                                  │
│    │   JWT)      │  │   tion)       │  │ (Ingestion) │                                  │
│    └──────┬──────┘  └───────┬───────┘  └──────┬──────┘                                  │
│           │                │                │                                           │
│    ┌──────▼──────┐  ┌───────▼───────┐  ┌──────▼──────┐                                  │
│    │  Fleet Svc  │  │Trajectory Svc │  │  User Svc   │                                  │
│    │  (Drones)   │  │ (Prediction)  │  │  (Admin)    │                                  │
│    └──────┬──────┘  └───────┬───────┘  └─────────────┘                                  │
│           │                │                                                           │
│           └────────┬───────┘                                                           │
│                    │                                                                    │
│           ┌───────▼───────┐                                                            │
│           │  Coordination │                                                            │
│           │     Svc       │                                                            │
│           └───────┬───────┘                                                            │
│                   │                                                                    │
│    ┌──────────────┼─────────────────────────────────┐                                   │
│    │              │                                 │                                   │
│┌───▼───┐    ┌─────▼─────┐                    ┌─────▼─────┐                            │
│PostgreSQL│    │   Redis  │                    │  Kafka    │                            │
│+Timescale│    │  (Cache) │                    │  (Events) │                            │
└─────────┘    └───────────┘                    └───────────┘                            │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 1. Architecture Microservices

### 1.1 Découplage par Domaine (DDD)

| Service | Responsabilité | Technologie | Port |
|---------|----------------|-------------|------|
| **auth-service** | Authentification, JWT, OAuth2 | Node.js/TypeScript | 3001 |
| **mission-service** | Orchestration des missions, planification | Go | 3002 |
| **fleet-service** | Gestion des drones, statut, santé | Python/FastAPI | 3003 |
| **truck-service** | Gestion du camion-benne, position, capacité | Python/FastAPI | 3004 |
| **trajectory-service** | Prédiction de trajectoire (EKF + LSTM) | Python/PyTorch | 3005 |
| **telemetry-service** | Ingestion temps réel, buffering | Go | 3006 |
| **coordination-service** | Sync inter-services, mutex distribué | Go | 3007 |
| **user-service** | Gestion utilisateurs, RBAC | Node.js/TypeScript | 3008 |
| **notification-service** | Alertes, push notifications | Node.js | 3009 |

### 1.2 Communication

```
┌─────────────────────────────────────────────────────────────────┐
│                    COMMUNICATION PATTERNS                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  SYNC (Requête/Réponse)              ASYNC (Event-Driven)       │
│  ─────────────────────────           ──────────────────────      │
│                                                                  │
│  Client → API Gateway ─────────►     Producer → Kafka ──────►   │
│       │                              │              │           │
│       │    gRPC                      │              ▼           │
│       ▼                              │         Consumer         │
│  Service A ───────────► Service B    │              │           │
│       │            (inter-service)   │              ▼           │
│       │                              │         Service(s)        │
│  REST/GraphQL                       │              │           │
│       │                              │              ▼           │
│       ▼                              │        Elasticsearch     │
│  Endpoints clients                  │        / Prometheus      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. API REST/GraphQL

### 2.1 API Gateway (Kong)

```yaml
# kong.yaml - Configuration
services:
  - name: auth-service
    url: http://auth-service:3001
    routes:
      - name: auth-routes
        paths: [/api/v1/auth]
    plugins:
      - name: jwt
      - name: rate-limiting
        config:
          minute: 100

  - name: mission-service
    url: http://mission-service:3002
    routes:
      - name: mission-routes
        paths: [/api/v1/missions]
    plugins:
      - name: jwt
      - name: correlation-id

  - name: fleet-service
    url: http://fleet-service:3003
    routes:
      - name: fleet-routes
        paths: [/api/v1/fleet]
    plugins:
      - name: jwt

  - name: telemetry-service
    url: http://telemetry-service:3006
    routes:
      - name: telemetry-routes
        paths: [/api/v1/telemetry]
    plugins:
      - name: jwt
        config:
          header: X-Client-Cert
```

### 2.2 Endpoints REST

#### Auth Service
```
POST   /api/v1/auth/register          - Inscription utilisateur
POST   /api/v1/auth/login            - Connexion (retourne JWT)
POST   /api/v1/auth/refresh          - Refresh token
POST   /api/v1/auth/logout            - Déconnexion
GET    /api/v1/auth/me                - Profil courant
PUT    /api/v1/auth/me                - Mise à jour profil
POST   /api/v1/auth/password-reset    - Reset mot de passe
```

#### Mission Service
```
POST   /api/v1/missions               - Créer mission
GET    /api/v1/missions               - Liste missions (paginé)
GET    /api/v1/missions/{id}          - Détail mission
PUT    /api/v1/missions/{id}          - Modifier mission
DELETE /api/v1/missions/{id}          - Annuler mission
POST   /api/v1/missions/{id}/start    - Démarrer mission
POST   /api/v1/missions/{id}/pause    - Pause mission
POST   /api/v1/missions/{id}/complete - Terminer mission
GET    /api/v1/missions/active        - Missions actives
GET    /api/v1/missions/scheduled     - Missions planifiées
```

#### Fleet Service (Drones)
```
GET    /api/v1/fleet/drones           - Liste tous drones
GET    /api/v1/fleet/drones/{id}      - Détail drone
PUT    /api/v1/fleet/drones/{id}      - Mise à jour drone
GET    /api/v1/fleet/drones/{id}/status   - Statut temps réel
POST   /api/v1/fleet/drones/{id}/command  - Envoyer commande
GET    /api/v1/fleet/drones/available     - Drones disponibles
GET    /api/v1/fleet/drones/nearby        - Drones près d'un point
POST   /api/v1/fleet/drones/{id}/maintenance   - Programmation maintenance
```

#### Truck Service
```
GET    /api/v1/trucks                 - Liste tous camions
GET    /api/v1/trucks/{id}            - Détail camion
GET    /api/v1/trucks/{id}/status     - Statut temps réel
PUT    /api/v1/trucks/{id}            - Mise à jour camion
GET    /api/v1/trucks/{id}/position   - Position actuelle
GET    /api/v1/trucks/{id}/capacity   - Capacité actuelle benne
POST   /api/v1/trucks/{id}/command    - Envoyer commande
GET    /api/v1/trucks/active          - Camion actif
```

#### Trajectory Service
```
POST   /api/v1/trajectory/predict     - Prédire trajectoire
GET    /api/v1/trajectory/drone/{id}  - Historique trajectoire drone
GET   /api/v1/trajectory/truck/{id}   - Historique trajectoire camion
POST   /api/v1/trajectory/optimize    - Optimiser route
GET    /api/v1/trajectory/collision-check  - Vérif collision
```

#### Telemetry Service
```
POST   /api/v1/telemetry/ingest      - Ingestion données (batch)
GET    /api/v1/telemetry/drone/{id}  - Télémétrie drone
GET    /api/v1/telemetry/truck/{id}  - Télémétrie camion
GET    /api/v1/telemetry/mission/{id} - Télémétrie mission
GET    /api/v1/telemetry/realtime/{id}   - WebSocket stream
```

#### User Service
```
GET    /api/v1/users                  - Liste utilisateurs (admin)
GET    /api/v1/users/{id}             - Détail utilisateur
PUT    /api/v1/users/{id}             - Modifier utilisateur
DELETE /api/v1/users/{id}             - Supprimer utilisateur
GET    /api/v1/users/{id}/roles       - Rôles utilisateur
PUT    /api/v1/users/{id}/roles       - Assigner rôles
GET    /api/v1/roles                  - Liste rôles
POST   /api/v1/roles                  - Créer rôle
```

### 2.3 Schéma GraphQL (Optionnel)

```graphql
type Query {
  # Missions
  missions(filter: MissionFilter, limit: Int, offset: Int): MissionConnection!
  mission(id: ID!): Mission
  
  # Flotte
  drones(filter: DroneFilter): DroneConnection!
  drone(id: ID!): Drone
  
  # Camions
  trucks: TruckConnection!
  truck(id: ID!): Truck
  
  # Trajectoire
  trajectory(entityId: ID!, from: DateTime!, to: DateTime!): [Position!]!
  
  # Utilisateurs
  users(role: String): UserConnection!
  me: User!
}

type Mutation {
  # Auth
  login(email: String!, password: String!): AuthPayload!
  register(input: RegisterInput!): AuthPayload!
  
  # Missions
  createMission(input: MissionInput!): Mission!
  updateMission(id: ID!, input: MissionInput!): Mission!
  deleteMission(id: ID!): Boolean!
  startMission(id: ID!): Mission!
  
  # Drones
  updateDrone(id: ID!, input: DroneInput!): Drone!
  sendDroneCommand(id: ID!, command: DroneCommand!): CommandResult!
  
  # Camions
  updateTruck(id: ID!, input: TruckInput!): Truck!
  sendTruckCommand(id: ID!, command: TruckCommand!): CommandResult!
}

type Subscription {
  missionUpdated(id: ID!): Mission!
  droneStatusChanged: DroneStatus!
  truckPositionUpdated: Position!
  telemetryUpdated(entityId: ID!): TelemetryPoint!
}
```

---

## 3. Base de Données

### 3.1 Schéma PostgreSQL (avec TimescaleDB)

```sql
-- =====================================================
-- SCHEMA POSTGRESQL + TIMESCALEDB
-- =====================================================

-- Extensions
CREATE EXTENSION IF NOT EXISTS timescaledb;
CREATE EXTENSION IF NOT EXISTS postgis;
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- =====================================================
-- TABLES PRINCIPALES
-- =====================================================

-- Utilisateurs
-- [CORRECTION 1] - Hash de mot de passe sécurisé avec bcrypt
-- Les mots de passe sont hashés avec bcrypt (cost factor 12)
-- Le champ stocke le hash au format: $2b$12$<salt><hash>
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    first_name VARCHAR(100),
    last_name VARCHAR(100),
    phone VARCHAR(20),
    avatar_url TEXT,
    role user_role DEFAULT 'operator',
    status user_status DEFAULT 'active',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    last_login_at TIMESTAMPTZ,
    metadata JSONB DEFAULT '{}',
    -- [CORRECTION 2] RGPD - Champ pour le droit à l'effacement
    deleted_at TIMESTAMPTZ,
    consent_marketing BOOLEAN DEFAULT FALSE,
    consent_analytics BOOLEAN DEFAULT TRUE
);

CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_role ON users(role);
-- Index pour soft delete RGPD
CREATE INDEX idx_users_deleted ON users(deleted_at) WHERE deleted_at IS NOT NULL;

-- Rôles
CREATE TABLE roles (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(50) UNIQUE NOT NULL,
    permissions TEXT[] DEFAULT '{}',
    description TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Permissions
CREATE TABLE permissions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(100) UNIQUE NOT NULL,
    resource VARCHAR(50) NOT NULL,
    action VARCHAR(50) NOT NULL,
    description TEXT
);

-- =====================================================
-- GESTION DE LA FLOTTE DE DRONES
-- =====================================================

CREATE TABLE drones (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    serial_number VARCHAR(100) UNIQUE NOT NULL,
    model VARCHAR(100) NOT NULL,
    manufacturer VARCHAR(100),
    firmware_version VARCHAR(50),
    status drone_status DEFAULT 'grounded',
    battery_capacity_wh INT,
    max_payload_g INT,
    max_speed_kmh DECIMAL(6,2),
    operational_range_km DECIMAL(6,2),
    capabilities TEXT[] DEFAULT '{}',
    registration_date DATE,
    last_maintenance_at TIMESTAMPTZ,
    next_maintenance_at TIMESTAMPTZ,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE drone_positions (
    id BIGSERIAL PRIMARY KEY,
    drone_id UUID REFERENCES drones(id) ON DELETE CASCADE,
    latitude DECIMAL(10,8) NOT NULL,
    longitude DECIMAL(11,8) NOT NULL,
    altitude_m DECIMAL(8,2),
    heading DECIMAL(5,2),
    speed_kmh DECIMAL(6,2),
    accuracy_m DECIMAL(5,2),
    timestamp TIMESTAMPTZ DEFAULT NOW()
);

-- TimescaleDB hypertable
SELECT create_hypertable('drone_positions', 'timestamp', 
    chunk_time_interval => INTERVAL '1 hour');

CREATE INDEX idx_drone_pos_drone_time ON drone_positions(drone_id, timestamp DESC);

-- Télémétrie drone
CREATE TABLE drone_telemetry (
    id BIGSERIAL PRIMARY KEY,
    drone_id UUID REFERENCES drones(id) ON DELETE CASCADE,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    
    -- Batterie
    battery_percent INT CHECK (battery_percent BETWEEN 0 AND 100),
    battery_voltage_mv INT,
    battery_temperature_c DECIMAL(5,2),
    battery_health_percent INT,
    
    -- Mouvement
    velocity_x DECIMAL(8,4),
    velocity_y DECIMAL(8,4),
    velocity_z DECIMAL(8,4),
    acceleration_x DECIMAL(8,4),
    acceleration_y DECIMAL(8,4),
    acceleration_z DECIMAL(8,4),
    rotation_roll DECIMAL(7,4),
    rotation_pitch DECIMAL(7,4),
    rotation_yaw DECIMAL(7,4),
    
    -- Environnement
    temperature_c DECIMAL(5,2),
    pressure_hpa INT,
    humidity_percent INT,
    wind_speed_kmh DECIMAL(6,2),
    wind_direction_deg DECIMAL(5,2),
    
    -- Santé système
    cpu_usage_percent INT,
    memory_usage_percent INT,
    gps_satellite_count INT,
    signal_strength_dbm INT,
    
    -- Métadonnées
    metadata JSONB DEFAULT '{}'
);

SELECT create_hypertable('drone_telemetry', 'timestamp',
    chunk_time_interval => INTERVAL '1 minute');

CREATE INDEX idx_drone_tel_drone_time ON drone_telemetry(drone_id, timestamp DESC);

-- =====================================================
-- GESTION DU CAMION-BENNE
-- =====================================================

CREATE TABLE trucks (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    vin VARCHAR(50) UNIQUE NOT NULL,
    license_plate VARCHAR(20) UNIQUE NOT NULL,
    model VARCHAR(100) NOT NULL,
    manufacturer VARCHAR(100),
    year INT,
    status truck_status DEFAULT 'idle',
    truck_type truck_type DEFAULT 'dump_truck',
    
    -- Spécifications techniques
    max_capacity_kg INT,
    current_load_kg INT DEFAULT 0,
    fuel_type fuel_type DEFAULT 'diesel',
    fuel_level_percent INT DEFAULT 100,
    
    --localisation
    current_latitude DECIMAL(10,8),
    current_longitude DECIMAL(11,8),
    heading DECIMAL(5,2),
    speed_kmh DECIMAL(6,2),
    
    -- Capteurs
    has_lidar BOOLEAN DEFAULT TRUE,
    has_radar BOOLEAN DEFAULT TRUE,
    has_camera BOOLEAN DEFAULT TRUE,
    sensor_config JSONB DEFAULT '{}',
    
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE truck_positions (
    id BIGSERIAL PRIMARY KEY,
    truck_id UUID REFERENCES trucks(id) ON DELETE CASCADE,
    latitude DECIMAL(10,8) NOT NULL,
    longitude DECIMAL(11,8) NOT NULL,
    altitude_m DECIMAL(8,2),
    heading DECIMAL(5,2),
    speed_kmh DECIMAL(6,2),
    timestamp TIMESTAMPTZ DEFAULT NOW()
);

SELECT create_hypertable('truck_positions', 'timestamp',
    chunk_time_interval => INTERVAL '1 hour');

CREATE INDEX idx_truck_pos_truck_time ON truck_positions(truck_id, timestamp DESC);

-- Télémétrie camion
CREATE TABLE truck_telemetry (
    id BIGSERIAL PRIMARY KEY,
    truck_id UUID REFERENCES trucks(id) ON DELETE CASCADE,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    
    -- Propulsion
    engine_rpm INT,
    engine_temperature_c DECIMAL(5,2),
    transmission_gear INT,
    torque_nm DECIMAL(7,2),
    
    -- Batterie (si électrique)
    battery_percent INT,
    battery_voltage_v DECIMAL(6,2),
    motor_power_kw DECIMAL(6,2),
    
    -- Carrosserie benne
    bed_angle_deg DECIMAL(5,2),
    bed_load_kg INT,
    bed_load_percent INT,
    
    -- Sécurité
    tire_pressure_fl INT,
    tire_pressure_fr INT,
    tire_pressure_rl INT,
    tire_pressure_rr INT,
    brake_temperature_c DECIMAL(5,2),
    
    -- Détection
    lidar_points INT,
    radar_objects INT,
    camera_objects INT,
    
    -- Santé
    odometer_km DECIMAL(10,2),
    fuel_consumed_l DECIMAL(7,2),
    
    metadata JSONB DEFAULT '{}'
);

SELECT create_hypertable('truck_telemetry', 'timestamp',
    chunk_time_interval => INTERVAL '1 minute');

CREATE INDEX idx_truck_tel_truck_time ON truck_telemetry(truck_id, timestamp DESC);

-- =====================================================
-- MISSIONS
-- =====================================================

CREATE TABLE missions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(255) NOT NULL,
    description TEXT,
    mission_type mission_type NOT NULL,
    status mission_status DEFAULT 'pending',
    priority mission_priority DEFAULT 'normal',
    
    -- Planification
    scheduled_start TIMESTAMPTZ,
    scheduled_end TIMESTAMPTZ,
    actual_start TIMESTAMPTZ,
    actual_end TIMESTAMPTZ,
    duration_minutes INT,
    
    -- Zone géographique
    area_geojson JSONB,
    waypoints JSONB,
    
    -- Assignment
    -- [CORRECTION 3] Contrainte FK sur assigned_drone_ids
    -- La colonne est un tableau UUID avec contrainte de vérification
    assigned_drone_ids UUID[] DEFAULT '{}',
    assigned_truck_id UUID REFERENCES trucks(id),
    assigned_user_id UUID REFERENCES users(id),
    
    -- Contenu
    payload_type TEXT,
    payload_quantity_kg DECIMAL(8,2),
    collection_points JSONB,
    dropoff_point JSONB,
    
    -- Statistiques
    distance_covered_km DECIMAL(8,2),
    items_collected INT,
    items_failed INT,
    
    -- Erreurs
    error_message TEXT,
    error_code VARCHAR(50),
    
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    created_by UUID REFERENCES users(id)
);

-- [CORRECTION 3] Fonction de vérification FK pour assigned_drone_ids
-- Cette fonction vérifie que tous les UUIDs du tableau existent dans la table drones
CREATE OR REPLACE FUNCTION validate_assigned_drone_ids()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.assigned_drone_ids IS NOT NULL AND array_length(NEW.assigned_drone_ids, 1) > 0 THEN
        IF EXISTS (
            SELECT 1 FROM drones d
            WHERE d.id = ANY(NEW.assigned_drone_ids)
            HAVING count(DISTINCT d.id) < array_length(NEW.assigned_drone_ids, 1)
        ) THEN
            RAISE EXCEPTION 'Invalid drone ID in assigned_drone_ids: one or more IDs do not exist in drones table';
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_validate_assigned_drone_ids
    BEFORE INSERT OR UPDATE ON missions
    FOR EACH ROW
    EXECUTE FUNCTION validate_assigned_drone_ids();

CREATE INDEX idx_missions_status ON missions(status);
CREATE INDEX idx_missions_scheduled ON missions(scheduled_start);
CREATE INDEX idx_missions_type ON missions(mission_type);
CREATE INDEX idx_missions_assigned_drone USING GIN (assigned_drone_ids);

-- Étapes de mission
CREATE TABLE mission_steps (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    mission_id UUID REFERENCES missions(id) ON DELETE CASCADE,
    step_order INT NOT NULL,
    step_type step_type NOT NULL,
    status step_status DEFAULT 'pending',
    
    -- Localisation
    target_latitude DECIMAL(10,8),
    target_longitude DECIMAL(11,8),
    target_altitude_m DECIMAL(8,2),
    arrival_radius_m DECIMAL(6,2),
    
    -- Action
    action JSONB,
    expected_duration_seconds INT,
    
    -- Timing
    scheduled_time TIMESTAMPTZ,
    actual_start_time TIMESTAMPTZ,
    actual_end_time TIMESTAMPTZ,
    
    -- Résultat
    completion_percent INT DEFAULT 0,
    success BOOLEAN,
    notes TEXT
);

CREATE INDEX idx_mission_steps_mission ON mission_steps(mission_id, step_order);

-- =====================================================
-- TRAJECTOIRES PRÉDITES
-- =====================================================

CREATE TABLE predicted_trajectories (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    entity_type entity_type NOT NULL,
    entity_id UUID NOT NULL,
    mission_id UUID REFERENCES missions(id),
    
    -- Prédiction
    predicted_positions JSONB NOT NULL,
    confidence_score DECIMAL(4,3),
    model_version VARCHAR(50),
    prediction_horizon_seconds INT,
    
    -- Contexte
    weather_conditions JSONB,
    traffic_conditions JSONB,
    
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_pred_traj_entity ON predicted_trajectories(entity_type, entity_id, created_at DESC);

-- =====================================================
-- ÉVÉNEMENTS (EVENT SOURCING)
-- =====================================================

CREATE TABLE events (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    aggregate_type VARCHAR(100) NOT NULL,
    aggregate_id UUID NOT NULL,
    event_type VARCHAR(100) NOT NULL,
    event_version INT DEFAULT 1,
    payload JSONB NOT NULL,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_events_aggregate ON events(aggregate_type, aggregate_id, created_at DESC);
CREATE INDEX idx_events_type ON events(event_type, created_at DESC);

-- =====================================================
-- [CORRECTION 2] RGPD - REGISTRE DES TRAITEMENTS
-- =====================================================

CREATE TABLE rgpd_processing_registry (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    processing_name VARCHAR(255) NOT NULL,
    processing_type VARCHAR(50) NOT NULL, -- 'automated', 'manual'
    purpose TEXT NOT NULL,
    legal_basis VARCHAR(100) NOT NULL, -- 'consent', 'contract', 'legal_obligation', 'legitimate_interest'
    data_categories TEXT[] NOT NULL,
    data_subjects VARCHAR(50) NOT NULL, -- 'users', 'operators', 'drones', etc.
    recipients TEXT[] DEFAULT '{}',
    third_country_transfers BOOLEAN DEFAULT FALSE,
    retention_period VARCHAR(50),
    security_measures TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- [CORRECTION 2] RGPD - Journal des consentements
CREATE TABLE rgpd_consent_log (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id),
    consent_type VARCHAR(50) NOT NULL,
    granted BOOLEAN NOT NULL,
    ip_address INET,
    user_agent TEXT,
    recorded_at TIMESTAMPTZ DEFAULT NOW()
);

-- [CORRECTION 2] RGPD - Demandes d'exercice des droits
CREATE TABLE rgpd_rights_requests (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id),
    request_type VARCHAR(50) NOT NULL, -- 'access', 'rectification', 'erasure', 'portability', 'objection'
    status request_status DEFAULT 'pending', -- 'pending', 'processing', 'completed', 'rejected'
    request_details JSONB,
    response_details JSONB,
    requested_at TIMESTAMPTZ DEFAULT NOW(),
    completed_at TIMESTAMPTZ,
    handled_by UUID REFERENCES users(id)
);

-- =====================================================
-- TYPES ENUM
-- =====================================================

CREATE TYPE user_role AS ENUM ('admin', 'operator', 'viewer', 'maintenance');
CREATE TYPE user_status AS ENUM ('active', 'inactive', 'suspended');
CREATE TYPE drone_status AS ENUM ('grounded', 'pre_flight', 'flying', 'returning', 'maintenance', 'error');
CREATE TYPE truck_status AS ENUM ('idle', 'moving', 'loading', 'unloading', 'maintenance', 'error');
CREATE TYPE truck_type AS ENUM ('dump_truck', 'flatbed', 'tanker', 'roll_off');
CREATE TYPE fuel_type AS ENUM ('diesel', 'electric', 'hybrid');
CREATE TYPE mission_type AS ENUM ('collection', 'delivery', 'survey', 'inspection', 'transport');
CREATE TYPE mission_status AS ENUM ('pending', 'scheduled', 'in_progress', 'paused', 'completed', 'failed', 'cancelled');
CREATE TYPE mission_priority AS ENUM ('low', 'normal', 'high', 'urgent');
CREATE TYPE step_type AS ENUM ('takeoff', 'waypoint', 'collection', 'delivery', 'landing', 'wait', 'charge');
CREATE TYPE step_status AS ENUM ('pending', 'in_progress', 'completed', 'skipped', 'failed');
CREATE TYPE entity_type AS ENUM ('drone', 'truck');
CREATE TYPE request_status AS ENUM ('pending', 'processing', 'completed', 'rejected');

-- =====================================================
-- VUES ET FONCTIONS UTILITAIRES
-- =====================================================

-- Vue: Statut actuel des drones
CREATE VIEW v_drone_current_status AS
SELECT 
    d.id, d.serial_number, d.model, d.status,
    dp.latitude, dp.longitude, dp.altitude_m, dp.heading, dp.speed_kmh,
    dt.battery_percent, dt.temperature_c, dt.cpu_usage_percent,
    dp.timestamp as last_position_time
FROM drones d
LEFT JOIN LATERAL (
    SELECT * FROM drone_positions 
    WHERE drone_id = d.id 
    ORDER BY timestamp DESC LIMIT 1
) dp ON true
LEFT JOIN LATERAL (
    SELECT * FROM drone_telemetry 
    WHERE drone_id = d.id 
    ORDER BY timestamp DESC LIMIT 1
) dt ON true;

-- Vue: Statut actuel des camions
CREATE VIEW v_truck_current_status AS
SELECT 
    t.id, t.vin, t.license_plate, t.model, t.status,
    tp.latitude, tp.longitude, tp.heading, tp.speed_kmh,
    tt.fuel_level_percent, tt.bed_load_kg, tt.bed_load_percent,
    tp.timestamp as last_position_time
FROM trucks t
LEFT JOIN LATERAL (
    SELECT * FROM truck_positions 
    WHERE truck_id = t.id 
    ORDER BY timestamp DESC LIMIT 1
) tp ON true
LEFT JOIN LATERAL (
    SELECT * FROM truck_telemetry 
    WHERE truck_id = t.id 
    ORDER BY timestamp DESC LIMIT 1
) tt ON true;

-- [CORRECTION 2] RGPD - Fonction pour le droit à l'effacement
CREATE OR REPLACE FUNCTION anonymize_user_data(p_user_id UUID)
RETURNS VOID AS $$
BEGIN
    -- Anonymiser les données personnelles
    UPDATE users 
    SET 
        first_name = 'ANONYMIZED',
        last_name = 'ANONYMIZED',
        phone = NULL,
        avatar_url = NULL,
        email = CONCAT('deleted-', p_user_id, '@anonymous.local'),
        deleted_at = NOW()
    WHERE id = p_user_id;
    
    -- Supprimer les données de consentement
    DELETE FROM rgpd_consent_log WHERE user_id = p_user_id;
    
    -- Conserver la trace de la demande pour conformité
    -- (les missions et données associées sont conservées pour obligation légale)
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;
```

### 3.2 Redis (Cache & Sessions)

```json
{
  "keys": {
    "session:{user_id}": "JWT refresh token",
    "drone:{drone_id}:status": "cached drone status",
    "truck:{truck_id}:status": "cached truck status",
    "mission:{mission_id}:lock": "distributed lock",
    "rate_limit:{user_id}:{endpoint}": "rate limit counter",
    "geo:drone:nearby": "geospatial index for nearby drones"
  },
  "ttl": {
    "session": "7 days",
    "status": "5 seconds",
    "lock": "30 seconds",
    "rate_limit": "1 minute"
  }
}
```

---

## 4. Orchestrateur de Missions

### 4.1 Diagramme de Flux

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         MISSION ORCHESTRATION FLOW                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│    ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐                  │
│    │  Client  │────►│  API     │────►│Mission  │────►│ Validate │                  │
│    │ Request  │     │Gateway   │     │Service  │     │ Request  │                  │
│    └──────────┘     └──────────┘     └──────────┘     └────┬─────┘                  │
│                                                            │                        │
│                              ┌─────────────────────────────┼─────────────────────┐  │
│                              │                             │                     │  │
│                              ▼                             ▼                     ▼  │
│                       ┌─────────────┐              ┌─────────────┐        ┌──────────┐│
│                       │  Valid      │              │  Invalid    │        │   Queue  ││
│                       │  ✓          │              │  ✗          │        │   Kafka  ││
│                       └──────┬──────┘              └─────────────┘        └────┬─────┘│
│                              │                                                  │       │
│                              ▼                                                  ▼       │
│                    ┌──────────────────┐                            ┌───────────────┤       │
│                    │  Check Resource  │                            │               │       │
│                    │  Availability    │                            │   Consume     │       │
│                    └────────┬─────────┘                            │   & Process   │       │
│                             │                                      └───────┬───────┘       │
│              ┌──────────────┼──────────────┐                               │               │
│              ▼              ▼              ▼                               ▼               │
│       ┌───────────┐  ┌───────────┐  ┌───────────┐                                              │
│       │  Drones   │  │  Trucks   │  │  Neither │                                              │
│       │Available │  │Available  │  │Available │                                              │
│       └─────┬─────┘  └─────┬─────┘  └─────┬─────┘                                              │
│             │             │             │                                                   │
│             ▼             ▼             ▼                                                   │
│    ┌─────────────────────────────┐                                                         │
│    │   Assign Resources & Create │                                                         │
│    │   Mission Record            │                                                         │
│    └─────────────┬───────────────┘                                                         │
│                  │                                                                         │
│                  ▼                                                                         │
│    ┌─────────────────────────────┐                                                         │
│    │   Publish MissionEvent      │                                                         │
│    │   (Kafka)                   │                                                         │
│    └─────────────┬───────────────┘                                                         │
│                  │                                                                         │
│      ┌───────────┴───────────┐                                                             │
│      ▼                       ▼                                                             │
│ ┌─────────────┐      ┌─────────────┐                                                       │
│ │ Fleet Svc   │      │ Truck Svc   │                                                       │
│ │ (Drones)    │      │             │                                                       │
│ └──────┬──────┘      └──────┬──────┘                                                       │
│        │                    │                                                              │
│        │         ┌──────────┴──────────┐                                                   │
│        │         │   Coordination Svc  │                                                   │
│        │         │   (Mission Sync)    │                                                   │
│        │         └──────────┬──────────┘                                                   │
│        │                    │                                                             │
│        ▼                    ▼                                                             │
│   ┌─────────────────────────────────────┐                                                 │
│   │    Execute Mission Steps           │                                                 │
│   │    (Waypoint navigation)            │                                                 │
│   └──────────────────┬──────────────────┘                                                 │
│                      │                                                                    │
│                      ▼                                                                    │
│         ┌────────────────────────┐                                                        │
│         │ Update Mission Status │                                                        │
│         │ & Telemetry           │                                                        │
│         └────────────────────────┘                                                        │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Algorithme d'Assignment

```python
# mission_service/assignment.py

from dataclasses import dataclass
from typing import List, Optional
import numpy as np

@dataclass
class DroneScore:
    drone_id: str
    score: float
    distance_km: float
    battery_percent: float
    availability_hours: float

async def assign_drones_to_mission(
    mission: Mission,
    available_drones: List[Drone],
    required_count: int
) -> List[Drone]:
    """
    Algorithme d'assignation multi-critères:
    - Distance au point de départ
    - Niveau de batterie (doit être > 30%)
    - Capacité de charge requise
    - Disponibilité (pas de mission concurrente)
    - Score composite pondéré
    """
    
    scores = []
    mission_origin = mission.collection_points[0]
    
    for drone in available_drones:
        if drone.status != 'grounded':
            continue
        if drone.battery_percent < 30:
            continue
        if drone.max_payload_g < mission.payload_quantity_kg * 1000:
            continue
            
        # Calcul distance
        distance_km = haversine(drone.position, mission_origin)
        
        # Score composite (plus haut = meilleur)
        score = (
            0.35 * (1 - distance_km / MAX_DISTANCE) +    # Proximité
            0.30 * (drone.battery_percent / 100) +         # Batterie
            0.20 * (drone.reliability_score) +             # Fiabilité historique
            0.15 * (1 - drone.maintenance_due_days / 90)  # Maintenance imminente
        )
        
        scores.append(DroneScore(
            drone_id=drone.id,
            score=score,
            distance_km=distance_km,
            battery_percent=drone.battery_percent,
            availability_hours=drone.estimated_available_hours
        ))
    
    # Sélection des meilleurs candidats
    scores.sort(key=lambda x: x.score, reverse=True)
    return scores[:required_count]
```

### 4.3 Machine à États de Mission

```
┌─────────┐
│ PENDING │ ──[validate]──► ┌──────────┐
└────┬────┘                  │SCHEDULED│
     │                       └────┬─────┘
     │                            │ ──[start]──► ┌───────────┐
     │                            │              │IN_PROGRESS│
     │                            │              └─────┬─────┘
     │                            │                    │
     │                            │    ┌───────────────┼───────────────┐
     │                            │    │               │               │
     │                            │    ▼               ▼               ▼
     │                            │ ┌──────┐     ┌─────────┐    ┌──────┐
     │                            │ │PAUSED│     │COMPLETED│    │FAILED│
     │                            │ └──────┘     └─────────┘    └──────┘
     │                            │    │
     │                            │    └──[resume]──► IN_PROGRESS
     │                            │
     └──[cancel]──► ┌────────────┐│
                   │ CANCELLED   ││
                   └─────────────┘│
```

---

## 5. Gestion des Drones

### 5.1 Modèle de données drone

```python
# fleet_service/models.py

from enum import Enum
from pydantic import BaseModel
from typing import List, Optional
from datetime import datetime

class DroneStatus(str, Enum):
    GROUNDED = "grounded"
    PRE_FLIGHT = "pre_flight"
    FLYING = "flying"
    RETURNING = "returning"
    MAINTENANCE = "maintenance"
    ERROR = "error"

class Drone(BaseModel):
    id: str
    serial_number: str
    model: str
    manufacturer: str
    firmware_version: str
    status: DroneStatus
    
    # Spécifications
    battery_capacity_wh: int
    max_payload_g: int
    max_speed_kmh: float
    operational_range_km: float
    capabilities: List[str]
    
    # Position actuelle
    position: Optional[Position]
    battery_percent: int
    health_score: float
    
    # Maintenance
    last_maintenance_at: datetime
    next_maintenance_at: datetime
    flight_hours_total: int

class Position(BaseModel):
    latitude: float
    longitude: float
    altitude_m: float
    heading: float
    speed_kmh: float
    accuracy_m: float
    timestamp: datetime

class DroneCommand(BaseModel):
    command_type: str  # takeoff, land, goto, hover, return_to_base
    parameters: dict
    priority: int = 5
    timeout_seconds: int = 60
```

### 5.2 Gestion de la santé (Health Manager)

```
┌─────────────────────────────────────────────────────────────────┐
│                    DRONE HEALTH MANAGEMENT                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │  Battery   │    │   Motors    │    │  Sensors    │         │
│  │  Manager   │    │   Manager   │    │   Manager   │         │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘         │
│         │                  │                  │                 │
│         ▼                  ▼                  ▼                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              HEALTH AGGREGATOR                           │   │
│  │  - weighted score: battery(40%), motors(30%), sensors(30%)│   │
│  │  - seuils: OK(>80%), Warning(50-80%), Critical(<50%)     │   │
│  └─────────────────────────┬───────────────────────────────┘   │
│                            │                                    │
│                            ▼                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              AUTOMATED ACTIONS                           │   │
│  │  - Warning: Notifier opérateur, suggérer maintenance    │   │
│  │  - Critical: Retour automatique à la base, interdire vol│   │
│  │  - Error: Mode sans échec, transmission données diagnostic│  │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.3 Protocole de communication drone

```
┌─────────────────────────────────────────────────────────────────┐
│                  DRONE COMMUNICATION PROTOCOL                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  [Drone] <───── TLS 1.3 ─────> [Gateway] <────> [Backend]       │
│                                                                  │
│  Frame: {                                                       │
│    "msg_type": "TELEMETRY|STATUS|COMMAND|ACK",                  │
│    "drone_id": "uuid",                                          │
│    "timestamp": "ISO8601",                                      │
│    "sequence": 12345,                                           │
│    "payload": { ... }                                           │
│  }                                                              │
│                                                                  │
│  Heartbeat: toutes les 2 secondes                               │
│  Timeout: 10 secondes sans heartbeat → drone.mark_error()       │
│                                                                  │
│  Command Queue (drone):                                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Queue commandes avec acknowledge (ACK) pour chaque cmd   │   │
│  │ Retry automatique si pas d'ACK après 3 tentatives        │   │
│  │ File d'urgence prioritaire (ex: emergency_land)          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6. Gestion du Camion

### 6.1 Modèle de données camion

```python
# truck_service/models.py

class TruckStatus(str, Enum):
    IDLE = "idle"
    MOVING = "moving"
    LOADING = "loading"
    UNLOADING = "unloading"
    MAINTENANCE = "maintenance"
    ERROR = "error"

class Truck(BaseModel):
    id: str
    vin: str
    license_plate: str
    model: str
    manufacturer: str
    year: int
    status: TruckStatus
    truck_type: str
    
    # Capacités
    max_capacity_kg: int
    current_load_kg: int
    
    # Localisation
    position: Position
    route: Optional[Route]
    
    # État technique
    fuel_level_percent: int
    health_score: float
    tire_pressure: dict
    brake_status: str

class DumpBedState(BaseModel):
    angle_deg: float           # Angle de la benne (0 = abaissée)
    is_raised: bool           # Benne levée?
    load_kg: int              # Charge actuelle
    load_percent: int         # Pourcentage de capacité
    max_angle_deg: float      # Angle max de déversement
```

### 6.2 Gestion de la benne

```
┌─────────────────────────────────────────────────────────────────┐
│                    DUMP BED CONTROL                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Opérateur ──► API ──► Truck Svc ──► CAN Bus ──► Benne          │
│                                                                  │
│  Commandes:                                                     │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │ RAISE_BED    → Lever la benne pour déversement           │ │
│  │ LOWER_BED    → Abaisser la benne                          │ │
│  │ SET_ANGLE    → Angle spécifique (0-60°)                  │ │
│  │ LOCK_BED     → Verrouiller benne (transport)             │ │
│  │ UNLOCK_BED   → Déverrouiller benne                       │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                  │
│  Capteurs:                                                       │
│  - Capteur de charge (pesage)                                   │
│  - Capteur d'angle (potentiomètre)                              │
│  - Capteur de verrouillage                                       │
│  - Limiteurs de course (sécurité)                                │
│                                                                  │
│  Sécurité:                                                       │
│  - Interdiction de lever si charge < 100kg                      │
│  - Interdiction de rouler si benne non verrouillée              │
│  - Alerte si surcharge                                          │                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Prédiction de Trajectoire

### 7.1 Architecture du modèle

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      TRAJECTORY PREDICTION PIPELINE                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  INPUTS                                                                              │
│  ───────                                                                             │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐                  │
│  │  Position  │  │  Velocity  │  │  Weather   │  │   Map      │                  │
│  │  History   │  │  Vector    │  │  Forecast  │  │  Data      │                  │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘                  │
│        │               │               │               │                           │
│        └───────────────┼───────────────┼───────────────┘                           │
│                        ▼               ▼                                            │
│              ┌─────────────────┐ ┌─────────────────┐                                │
│              │ Extended Kalman │ │  LSTM Network  │                                │
│              │    Filter      │ │  (Prediction)  │                                │
│              │ (State Est.)   │ │                │                                │
│              └────────┬────────┘ └────────┬────────┘                                │
│                       │                    │                                          │
│                       │    ┌───────────────┼───────────────┐                         │
│                       │    │               │               │                          │
│                       │    ▼               ▼               ▼                          │
│                       │ ┌─────────────────────────────────────────────┐              │
│                       │ │         ENSEMBLE COMBINER                   │              │
│                       │ │  - Weighted average based on confidence    │              │
│                       │ │  - EKF weight: 0.4, LSTM weight: 0.6      │              │
│                       │ └──────────────────────┬──────────────────────┘              │
│                       │                        │                                       │
│                       ▼                        ▼                                       │
│              ┌─────────────────────────────────────────────────┐                      │
│              │              OUTPUT                              │                      │
│              │  - Predicted trajectory (N points)              │                      │
│              │  - Confidence interval (uncertainty)           │                      │
│              │  - ETA to each waypoint                         │                      │
│              │  - Collision risk assessment                    │                      │
│              └─────────────────────────────────────────────────┘                      │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Modèles ML

```python
# trajectory_service/models.py

import torch
import torch.nn as nn

class TrajectoryLSTM(nn.Module):
    """
    LSTM pour prédiction de trajectoire à horizon de 30 secondes.
    Entrée: séquence de positions/timestamps (últimas 20 observations)
    Sortie: 30 positions prédites avec intervalles de confiance
    """
    
    def __init__(self, input_dim=6, hidden_dim=128, num_layers=2, output_horizon=30):
        super().__init__()
        self.lstm = nn.LSTM(
            input_size=input_dim,
            hidden_size=hidden_dim,
            num_layers=num_layers,
            batch_first=True,
            dropout=0.2
        )
        self.fc_mean = nn.Linear(hidden_dim, output_horizon * 2)  # lat, lon
        self.fc_var = nn.Linear(hidden_dim, output_horizon * 2)   # variance
        
    def forward(self, x):
        # x: (batch, seq_len, input_dim)
        lstm_out, _ = self.lstm(x)
        last_out = lstm_out[:, -1, :]  # (batch, hidden_dim)
        
        mean = self.fc_mean(last_out)  # (batch, output_horizon * 2)
        var = torch.exp(self.fc_var(last_out))  # variance positive
        
        return mean, var


class ExtendedKalmanFilter:
    """
    Filtre de Kalman étendu pour estimation d'état.
    Utilisé pour fusionner les données GPS + IMU.
    """
    
    def __init__(self, state_dim=6, measurement_dim=4):
        self.state_dim = state_dim
        self.x = np.zeros(state_dim)  # [x, y, vx, vy, ax, ay]
        self.P = np.eye(state_dim)    # covariance
        self.Q = np.eye(state_dim)    # process noise
        self.R = np.eye(measurement_dim)  # measurement noise
        
    def predict(self, dt):
        # Propagation d'état
        F = np.array([
            [1, 0, dt, 0,  dt**2/2, 0],
            [0, 1, 0, dt,  0,        dt**2/2],
            [0, 0, 1, 0,  dt,         0],
            [0, 0, 0, 1,  0,         dt],
            [0, 0, 0, 0,  1,         0],
            [0, 0, 0, 0,  0,         1],
        ])
        self.x = F @ self.x
        self.P = F @ self.P @ F.T + self.Q
        
    def update(self, z):
        # Mise à jour avec mesure
        H = np.array([
            [1, 0, 0, 0, 0, 0],  # x
            [0, 1, 0, 0, 0, 0],  # y
            [0, 0, 1, 0, 0, 0],  # vx
            [0, 0, 0, 1, 0, 0],  # vy
        ])
        y = z - H @ self.x
        S = H @ self.P @ H.T + self.R
        K = self.P @ H.T @ np.linalg.inv(S)
        self.x = self.x + K @ y
        self.P = (np.eye(self.state_dim) - K @ H) @ self.P
```

### 7.3 API de prédiction

```python
# trajectory_service/api.py

@app.post("/api/v1/trajectory/predict")
async def predict_trajectory(request: TrajectoryPredictRequest):
    """
    Prédit la trajectoire future d'une entité.
    """
    # Récupérer historique positions
    history = await telemetry_store.get_positions(
        entity_id=request.entity_id,
        from_timestamp=datetime.now() - timedelta(minutes=5),
        to_timestamp=datetime.now()
    )
    
    # Prédiction EKF + LSTM
    ekf_state = ekf_filter.predict(current_state)
    lstm_pred = lstm_model.predict(sequence_history)
    
    # Fusion ensembliste
    final_trajectory = ensemble_combine(ekf_state, lstm_pred)
    
    # Vérification collisions
    collisions = await check_collisions(final_trajectory)
    
    return TrajectoryResponse(
        trajectory=final_trajectory.points,
        confidence=final_trajectory.confidence,
        collision_risks=collisions,
        model_version="2.1.0"
    )
```

---

## 8. Gestion Utilisateurs & RBAC

### 8.1 Modèle de permissions

```
┌─────────────────────────────────────────────────────────────────┐
│                      ROLE-BASED ACCESS CONTROL                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ROLES                                                                      │
│  ──────                                                                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │  ADMIN  │  │OPERATOR │  │ VIEWER  │  │MAINTENANCE│            │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘              │
│       │            │            │            │                   │
│  ┌────┴────┐  ┌────┴────┐  ┌────┴────┐  ┌────┴────┐             │
│  │ ALL     │  │ CRUD    │  │ READ    │  │ CRUD    │             │
│  │ PERMS   │  │ missions│  │ all     │  │ drones  │             │
│  │         │  │ READ    │  │         │  │ trucks │             │
│  │         │  │ fleet   │  │         │  │ READ    │             │
│  │         │  │         │  │         │  │ missions│             │
│  │         │  │         │  │         │  │         │             │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘             │
│                                                                  │
│  PERMISSION MATRIX                                                │
│  ───────────────                                                  │
│  ┌──────────────────┬───────┬─────────┬───────┬───────────┐      │
│  │ Resource         │ ADMIN │OPERATOR │VIEWER │MAINTENANCE│      │
│  ├──────────────────┼───────┼─────────┼───────┼───────────┤      │
│  │ missions:*       │   ✓   │    ✓    │   -   │     -     │      │
│  │ missions:create  │   ✓   │    ✓    │   -   │     -     │      │
│  │ missions:read    │   ✓   │    ✓    │   ✓   │     ✓     │      │
│  │ missions:update  │   ✓   │    ✓    │   -   │     -     │      │
│  │ missions:delete  │   ✓   │    -    │   -   │     -     │      │
│  ├──────────────────┼───────┼─────────┼───────┼───────────┤      │
│  │ drones:*         │   ✓   │    ✓    │   -   │     ✓     │      │
│  │ drones:command   │   ✓   │    ✓    │   -   │     -     │      │
│  ├──────────────────┼───────┼─────────┼───────┼───────────┤      │
│  │ trucks:*        │   ✓   │    ✓    │   -   │     ✓     │      │
│  │ trucks:command   │   ✓   │    ✓    │   -   │     -     │      │
│  ├──────────────────┼───────┼─────────┼───────┼───────────┤      │
│  │ users:*          │   ✓   │    -    │   -   │     -     │      │
│  │ telemetry:*      │   ✓   │    ✓    │   ✓   │     ✓     │      │
│  └──────────────────┴───────┴─────────┴───────┴───────────┘      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 8.2 Schéma d'authentification

```python
# auth_service/jwt.py
# [CORRECTION 1] - Utilisation de bcrypt pour le hash des mots de passe

from datetime import datetime, timedelta
from typing import Optional
import jwt
import bcrypt

class PasswordHasher:
    """Gestion du hash de mot de passe avec bcrypt"""
    
    def __init__(self, rounds: int = 12):
        self.rounds = rounds  # Cost factor 12 (recommandé)
    
    def hash_password(self, plain_password: str) -> str:
        """Hash un mot de passe en clair avec bcrypt"""
        salt = bcrypt.gensalt(rounds=self.rounds)
        hashed = bcrypt.hashpw(plain_password.encode('utf-8'), salt)
        return hashed.decode('utf-8')
    
    def verify_password(self, plain_password: str, hashed_password: str) -> bool:
        """Vérifie un mot de passe contre son hash bcrypt"""
        return bcrypt.checkpw(
            plain_password.encode('utf-8'), 
            hashed_password.encode('utf-8')
        )

class JWTManager:
    def __init__(self, secret_key: str, algorithm: str = "HS256"):
        self.secret_key = secret_key
        self.algorithm = algorithm
        self.access_token_expire = 15  # minutes
        self.refresh_token_expire = 7  # days
        
    def create_access_token(self, user: User) -> str:
        payload = {
            "sub": user.id,
            "email": user.email,
            "role": user.role,
            "iat": datetime.utcnow(),
            "exp": datetime.utcnow() + timedelta(minutes=self.access_token_expire),
            "type": "access"
        }
        return jwt.encode(payload, self.secret_key, algorithm=self.algorithm)
    
    def create_refresh_token(self, user: User) -> str:
        payload = {
            "sub": user.id,
            "iat": datetime.utcnow(),
            "exp": datetime.utcnow() + timedelta(days=self.refresh_token_expire),
            "type": "refresh",
            "jti": str(uuid.uuid4())  # Unique identifier for revocation
        }
        return jwt.encode(payload, self.secret_key, algorithm=self.algorithm)
    
    def verify_token(self, token: str) -> Optional[dict]:
        try:
            return jwt.decode(token, self.secret_key, algorithms=[self.algorithm])
        except jwt.ExpiredSignatureError:
            return None
        except jwt.InvalidTokenError:
            return None
```

---

## 9. Sécurité

### 9.1 Architecture de sécurité

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         SECURITY ARCHITECTURE                                        │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            INTERNET                                            │  │
│  └────────────────────────────────────┬────────────────────────────────────────────┘  │
│                                       │                                               │
│                                       ▼                                               │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │  WAF (Web Application Firewall)                                            │  │
│  │  - Filtrage SQL injection, XSS, CSRF                                        │  │
│  │  - Rate limiting par IP                                                      │  │
│  │  - Geo-blocking (optionnel)                                                 │  │
│  └────────────────────────────────────┬────────────────────────────────────────────┘  │
│                                       │                                               │
│                                       ▼                                               │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │  Load Balancer / API Gateway                                                 │  │
│  │  - TLS termination                                                          │  │
│  │  - Certificate management (LetsEncrypt / Vault)                            │  │
│  │  - mTLS pour communication inter-services                                    │  │
│  └────────────────────────────────────┬────────────────────────────────────────────┘  │
│                                       │                                               │
│       ┌───────────────────────────────┼───────────────────────────────────────┐      │
│       │                               │                                       │      │
│       ▼                               ▼                                       ▼      │
│  ┌─────────┐                    ┌─────────┐                              ┌─────────┐│
│  │  Auth  │                    │ Service │                              │  Kafka ││
│  │  Svc   │                    │ Mesh    │                              │  TLS   ││
│  └─────────┘                    └─────────┘                              └─────────┘│
│                                       │                                               │
│                                       ▼                                               │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                          VAULT (Secrets Management)                         │  │
│  │  - Chiffrement au repos                                                     │  │
│  │  - Rotation automatique des clés                                            │  │
│  │  - Audit logs                                                               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 9.2 Configuration mTLS

```yaml
# istio/mtls-policy.yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: default
spec:
  mtls:
    mode: STRICT
---
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: service-authz
spec:
  selector:
    matchLabels:
      app: backend-service
  rules:
  - from:
    - source:
        principals:
          - "cluster.local/ns/default/sa/auth-service"
          - "cluster.local/ns/default/sa/mission-service"
          - "cluster.local/ns/default/sa/fleet-service"
    to:
    - operation:
        methods: ["GET", "POST", "PUT", "DELETE"]
```

### 9.3 Chiffrement

| Layer | Technology | Details |
|-------|------------|---------|
| **Transport** | TLS 1.3 | Certificats renouvelés auto (Let's Encrypt) |
| **Application** | JWT | Access tokens signés HS256/RS256 |
| **Database** | pgcrypto | Chiffrement colonnes sensibles |
| **Passwords** | bcrypt | Cost factor 12 (protection mot de passe) |
| **Secrets** | Vault | AES-256 pour secrets au repos |
| **Messages** | TLS + SASL | Kafka with SCRAM-SHA-512 |

---

## 10. Logs et Télémétrie

### 10.1 Architecture de logging

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         LOGGING & TELEMETRY PIPELINE                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  SOURCES                                                                     │
│  ───────                                                                     │
│  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐            │
│  │ Apps   │  │  K8s   │  │  NGINX │  │  DB    │  │  API   │  │  IoT   │            │
│  │ (JSON) │  │  (logs)│  │ (access)│  │ (audit)│  │(request)│ │ (device)│           │
│  └───┬────┘  └───┬────┘  └───┬────┘  └───┬────┘  └───┬────┘  └───┬────┘            │
│      │            │            │            │            │            │              │
│      └────────────┼────────────┼────────────┼────────────┼────────────┘              │
│                   │            │            │            │                          │
│                   ▼            ▼            ▼            ▼                          │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           KAFKA (Event Bus)                                  │  │
│  │  Topics: app-logs, access-logs, audit-logs, telemetry, metrics              │  │
│  │  Partitions: 3, Replication: 3                                               │  │
│  └────────────────────────────────────┬───────────────────────────────────────────┘  │
│                                        │                                            │
│                                        ▼                                            │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                        LOG AGGREGATION                                        │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                        │  │
│  │  │   Fluentd   │  │   Filebeat   │  │   Promtail  │                        │  │
│  │  │  (parsers)  │  │  (containers)│  │  (static)  │                        │  │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                        │  │
│  │         └─────────────────┼─────────────────┘                                 │  │
│  └──────────────────────────┼───────────────────────────────────────────────────┘  │
│                              │                                                      │
│                              ▼                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                      STORAGE & VISUALIZATION                                  │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                       │  │
│  │  │Elasticsearch│  │   Prometheus │  │    Grafana   │                       │  │
│  │  │   (logs)     │  │   (metrics)   │  │  (dashboards)│                       │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘                       │  │
│  │  ┌──────────────┐  ┌──────────────┐                                           │  │
│  │  │     S3       │  │    Jaeger    │                                           │  │
│  │  │  (archives)  │  │  (tracing)  │                                           │  │
│  │  └──────────────┘  └──────────────┘                                           │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 10.2 Format de log standardisé

```json
{
  "timestamp": "2026-03-31T22:38:00.000Z",
  "level": "INFO",
  "service": "mission-service",
  "trace_id": "abc123-def456-ghi789",
  "span_id": "span-001",
  "parent_id": "span-000",
  "message": "Mission created successfully",
  "context": {
    "mission_id": "uuid-1234",
    "user_id": "uuid-5678",
    "mission_type": "collection"
  },
  "metrics": {
    "duration_ms": 45
  }
}
```

### 10.3 Métriques Prometheus

```yaml
# prometheus/rules/metrics.yaml
groups:
  - name: application
    rules:
      - alert: HighErrorRate
        expr: |
          sum(rate(http_requests_total{status=~"5.."}[5m])) 
          / sum(rate(http_requests_total[5m])) > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High error rate detected"
          
      - alert: HighLatency
        expr: histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m])) > 2
        for: 5m
        labels:
          severity: warning
          
  - name: infrastructure
    rules:
      - alert: HighCPUUsage
        expr: sum(rate(process_cpu_seconds_total[5m])) by (pod) > 0.8
        for: 10m
        labels:
          severity: warning
          
      - alert: HighMemoryUsage
        expr: container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.85
        for: 10m
        labels:
          severity: warning

  - name: drone_metrics
    rules:
      - alert: DroneLowBattery
        expr: drone_battery_percent < 20
        for: 2m
        labels:
          severity: warning
          
      - alert: DroneConnectionLost
        expr: absence(drone_heartbeat{drone_id=~".*"}) for 30s
        labels:
          severity: critical
```

### 10.4 Dashboards Grafana

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         GRAFANA DASHBOARD OVERVIEW                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────┬────────────────────────────────┐               │
│  │     FLIGHT OPERATIONS          │       SYSTEM HEALTH             │               │
│  │  ┌────────┐ ┌────────┐ ┌────┐  │  ┌────────┐ ┌────────┐ ┌────┐ │               │
│  │  │Active  │ │Completed│ │Failed│  │ CPU    │ │ Memory │ │Disk │ │               │
│  │  │Drones  │ │Today    │ │Today │  │ Usage  │ │ Usage  │ │Usage│ │               │
│  │  │   12   │ │   47    │ │  3  │  │  45%   │ │  62%   │ │ 38% │ │               │
│  │  └────────┘ └────────┘ └────┘  │  └────────┘ └────────┘ └────┘ │               │
│  └────────────────────────────────┴────────────────────────────────┘               │
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────┐     │
│  │                      REAL-TIME MAP (Drone Positions)                        │     │
│  │  ┌─────────────────────────────────────────────────────────────────────┐  │     │
│  │  │                                                                     │  │     │
│  │  │         🛸 drone-001 ────────────► 🛸 drone-003                    │  │     │
│  │  │            │                                                      │  │     │
│  │  │            │                                                      │  │     │
│  │  │    🚛 truck-001 ◄───────────────►                               │  │     │
│  │  │                                                                     │  │     │
│  │  └─────────────────────────────────────────────────────────────────────┘  │     │
│  └───────────────────────────────────────────────────────────────────────────┘     │
│                                                                                      │
│  ┌────────────────────────────────┬────────────────────────────────┐               │
│  │     BATTERY LEVELS             │       ALERTS                   │               │
│  │  ┌──────────────────────────┐  │  ┌──────────────────────────┐  │               │
│  │  │ drone-001: ████████ 85%  │  │  │ ⚠️ drone-005 low battery│  │               │
│  │  │ drone-002: ██████ 62%    │  │  │ ⚠️ truck-002 maintenance │  │               │
│  │  │ drone-003: ████ 45%      │  │  │ ⚠️ weather warning      │  │               │
│  │  │ drone-004: ████ 42%      │  │  │ ✅ All systems nominal  │  │               │
│  │  └──────────────────────────┘  │  └──────────────────────────┘  │               │
│  └────────────────────────────────┴────────────────────────────────┘               │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 11. Scalabilité

### 11.1 Stratégie de mise à l'échelle

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         SCALABILITY STRATEGY                                          │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  HORIZONTAL SCALING (Kubernetes HPA)                                  METRICS        │
│  ───────────────────────────────                                  ─────────        │
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │  Service          Metric                    Min    Max    Target            │   │
│  │  ──────────────────────────────────────────────────────────────────────────  │   │
│  │  auth-service     CPU usage                 2      10     70%                │   │
│  │  mission-service  Queue depth               2      20     100 msg/pod        │   │
│  │  fleet-service    Connection count          3      30     500 conn/pod       │   │
│  │  telemetry-svc    Throughput                5      50     10K events/sec     │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│  VERTICAL SCALING (ressources)                                                      │
│  ───────────────────────────                                                        │
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐       │
│  │  Service          CPU            Memory       Storage      Network         │       │
│  │  ───────────────────────────────────────────────────────────────────────  │       │
│  │  postgres         4 cores        16 GB        500 GB SSD   1 Gbps          │       │
│  │  redis            2 cores        8 GB         50 GB        1 Gbps          │       │
│  │  kafka            4 cores        16 GB        1 TB         10 Gbps         │       │
│  │  elasticsearch    8 cores        32 GB        2 TB         10 Gbps         │       │
│  └──────────────────────────────────────────────────────────────────────────────┘       │
│                                                                                      │
│  AUTO-SCALING BEHAVIOR                                                              │
│  ──────────────────────                                                             │
│                                                                                      │
│  1. Request spike detected ──► Scale out (add pods)                               │
│  2. Traffic normalizes ──► Scale in after 5 min idle                              │
│  3. Memory leak detected ──► Restart pod + alert                                   │
│  4. Database bottleneck ──► Add read replicas                                      │
│  5. Kafka lag increasing ──► Add partitions + consumers                           │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 11.2 Architecture multi-régions (Disaster Recovery)

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                     MULTI-REGION ARCHITECTURE                                        │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│                         ┌─────────────────┐                                         │
│                         │   Global DNS    │                                         │
│                         │   (Route 53)    │                                         │
│                         └────────┬────────┘                                         │
│                                  │                                                  │
│                    ┌─────────────┴─────────────┐                                    │
│                    │                           │                                    │
│               ┌────▼────┐                ┌─────▼─────┐                             │
│               │ PRIMARY │                │  STANDBY  │                             │
│               │ eu-west │                │ us-east   │                             │
│               └────┬────┘                └─────┬─────┘                             │
│                    │                           │                                    │
│         ┌──────────┼──────────┐      ┌─────────┼─────────┐                        │
│         │          │          │      │         │         │                         │
│         ▼          ▼          ▼      ▼         ▼         ▼                         │
│   ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐         │
│   │  Kong   │ │  Auth   │ │Mission  │ │  Kong   │ │  Auth   │ │Mission  │         │
│   │ Gateway │ │ Service │ │ Service │ │ Gateway │ │ Service │ │ Service │         │
│   └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘         │
│        │           │           │           │           │           │               │
│        │     ┌─────┴─────┐     │           │     ┌─────┴─────┐     │               │
│        │     │  PostgreSQL│     │           │     │  PostgreSQL│     │               │
│        │     │ (Master)  │     │           │     │ (Replica) │     │               │
│        │     └─────┬─────┘     │           │     └───────────┘     │               │
│        │           │           │           │                       │               │
│        │     ┌─────┴─────┐     │           │     ┌───────────┐     │               │
│        │     │   Redis  │     │           │     │   Redis   │     │               │
│        │     │  Cache   │     │           │     │  Cache    │     │               │
│        │     └─────────┘     │           │     └───────────┘     │               │
│        │                     │           │                       │               │
│        │     ┌───────────┐   │           │     ┌───────────┐     │               │
│        │     │  Kafka    │   │  Sync     │     │  Kafka    │     │               │
│        │     │  Cluster │◄──┼───────────┼────►│  Mirror   │     │               │
│        │     └───────────┘   │           │     └───────────┘     │               │
│        │                     │           │                       │               │
│        │  Active Region      │           │  Standby (failover)   │               │
│        └─────────────────────┘           └───────────────────────┘               │
│                                                                                      │
│  REPLICATION                                                            SYNC         │
│  ──────────                                                              ────        │
│  PostgreSQL: Async streaming replication                                  │         │
│  Redis: Redis Cluster with replica nodes                                  │         │
│  Kafka: MirrorMaker2 for cross-region replication                        │         │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 11.3 Plan de capacity

| Composant | Requête initiale | 6 mois | 1 an | 3 ans |
|-----------|-----------------|--------|------|-------|
| **Utilisateurs actifs** | 100 | 500 | 2,000 | 10,000 |
| **Drones** | 10 | 50 | 200 | 500 |
| **Camions** | 3 | 10 | 25 | 50 |
| **Missions/jour** | 50 | 200 | 1,000 | 5,000 |
| **Événements telemetry/sec** | 1,000 | 5,000 | 20,000 | 50,000 |
| **Stockage PostgreSQL** | 100 GB | 500 GB | 2 TB | 10 TB |
| **Rétention logs** | 30 jours | 30 jours | 90 jours | 90 jours |

---

## 12. Infrastructure

### 12.1 Kubernetes Deployment

```yaml
# k8s/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mission-service
  labels:
    app: mission-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: mission-service
  template:
    metadata:
      labels:
        app: mission-service
    spec:
      containers:
      - name: mission-service
        image: registry.example.com/mission-service:v2.1.0
        ports:
        - containerPort: 3002
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: url
        - name: KAFKA_BROKERS
          value: "kafka-0:9092,kafka-1:9092,kafka-2:9092"
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "2000m"
            memory: "2Gi"
        livenessProbe:
          httpGet:
            path: /health
            port: 3002
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 3002
          initialDelaySeconds: 10
          periodSeconds: 5
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: mission-service-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: mission-service
  minReplicas: 2
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: kafka_consumer_group_lag
      target:
        type: AverageValue
        averageValue: "100"
```

---

## 13. [CORRECTION 4] Pipeline CI/CD

### 13.1 Vue d'ensemble du pipeline

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         CI/CD PIPELINE OVERVIEW                                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐            │
│  │  CODE   │───►│  BUILD  │───►│  TEST   │───►│ STAGING │───►│PRODUCTION│           │
│  │  PUSH   │    │         │    │         │    │         │    │          │           │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘    └─────────┘            │
│       │              │             │             │             │                     │
│       ▼              ▼             ▼             ▼             ▼                     │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐                │
│  │  Git    │   │ Docker  │   │ Unit/   │   │Deploy to│   │ Blue/  │                │
│  │ Commit  │   │ Build   │   │Integr.  │   │ Staging │   │ Green  │                │
│  └─────────┘   └─────────┘   └─────────┘   └─────────┘   └─────────┘                │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 13.2 Étapes du pipeline

#### Étape 1: Commit & Lint
```yaml
# .github/workflows/ci.yaml
name: CI Pipeline

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Run linters
        run: |
          # ESLint for Node.js services
          npm ci && npm run lint
          # Flake8/Pylint for Python services
          pip install -r requirements-lint.txt
          flake8 .
          # go fmt for Go services
          go fmt ./...
```

#### Étape 2: Build & Scan
```yaml
  build:
    needs: lint
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Build Docker images
        run: |
          docker build -t ${{ github.repository }}/auth-service:${{ github.sha }} ./auth-service
          docker build -t ${{ github.repository }}/mission-service:${{ github.sha }} ./mission-service
          # ... autres services
      
      - name: Run Trivy security scan
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'fs'
          scan-ref: '.'
          format: 'sarif'
          output: 'trivy-results.sarif'
          
      - name: Upload results to GitHub Security
        uses: github/codeql-action/upload-sarif@v3
        with:
          sarif_file: 'trivy-results.sarif'
```

#### Étape 3: Tests unitaires et d'intégration
```yaml
  test:
    needs: build
    runs-on: ubuntu-latest
    services:
      postgres:
        image: postgres:15
        env:
          POSTGRES_PASSWORD: test
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
      redis:
        image: redis:7
    steps:
      - name: Run unit tests
        run: |
          npm test -- --coverage
          pytest --cov=. --cov-report=xml
      
      - name: Run integration tests
        run: |
          npm run test:integration
          pytest tests/integration/
```

#### Étape 4: Déploiement Staging
```yaml
  deploy-staging:
    needs: test
    if: github.ref == 'refs/heads/develop'
    runs-on: ubuntu-latest
    environment: staging
    steps:
      - name: Deploy to Kubernetes staging
        uses: azure/k8s-set-context@v4
        with:
          kubeconfig: ${{ secrets.KUBECONFIG_STAGING }}
          
      - name: Deploy application
        run: |
          kubectl apply -f k8s/staging/
          kubectl set image deployment/auth-service auth-service=${{ github.repository }}/auth-service:${{ github.sha }}
          # ... autres services
      
      - name: Run smoke tests
        run: |
          ./scripts/smoke-tests.sh staging
```

#### Étape 5: Déploiement Production
```yaml
  deploy-production:
    needs: deploy-staging
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    environment: production
    steps:
      - name: Deploy Blue/Green
        run: |
          # Déployer sur le nouveau stack
          kubectl apply -f k8s/production/
          
      - name: Run canary analysis
        run: |
          # Attendre que les métriques soient stabilisées
          sleep 60
          ./scripts/canary-check.sh production
          
      - name: Promote or rollback
        run: |
          if [ $CANARY_SUCCESS == "true" ]; then
            kubectl rollout undo deployment/auth-service
          fi
```

### 13.3 Stratégie de déploiement

| Environnement | Stratégie | Rôle |
|--------------|-----------|------|
| **Development** | Auto-commit | Tests unitaires rapides |
| **Staging** | Rolling update | Tests d'intégration, QA |
| **Production** | Blue/Green + Canary | Déploiement à risque minimal |

---

## 14. [CORRECTION 5] Stratégie Backup/Restore

### 14.1 Architecture de sauvegarde

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         BACKUP ARCHITECTURE                                           │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                          │
│  │  PostgreSQL │    │    Redis     │    │   Kafka     │                          │
│  │   Primary   │    │   Cluster    │    │  Cluster    │                          │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘                          │
│         │                    │                    │                                   │
│         ▼                    ▼                    ▼                                   │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                          │
│  │   pg_dump    │    │  RDB save    │    │ MirrorMaker2 │                          │
│  │  (Full/Diff) │    │ (Background) │    │  (Replicat.) │                          │
│  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘                          │
│         │                    │                    │                                   │
│         └────────────────────┼────────────────────┘                                   │
│                            │                                                         │
│                            ▼                                                         │
│               ┌────────────────────────┐                                             │
│               │    Object Storage     │                                             │
│               │   (S3 / MinIO)        │                                             │
│               │  ┌────────┬────────┐ │                                             │
│               │  │Hot Tier │Cold Tier│ │                                             │
│               │  │(30 days)│(1 year)│ │                                             │
│               │  └────────┴────────┘ │                                             │
│               └────────────────────────┘                                             │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 14.2 Plan de sauvegarde

| Type | Fréquence | Rétention | Cible |
|------|-----------|-----------|-------|
| **Full Backup PostgreSQL** | Quotidien (02:00 UTC) | 30 jours | S3 Hot Tier |
| **Incremental PostgreSQL** | Toutes les 6 heures | 7 jours | S3 Hot Tier |
| **WAL Archiving** | Continu | 7 jours | S3 Hot Tier |
| **RDB Redis** | Horaire | 24 heures | S3 Hot Tier |
| **AOF Redis** | Continu | 7 jours | S3 Hot Tier |
| **Kafka Topics** | Continu (MirrorMaker2) | 7 jours | Secondary Cluster |
| **Configuration** | Par commit | 1 an | Git + S3 Cold Tier |
| **Logs** | Quotidien | 90 jours | S3 Cold Tier |

### 14.3 Scripts de sauvegarde

```bash
#!/bin/bash
# scripts/backup.sh

set -e

# Configuration
S3_BUCKET="s3://backup-bucket-name"
DATE=$(date +%Y%m%d_%H%M%S)
PG_HOST="postgres.primary"
REDIS_HOST="redis.primary"

echo "[$(date)] Starting backup..."

# 1. PostgreSQL full backup
echo "[$(date)] Running PostgreSQL backup..."
pg_dump -h $PG_HOST -U backup_user -Fc -f "/tmp/db_full_$DATE.dump"
aws s3 cp "/tmp/db_full_$DATE.dump" "$S3_BUCKET/postgres/full/db_$DATE.dump"
rm "/tmp/db_full_$DATE.dump"

# 2. PostgreSQL WAL archiving (continu via pgbackrest config)
echo "[$(date)] PostgreSQL WAL archiving active"

# 3. Redis RDB backup
echo "[$(date)] Running Redis backup..."
redis-cli -h $REDIS_HOST BGSAVE
sleep 5
aws s3 cp /data/dump.rdb "$S3_BUCKET/redis/dump_$DATE.rdb"

# 4. Backup verification
echo "[$(date)] Verifying backup integrity..."
aws s3 ls "$S3_BUCKET/postgres/full/" | tail -1
aws s3 ls "$S3_BUCKET/redis/" | tail -1

echo "[$(date)] Backup completed successfully"
```

### 14.4 Configuration pgBackRest

```ini
# etc/pgbackrest.conf
[global]
repo1-s3-bucket=backup-bucket-name
repo1-s3-region=eu-west-1
repo1-type=s3
process-max=4
log-level-console=info
log-level-file=debug

[db]
db1-host=postgres.primary
db1-path=/var/lib/postgresql/data
db1-port=5432
db1-user=backup_user

[backup]
backup-type=full
retention-full=7
retention-diff=3
retention-archive=7

[restore]
recovery-target-timeline=latest
```

### 14.5 Procédure de restauration

```bash
#!/bin/bash
# scripts/restore.sh

set -e

BACKUP_DATE=$1  # Format: YYYYMMDD_HHMMSS
ENVIRONMENT=$2  # staging ou production

if [ -z "$BACKUP_DATE" ] || [ -z "$ENVIRONMENT" ]; then
    echo "Usage: $0 <backup_date> <environment>"
    echo "Example: $0 20260331_020000 production"
    exit 1
fi

S3_BUCKET="s3://backup-bucket-name"

echo "[$(date)] Starting restore for environment: $ENVIRONMENT"

# Arrêt des services
echo "[$(date)] Stopping services..."
kubectl scale deployment --replicas=0 -l app=backend

# Restauration PostgreSQL
echo "[$(date)] Restoring PostgreSQL..."
aws s3 cp "$S3_BUCKET/postgres/full/db_$BACKUP_DATE.dump" /tmp/restore.dump
pg_restore -h postgres.$ENVIRONMENT -U postgres -d postgres -c /tmp/restore.dump
rm /tmp/restore.dump

# Restauration Redis
echo "[$(date)] Restoring Redis..."
aws s3 cp "$S3_BUCKET/redis/dump_$BACKUP_DATE.rdb" /data/dump.rdb
systemctl restart redis

# Démarrage des services
echo "[$(date)] Starting services..."
kubectl scale deployment --replicas=3 -l app=backend

# Vérification
echo "[$(date)] Running health checks..."
sleep 30
kubectl get pods -l app=backend
curl -f http://api.$ENVIRONMENT/health

echo "[$(date)] Restore completed successfully"
```

### 14.6 Tests de restauration

```yaml
# .github/workflows/backup-test.yaml
name: Backup Restoration Test

on:
  schedule:
    - cron: '0 3 * * *'  # Tous les jours à 03:00
  
jobs:
  test-restore:
    runs-on: ubuntu-latest
    steps:
      - name: Restore to test environment
        run: |
          ./scripts/restore.sh $(date -d 'yesterday' +%Y%m%d) test
          
      - name: Run smoke tests
        run: |
          ./scripts/smoke-tests.sh test
          
      - name: Verify data integrity
        run: |
          psql -h postgres.test -U postgres -c "SELECT count(*) FROM users;"
          redis-cli -h redis.test INFO | grep used_memory_human
```

### 14.7 RTO/RPO Objectifs

| Métrique | Cible | Implémentation |
|----------|-------|----------------|
| **RTO** (Recovery Time Objective) | < 1 heure | PostgreSQL streaming replication + Redis replica |
| **RPO** (Recovery Point Objective) | < 1 heure | Sauvegarde toutes les heures + WAL shipping |

---

## 15. [CORRECTION 2] Conformité RGPD

### 15.1 Principes fondamentaux

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         RGPD COMPLIANCE                                             │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                     PRINCIPES RGPD                                            │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │   │
│  │  │ Licéité    │  │  Minimisation│  │   Exactitude │  │  Limitation │       │   │
│  │  │ du traitement│ │   des données │  │             │  │  de durée   │       │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                         │   │
│  │  │   Intégrité │  │ Confidentialité│  │   Accountability│                     │   │
│  │  │  et confidentialité │ │            │  │             │                     │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                         │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                     DROITS DES PERSONNES                                      │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  Accès (Art.15)  │  Rectification (Art.16)  │  Effacement (Art.17)         │   │
│  │  ┌─────────────┐  │  ┌─────────────┐        │  ┌─────────────┐            │   │
│  │  │   #user     │  │  │   #user     │        │  │  #user      │            │   │
│  │  │  /api/v1/   │  │  │  PUT /api/  │        │  │ DELETE /api │            │   │
│  │  │  /users/:id │  │  │  /users/:id │        │  │  /users/:id │            │   │
│  │  └─────────────┘  │  └─────────────┘        │  └─────────────┘            │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  Limitation (Art.18)  │  Portabilité (Art.20)  │  Opposition (Art.21)     │   │
│  │  ┌─────────────┐        │  ┌─────────────┐      │  ┌─────────────┐          │   │
│  │  │  #request  │        │  │  #user      │      │  │  #request  │          │   │
│  │  │  /api/v1/  │        │  │  GET /api/  │      │  │  /api/v1/  │          │   │
│  │  │  /users/:id│        │  │  /export    │      │  │  /users/:id│          │   │
│  │  │  /restrict │        │  └─────────────┘      │  │  /object   │          │   │
│  │  └─────────────┘        └──────────────────────┘  └─────────────┘          │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 15.2 Implémentation des droits

```python
# user_service/rgpd.py

from fastapi import APIRouter, Depends, HTTPException
from typing import List
import uuid

router = APIRouter(prefix="/api/v1/rgpd", tags=["RGPD"])

class RGPDService:
    """Service de gestion des droits RGPD"""
    
    def __init__(self, db: Database):
        self.db = db
    
    async def handle_rights_request(self, user_id: str, request_type: str) -> dict:
        """Gère les demandes d'exercice des droits"""
        
        # Enregistrer la demande
        request_record = await self.db.rgpd_rights_requests.insert({
            "user_id": user_id,
            "request_type": request_type,
            "status": "processing"
        })
        
        if request_type == "access":
            return await self._right_to_access(user_id)
        elif request_type == "erasure":
            return await self._right_to_erasure(user_id, request_record.id)
        elif request_type == "portability":
            return await self._right_to_portability(user_id)
        elif request_type == "rectification":
            return await self._right_to_rectification(user_id)
        
        raise HTTPException(status_code=400, detail="Invalid request type")
    
    async def _right_to_access(self, user_id: str) -> dict:
        """Droit d'accès (Article 15) - Extrait toutes les données"""
        
        user = await self.db.users.get(user_id)
        missions = await self.db.missions.filter(assigned_user_id=user_id)
        
        return {
            "personal_data": {
                "profile": user,
                "missions": missions,
                "consent_history": await self.db.rgpd_consent_log.filter(user_id=user_id),
                "rights_requests": await self.db.rgpd_rights_requests.filter(user_id=user_id)
            },
            "processing_info": {
                "legal_basis": "consent",
                "data_categories": ["identity", "contact", "usage"],
                "retention_period": "3 years after account deletion"
            }
        }
    
    async def _right_to_erasure(self, user_id: str, request_id: uuid.UUID) -> dict:
        """Droit à l'effacement (Article 17) - Suppression données personnelles"""
        
        # Exécuter la fonction d'anonymisation
        await self.db.execute(
            "SELECT anonymize_user_data(%s)",
            (user_id,)
        )
        
        # Mettre à jour le statut de la demande
        await self.db.rgpd_rights_requests.update(request_id, {
            "status": "completed",
            "completed_at": "now()"
        })
        
        return {
            "message": "Your personal data has been anonymized",
            "request_id": str(request_id),
            "completed_at": "now()"
        }
    
    async def _right_to_portability(self, user_id: str) -> dict:
        """Droit à la portabilité (Article 20) - Export données structurées"""
        
        user = await self.db.users.get(user_id)
        
        return {
            "format": "JSON",
            "data": {
                "user": user,
                "missions": await self.db.missions.filter(assigned_user_id=user_id)
            },
            "schema": "https://schema.example.com/v1/rgpd-export"
        }

# Endpoints RGPD
@router.post("/rights-request")
async def create_rights_request(
    request: RGPDRightsRequest,
    current_user: User = Depends(get_current_user)
):
    """Crée une demande d'exercice des droits RGPD"""
    rgpd_service = RGPDService(db)
    return await rgpd_service.handle_rights_request(
        current_user.id, 
        request.request_type
    )

@router.get("/export")
async def export_personal_data(
    current_user: User = Depends(get_current_user)
):
    """Exporte les données personnelles (droit d'accès)"""
    rgpd_service = RGPDService(db)
    return await rgpd_service._right_to_access(current_user.id)

@router.delete("/anonymize")
async def request_erasure(
    current_user: User = Depends(get_current_user)
):
    """Demande d'effacement des données"""
    rgpd_service = RGPDService(db)
    return await rgpd_service.handle_rights_request(
        current_user.id, 
        "erasure"
    )
```

### 15.3 Registre des traitements

Le registre des traitements est stocké en base de données (`rgpd_processing_registry`) et comprend:

| Traitement | Catégorie | Base légale | Rétention |
|------------|-----------|-------------|-----------|
| Authentification | Données identitité | Contract | 3 ans après suppression |
| Gestion missions | Données opérationnelles | Contract | 5 ans |
| Télémétrie drone | Données techniques | Legitimate interest | 2 ans |
| Analyse usage | Données analytics | Consent | 1 an |
| Support client | Données contact | Legal obligation | 10 ans |

---

## 16. Récapitulatif des Services

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                         SERVICES SUMMARY                                              │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  Service              Tech Stack         Protocol      Port    Dependencies          │
│  ────────────────────────────────────────────────────────────────────────────────   │
│  auth-service         Node.js/TS         REST/GraphQL 3001   PostgreSQL, Redis     │
│  mission-service      Go                 REST/gRPC    3002   PostgreSQL, Kafka     │
│  fleet-service        Python/FastAPI     REST/gRPC    3003   PostgreSQL, Redis     │
│  truck-service        Python/FastAPI     REST/gRPC    3004   PostgreSQL, Redis     │
│  trajectory-service   Python/PyTorch     REST        3005   PostgreSQL, Redis      │
│  telemetry-service    Go                 REST/WebSocket3006  Kafka, TimescaleDB    │
│  coordination-service  Go                 gRPC        3007   Redis (lock)          │
│  user-service         Node.js/TS         REST/GraphQL 3008   PostgreSQL, Redis     │
│  notification-service Node.js             HTTP        3009   Firebase/APNs         │
│                                                                                      │
│  DATABASES                                                                         │
│  ────────────────────────────────────────────────────────────────────────────────   │
│  PostgreSQL + TimescaleDB (missions, telemetry, users)                             │
│  Redis (cache, sessions, pub/sub, rate limiting)                                   │
│  Elasticsearch (logs, search)                                                       │
│  InfluxDB (optional - time-series metrics)                                         │
│                                                                                      │
│  MESSAGE BUS                                                                       │
│  ────────────────────────────────────────────────────────────────────────────────   │
│  Apache Kafka (events, telemetry stream, mission queue)                           │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 17. Conclusion

Cette architecture backend fournit :

- **Modularité** : Chaque service est indépendant, déployable et scalable séparément
- **Résilience** : Pattern circuit breaker, retry, fallback intégrés
- **Observabilité** : Logs structurés, métriques, tracing distribué
- **Sécurité** : mTLS, JWT, RBAC, chiffrement complet, bcrypt pour mots de passe
- **Performance** : Cache Redis, async processing, partitionnement Kafka
- **Maintenabilité** : Code standardisé, documentation API (OpenAPI), tests automatisés
- **Conformité RGPD** : Droit à l'effacement, registre des traitements, journal des consentements
- **Continuité** : Pipeline CI/CD complet, stratégie backup/restore documentée

L'architecture supporte une croissance de 10 → 500 drones avec des mises à jour incrémentales.

---

## Historique des corrections

| Version | Date | Corrections |
|---------|------|--------------|
| 1.0 | 2026-03-31 | Version initiale |
| 1.1 | 2026-03-31 | Corrections comité (bcrypt, RGPD, FK, CI/CD, backup) |