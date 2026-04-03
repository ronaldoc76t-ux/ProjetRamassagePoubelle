# Implémentation S7: Backend Minimal + API

## Date de début: 2026-04-01

## 1. Architecture Microservices

```
backend/
├── mission-service     # CRUD missions, assignation (port 8001)
├── fleet-service      # Gestion flotte drones/camions (port 8002)
├── trajectory-service # Prédiction trajectoire (port 8003)
├── telemetry-service  # Ingestion métriques (port 8004)
├── coordination-service # Orchestration rendez-vous (port 8005)
└── api-gateway        # Point d'entrée unique (port 8080)
```

## 2. Stack Technique

- **Langage**: Go 1.21+ (Gin framework)
- **Base de données**: PostgreSQL 15 + TimescaleDB
- **Cache**: Redis 7
- **Message Queue**: Kafka (topics: missions, telemetry, events)
- **Auth**: JWT avec Argon2id

## 3. Structure Projet

```
src/
├── cmd/
│   ├── api-gateway/main.go
│   ├── mission-service/main.go
│   ├── fleet-service/main.go
│   └── coordination-service/main.go
├── internal/
│   ├── config/
│   ├── models/
│   ├── handlers/
│   ├── middleware/
│   └── repository/
├── pkg/
│   ├── auth/
│   ├── kafka/
│   └── database/
├── proto/
│   └── *.proto
├── docker-compose.yml
├── Dockerfile
└── go.mod
```

## 4. Models - Mission

```go
// internal/models/mission.go
package models

import (
    "time"
    "github.com/google/uuid"
)

type Mission struct {
    ID              uuid.UUID `json:"id" gorm:"type:uuid;primary_key"`
    Status          MissionStatus `json:"status" gorm:"type:varchar(20)"`
    Type            MissionType   `json:"type" gorm:"type:varchar(20)"`
    ZoneID          string    `json:"zone_id" gorm:"type:varchar(50)"`
    ZoneName        string    `json:"zone_name"`
    AssignedDroneID *uuid.UUID `json:"assigned_drone_id" gorm:"type:uuid"`
    TruckID         string    `json:"truck_id" gorm:"type:varchar(50)"`
    ScheduledStart  time.Time `json:"scheduled_start"`
    ScheduledEnd    time.Time `json:"scheduled_end"`
    ActualStart     *time.Time `json:"actual_start"`
    ActualEnd       *time.Time `json:"actual_end"`
    CollectedCount  int       `json:"collected_count"`
    SuccessRate     float64   `json:"success_rate"`
    CreatedBy       uuid.UUID `json:"created_by" gorm:"type:uuid"`
    CreatedAt       time.Time `json:"created_at"`
    UpdatedAt       time.Time `json:"updated_at"`
}

type MissionStatus string
const (
    MissionStatusPending   MissionStatus = "PENDING"
    MissionStatusAssigned  MissionStatus = "ASSIGNED"
    MissionStatusInProgress MissionStatus = "IN_PROGRESS"
    MissionStatusCompleted MissionStatus = "COMPLETED"
    MissionStatusFailed    MissionStatus = "FAILED"
    MissionStatusCancelled MissionStatus = "CANCELLED"
)

type MissionType string
const (
    MissionTypeCollection MissionType = "COLLECTION"
    MissionTypeInspection MissionType = "INSPECTION"
    MissionTypeEmergency  MissionType = "EMERGENCY"
)
```

## 5. Models - Fleet

```go
// internal/models/fleet.go
package models

type Drone struct {
    ID           uuid.UUID `json:"id" gorm:"type:uuid;primary_key"`
    SerialNumber string    `json:"serial_number" gorm:"uniqueIndex;size:50"`
    Model        string    `json:"model" gorm:"size:100"`
    Status       DroneStatus `json:"status" gorm:"type:varchar(20)"`
    BatteryPercent float64  `json:"battery_percent"`
    LastPosition Position  `json:"last_position" gorm:"type:jsonb"`
    LastSeen     time.Time `json:"last_seen"`
    CreatedAt   time.Time `json:"created_at"`
}

type DroneStatus string
const (
    DroneStatusAvailable DroneStatus = "AVAILABLE"
    DroneStatusAssigned  DroneStatus = "ASSIGNED"
    DroneStatusFlying    DroneStatus = "FLYING"
    DroneStatusCharging  DroneStatus = "CHARGING"
    DroneStatusMaintenance DroneStatus = "MAINTENANCE"
    DroneStatusOffline   DroneStatus = "OFFLINE"
)

type Position struct {
    Latitude  float64 `json:"latitude"`
    Longitude float64 `json:"longitude"`
    Altitude  float64 `json:"altitude"`
    Heading   float64 `json:"heading"`
}

type Truck struct {
    ID           uuid.UUID `json:"id" gorm:"type:uuid;primary_key"`
    SerialNumber string    `json:"serial_number" gorm:"uniqueIndex;size:50"`
    Status       TruckStatus `json:"status" gorm:"type:varchar(20)"`
    BatteryPercent float64 `json:"battery_percent"`
    CurrentMissionID *uuid.UUID `json:"current_mission_id" gorm:"type:uuid"`
    LastPosition Position `json:"last_position" gorm:"type:jsonb"`
    LastSeen    time.Time `json:"last_seen"`
}
```

## 6. Handlers - Mission Service

```go
// internal/handlers/mission.go
package handlers

import (
    "net/http"
    "github.com/gin-gonic/gin"
    "github.com/google/uuid"
    "openclaw/backend/internal/models"
)

type MissionHandler struct {
    repo *repository.MissionRepository
    kafka *kafka.Producer
}

func NewMissionHandler(repo *repository.MissionRepository, kafka *kafka.Producer) *MissionHandler {
    return &MissionHandler{repo: repo, kafka: kafka}
}

// GET /api/v1/missions
func (h *MissionHandler) ListMissions(c *gin.Context) {
    var missions []models.Mission
    query := h.repo.Preload("AssignedDrone")
    
    // Filters
    if status := c.Query("status"); status != "" {
        query = query.Where("status = ?", status)
    }
    if zoneID := c.Query("zone_id"); zoneID != "" {
        query = query.Where("zone_id = ?", zoneID)
    }
    
    if err := query.Find(&missions).Error; err != nil {
        c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
        return
    }
    
    c.JSON(http.StatusOK, missions)
}

// GET /api/v1/missions/:id
func (h *MissionHandler) GetMission(c *gin.Context) {
    id := c.Param("id")
    missionID, err := uuid.Parse(id)
    if err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": "invalid mission id"})
        return
    }
    
    var mission models.Mission
    if err := h.repo.Preload("AssignedDrone").First(&mission, missionID).Error; err != nil {
        c.JSON(http.StatusNotFound, gin.H{"error": "mission not found"})
        return
    }
    
    c.JSON(http.StatusOK, mission)
}

// POST /api/v1/missions
func (h *MissionHandler) CreateMission(c *gin.Context) {
    var input models.Mission
    if err := c.ShouldBindJSON(&input); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    
    input.ID = uuid.New()
    input.Status = models.MissionStatusPending
    
    if err := h.repo.Create(&input).Error; err != nil {
        c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
        return
    }
    
    // Publish to Kafka
    h.kafka.Publish("missions", input)
    
    c.JSON(http.StatusCreated, input)
}

// PATCH /api/v1/missions/:id/status
func (h *MissionHandler) UpdateMissionStatus(c *gin.Context) {
    id := c.Param("id")
    missionID, err := uuid.Parse(id)
    if err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": "invalid mission id"})
        return
    }
    
    var input struct {
        Status models.MissionStatus `json:"status" binding:"required"`
    }
    if err := c.ShouldBindJSON(&input); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    
    if err := h.repo.Model(&models.Mission{}).Where("id = ?", missionID).
        Updates(map[string]interface{}{"status": input.Status, "updated_at": time.Now()}).Error; err != nil {
        c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
        return
    }
    
    c.JSON(http.StatusOK, gin.H{"status": "updated"})
}
```

## 7. Handlers - Fleet Service

```go
// internal/handlers/fleet.go
package handlers

type FleetHandler struct {
    repo *repository.FleetRepository
    cache *redis.Client
}

func NewFleetHandler(repo *repository.FleetRepository, cache *redis.Client) *FleetHandler {
    return &FleetHandler{repo: repo, cache: cache}
}

// GET /api/v1/fleet/drones
func (h *FleetHandler) ListDrones(c *gin.Context) {
    // Try cache first
    cached, err := h.cache.Get("drones:all").Result()
    if err == nil {
        c.String(http.StatusOK, cached)
        return
    }
    
    var drones []models.Drone
    if err := h.repo.Find(&drones).Error; err != nil {
        c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
        return
    }
    
    // Cache for 30 seconds
    json, _ := json.Marshal(drones)
    h.cache.SetEX("drones:all", string(json), 30)
    
    c.JSON(http.StatusOK, drones)
}

// GET /api/v1/fleet/drones/:id/status
func (h *FleetHandler) GetDroneStatus(c *gin.Context) {
    id := c.Param("id")
    
    // Get from cache or DB
    status, err := h.cache.Get("drone:" + id + ":status").Result()
    if err != nil {
        var drone models.Drone
        if err := h.repo.First(&drone, "serial_number = ?", id).Error; err != nil {
            c.JSON(http.StatusNotFound, gin.H{"error": "drone not found"})
            return
        }
        status = string(drone.Status)
    }
    
    c.JSON(http.StatusOK, gin.H{"status": status})
}

// POST /api/v1/fleet/drones/:id/telemetry
func (h *FleetHandler) UpdateDroneTelemetry(c *gin.Context) {
    id := c.Param("id")
    
    var input struct {
        BatteryPercent float64  `json:"battery_percent"`
        Position       struct {
            Latitude  float64 `json:"latitude"`
            Longitude float64 `json:"longitude"`
            Altitude  float64 `json:"altitude"`
        } `json:"position"`
    }
    if err := c.ShouldBindJSON(&input); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
        return
    }
    
    // Update in Redis for real-time
    h.cache.HSet("drone:"+id, map[string]interface{}{
        "battery": input.BatteryPercent,
        "lat": input.Position.Latitude,
        "lon": input.Position.Longitude,
        "alt": input.Position.Altitude,
    })
    
    c.JSON(http.StatusOK, gin.H{"updated": true})
}
```

## 8. Middleware - Auth

```go
// internal/middleware/auth.go
package middleware

import (
    "net/http"
    "strings"
    "github.com/gin-gonic/gin"
    "github.com/golang-jwt/jwt/v5"
)

type AuthMiddleware struct {
    jwtSecret []byte
}

func NewAuthMiddleware(secret string) *AuthMiddleware {
    return &AuthMiddleware{jwtSecret: []byte(secret)}
}

func (m *AuthMiddleware) RequireAuth() gin.HandlerFunc {
    return func(c *gin.Context) {
        authHeader := c.GetHeader("Authorization")
        if authHeader == "" {
            c.JSON(http.StatusUnauthorized, gin.H{"error": "authorization header required"})
            c.Abort()
            return
        }
        
        tokenString := strings.TrimPrefix(authHeader, "Bearer ")
        if tokenString == authHeader {
            c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid authorization format"})
            c.Abort()
            return
        }
        
        token, err := jwt.Parse(tokenString, func(token *jwt.Token) (interface{}, error) {
            return m.jwtSecret, nil
        })
        
        if err != nil || !token.Valid {
            c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid token"})
            c.Abort()
            return
        }
        
        claims := token.Claims.(jwt.MapClaims)
        c.Set("user_id", claims["sub"])
        c.Set("role", claims["role"])
        
        c.Next()
    }
}

func (m *AuthMiddleware) RequireRole(roles ...string) gin.HandlerFunc {
    return func(c *gin.Context) {
        userRole := c.GetString("role")
        for _, role := range roles {
            if userRole == role {
                c.Next()
                return
            }
        }
        c.JSON(http.StatusForbidden, gin.H{"error": "insufficient permissions"})
        c.Abort()
    }
}
```

## 9. Database Schema (PostgreSQL)

```sql
-- Schema PostgreSQL avec TimescaleDB

-- Extensions
CREATE EXTENSION IF NOT EXISTS timescaledb;
CREATE EXTENSION IF NOT EXISTS postgis;

-- Table missions
CREATE TABLE missions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    type VARCHAR(20) NOT NULL,
    zone_id VARCHAR(50),
    zone_name VARCHAR(255),
    assigned_drone_id UUID REFERENCES drones(id),
    truck_id VARCHAR(50) NOT NULL,
    scheduled_start TIMESTAMPTZ,
    scheduled_end TIMESTAMPTZ,
    actual_start TIMESTAMPTZ,
    actual_end TIMESTAMPTZ,
    collected_count INT DEFAULT 0,
    success_rate FLOAT DEFAULT 0.0,
    created_by UUID REFERENCES users(id),
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Table drones
CREATE TABLE drones (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    serial_number VARCHAR(50) UNIQUE NOT NULL,
    model VARCHAR(100),
    status VARCHAR(20) NOT NULL DEFAULT 'AVAILABLE',
    battery_percent FLOAT DEFAULT 100.0,
    last_position JSONB,
    last_seen TIMESTAMPTZ DEFAULT NOW(),
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Table trucks
CREATE TABLE trucks (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    serial_number VARCHAR(50) UNIQUE NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'IDLE',
    battery_percent FLOAT DEFAULT 100.0,
    current_mission_id UUID REFERENCES missions(id),
    last_position JSONB,
    last_seen TIMESTAMPTZ DEFAULT NOW()
);

-- Table users
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    first_name VARCHAR(100),
    last_name VARCHAR(100),
    role VARCHAR(20) NOT NULL DEFAULT 'VIEWER',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Hypertable pour télémétrie drone
CREATE TABLE drone_telemetry (
    time TIMESTAMPTZ NOT NULL,
    drone_id UUID REFERENCES drones(id),
    battery_percent FLOAT,
    latitude DECIMAL(10,8),
    longitude DECIMAL(11,8),
    altitude FLOAT,
    speed FLOAT,
    heading FLOAT
);

SELECT create_hypertable('drone_telemetry', 'time', 
    chunk_time_interval => INTERVAL '1 minute',
    if_not_exists => TRUE);

-- Index
CREATE INDEX idx_missions_status ON missions(status);
CREATE INDEX idx_missions_zone ON missions(zone_id);
CREATE INDEX idx_drones_status ON drones(status);
CREATE INDEX idx_drones_serial ON drones(serial_number);
```

## 10. Docker Compose

```yaml
version: '3.8'

services:
  api-gateway:
    build: ./cmd/api-gateway
    ports:
      - "8080:8080"
    environment:
      - JWT_SECRET=${JWT_SECRET}
      - DATABASE_URL=postgres://openclaw:password@postgres:5432/openclaw?sslmode=disable
      - REDIS_URL=redis://redis:6379
      - KAFKA_BROKERS=kafka:9092
    depends_on:
      - postgres
      - redis
      - kafka

  mission-service:
    build: ./cmd/mission-service
    ports:
      - "8001:8001"
    environment:
      - DATABASE_URL=postgres://openclaw:password@postgres:5432/openclaw?sslmode=disable
      - KAFKA_BROKERS=kafka:9092

  fleet-service:
    build: ./cmd/fleet-service
    ports:
      - "8002:8002"
    environment:
      - DATABASE_URL=postgres://openclaw:password@postgres:5432/openclaw?sslmode=disable
      - REDIS_URL=redis://redis:6379

  postgres:
    image: timescale/timescaledb:latest-pg15
    environment:
      POSTGRES_USER: openclaw
      POSTGRES_PASSWORD: password
      POSTGRES_DB: openclaw
    volumes:
      - postgres_data:/var/lib/postgresql/data

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"

  kafka:
    image: confluentinc/cp-kafka:latest
    environment:
      KAFKA_BROKER_ID: 1
      KAFKA_ZOOKEEPER_CONNECT: zookeeper:2181
      KAFKA_ADVERTISED_LISTENERS: PLAINTEXT://kafka:9092
    depends_on:
      - zookeeper

  zookeeper:
    image: confluentinc/cp-zookeeper:latest

volumes:
  postgres_data:
```

## 11. API Endpoints Summary

| Method | Endpoint | Description | Auth |
|--------|----------|-------------|------|
| GET | /api/v1/missions | List missions | ✅ |
| GET | /api/v1/missions/:id | Get mission | ✅ |
| POST | /api/v1/missions | Create mission | ✅ Operator |
| PATCH | /api/v1/missions/:id/status | Update status | ✅ Operator |
| GET | /api/v1/fleet/drones | List drones | ✅ |
| GET | /api/v1/fleet/drones/:id/status | Drone status | ✅ |
| POST | /api/v1/fleet/drones/:id/telemetry | Update telemetry | ✅ Drone |
| GET | /api/v1/fleet/trucks | List trucks | ✅ |
| POST | /api/v1/auth/login | Login | ❌ |
| POST | /api/v1/auth/refresh | Refresh token | ✅ |

## 12. Critères de Definition of Done

- [ ] Services compilent sans erreur
- [ ] Tests unitaires > 80% coverage
- [ ] Docker Compose fonctionnel
- [ ] API documentée (OpenAPI/Swagger)
- [ ] Peer review approuvée

---

*Document généré le 2026-04-01*
*Prochaine étape: S8 Orchestrateur*