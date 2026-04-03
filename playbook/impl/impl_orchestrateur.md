# Implémentation S8: Orchestrateur + Tests Intégrés

## Date de début: 2026-04-01

## 1. Architecture de l'Orchestrateur

```
orchestrateur/
├── core/                  # Logique centrale
│   ├── scheduler/         # Planification missions
│   ├── coordinator/       # Coordination truck-drone
│   ├── state_machine/     # Gestion états globaux
│   └── event_manager/     # Gestion événements
├── plugins/               # Plugins dynamiques
│   ├── truck_adapter/     # Interface ROS2 truck
│   ├── drone_adapter/     # Interface ROS2 drone
│   └── mission_adapter/   # Interface Backend API
├── tests/                 # Tests d'intégration
└── main.go               # Point d'entrée
```

## 2. Responsabilités

| Fonction | Description |
|----------|-------------|
| **Mission Planning** | Créer/assigner missions selon zone + dispo drone |
| **Trajectory Sync** | Synchroniser prédiction truck → drone |
| **Rendezvous Mgmt** | Orchestrer RV timing entre truck et drone |
| **Failover** | Gérer erreurs et rediriger ressources |
| **Monitoring** | Surveiller santé globale du système |

## 3. Core - Scheduler

```go
// internal/core/scheduler/scheduler.go
package scheduler

import (
    "time"
    "github.com/google/uuid"
    "openclaw/orchestrateur/internal/models"
)

type Scheduler struct {
    missionRepo repository.MissionRepository
    fleetRepo   repository.FleetRepository
    eventBus    *kafka.Producer
}

func NewScheduler(missionRepo repository.MissionRepository, fleetRepo repository.FleetRepository) *Scheduler {
    return &Scheduler{
        missionRepo: missionRepo,
        fleetRepo:   fleetRepo,
    }
}

// ScheduleNextMission - Assigne une mission au drone disponible
func (s *Scheduler) ScheduleNextMission() (*models.Mission, error) {
    // 1. Trouver drone disponible avec batterie suffisante
    drone, err := s.findAvailableDrone()
    if err != nil {
        return nil, err
    }
    
    // 2. Trouver zone avec highest priority
    zone, err := s.findHighestPriorityZone()
    if err != nil {
        return nil, err
    }
    
    // 3. Créer mission
    mission := &models.Mission{
        ID:          uuid.New(),
        Type:        models.MissionTypeCollection,
        ZoneID:      zone.ID,
        ZoneName:    zone.Name,
        Status:      models.MissionStatusPending,
        ScheduledStart: time.Now().Add(30 * time.Second),
    }
    
    // 4. Assigner drone
    mission.AssignedDroneID = &drone.ID
    drone.Status = models.DroneStatusAssigned
    
    // 5. Sauvegarder
    if err := s.missionRepo.Create(mission).Error; err != nil {
        return nil, err
    }
    if err := s.fleetRepo.Save(drone).Error; err != nil {
        return nil, err
    }
    
    // 6. Publier événement
    s.eventBus.Publish("mission.assigned", map[string]interface{}{
        "mission_id": mission.ID,
        "drone_id":   drone.ID,
        "zone":       zone.Name,
    })
    
    return mission, nil
}

func (s *Scheduler) findAvailableDrone() (*models.Drone, error) {
    var drone models.Drone
    err := s.fleetRepo.
        Where("status = ? AND battery_percent > ?", 
            models.DroneStatusAvailable, 40.0).
        Order("battery_percent DESC").
        First(&drone).Error
    
    if err != nil {
        return nil, err
    }
    return &drone, nil
}

func (s *Scheduler) findHighestPriorityZone() (*models.Zone, error) {
    var zone models.Zone
    err := s.fleetRepo.
        Where("priority > 0").
        Order("priority DESC").
        First(&zone).Error
    
    if err != nil {
        return nil, err
    }
    return &zone, nil
}
```

## 4. Core - Coordinator (RV Management)

```go
// internal/core/coordinator/coordinator.go
package coordinator

import (
    "time"
    "math"
    "github.com/google/uuid"
    "openclaw/orchestrateur/internal/models"
)

type Coordinator struct {
    truckAdapter  *plugins.TruckAdapter
    droneAdapter  *plugins.DroneAdapter
    trajectorySvc *TrajectoryService
    eventBus      *kafka.Producer
}

type RendezvousParams struct {
    TruckID       string
    DroneID       uuid.UUID
    TargetTime    time.Time
    MinDuration   time.Duration // Durée minimum collection
    MaxWait       time.Duration // Temps max attente drone
}

// InitiateRendezvous - Orchestre un rendez-vous truck-drone
func (c *Coordinator) InitiateRendezvous(params *RendezvousParams) (*models.Rendezvous, error) {
    // 1. Obtenir position actuelle du truck
    truckPos, err := c.truckAdapter.GetPosition(params.TruckID)
    if err != nil {
        return nil, err
    }
    
    // 2. Obtenir trajectoire prédite du truck
    trajectory, err := c.trajectorySvc.GetPredictedTrajectory(
        params.TruckID, 
        params.TargetTime,
    )
    if err != nil {
        return nil, err
    }
    
    // 3. Calculer position de RV optimale
    rvPoint := c.calculateRendezvousPoint(truckPos, trajectory)
    
    // 4. Demander drone de démarrer approche
    if err := c.droneAdapter.InitiateApproach(params.DroneID, rvPoint); err != nil {
        return nil, err
    }
    
    // 5. Informer truck du RV
    if err := c.truckAdapter.NotifyRendezvous(params.TruckID, rvPoint); err != nil {
        return nil, err
    }
    
    // 6. Créer enregistrement RV
    rv := &models.Rendezvous{
        ID:              uuid.New(),
        DroneID:         params.DroneID,
        TruckID:         params.TruckID,
        TargetPosition:  rvPoint,
        TargetTime:      params.TargetTime,
        Status:          models.RVStatusPending,
    }
    
    // Surveiller convergence
    go c.monitorRendezvous(rv)
    
    return rv, nil
}

func (c *Coordinator) calculateRendezvousPoint(
    currentPos models.Position, 
    trajectory []models.PredictedPosition,
) models.Position {
    // Trouver le point optimal sur la trajectoire
    // avec offset latéral pour sécurité drone
    
    if len(trajectory) == 0 {
        return currentPos
    }
    
    // Point le plus proche de maintenant + horizon
    targetIdx := len(trajectory) / 2
    
    return models.Position{
        Latitude:  trajectory[targetIdx].Latitude,
        Longitude: trajectory[targetIdx].Longitude + 2.0, // 2m offset
        Altitude:  3.0, // Hauteur d'approche
    }
}

// monitorRendezvous - Surveille la convergence drone-truck
func (c *Coordinator) monitorRendezvous(rv *models.Rendezvous) {
    ticker := time.NewTicker(500 * time.Millisecond)
    defer ticker.Stop()
    
    for {
        select {
        case <-ticker.C:
            dronePos, _ := c.droneAdapter.GetPosition(rv.DroneID)
            truckPos, _ := c.truckAdapter.GetPosition(rv.TruckID)
            
            distance := math.Sqrt(
                math.Pow(dronePos.Latitude-truckPos.Latitude, 2) +
                math.Pow(dronePos.Longitude-truckPos.Longitude, 2),
            )
            
            if distance < 0.5 { // < 50cm
                rv.Status = models.RVStatusDocked
                c.eventBus.Publish("rendezvous.completed", rv)
                return
            }
            
        case <-time.After(60 * time.Second): // Timeout
            rv.Status = models.RVStatusFailed
            c.eventBus.Publish("rendezvous.failed", rv)
            return
        }
    }
}
```

## 5. Plugins - Truck Adapter

```go
// internal/plugins/truck_adapter.go
package plugins

import (
    "rclgo/rclgo"
    "openclaw/orchestrateur/internal/models"
)

type TruckAdapter struct {
    node *rclgo.Node
    sub  *rclgo.Subscription
    pub  *rclgo.Publisher
}

func NewTruckAdapter() (*TruckAdapter, error) {
    node, err := rclgo.NewNode("orchestrator_truck_adapter")
    if err != nil {
        return nil, err
    }
    
    sub, err := node.NewSubscription(
        "/truck/odometry",
        "nav_msgs/msg/Odometry",
        rclgo.WithSubscriptionBestEffort(),
    )
    if err != nil {
        return nil, err
    }
    
    pub, err := node.NewPublisher(
        "/truck/rendezvous_command",
        "std_msgs/msg/String",
    )
    if err != nil {
        return nil, err
    }
    
    return &TruckAdapter{
        node: node,
        sub:  sub,
        pub:  pub,
    }, nil
}

func (t *TruckAdapter) GetPosition(truckID string) (models.Position, error) {
    // Subscribe to /truck/odometry and return latest position
    // This is a placeholder - actual implementation would cache position
    return models.Position{}, nil
}

func (t *TruckAdapter) NotifyRendezvous(truckID string, pos models.Position) error {
    // Publish rendezvous command to truck
    msg := map[string]interface{}{
        "type": "RVC_REQUEST",
        "position": pos,
    }
    data, _ := json.Marshal(msg)
    t.pub.Publish(string(data))
    return nil
}
```

## 6. State Machine Globale

```go
// internal/core/state_machine/global_state.go
package state_machine

type SystemState string

const (
    SystemStateInitializing SystemState = "INITIALIZING"
    SystemStateOperational  SystemState = "OPERATIONAL"
    SystemStateDegraded     SystemState = "DEGRADED"
    SystemStateEmergency    SystemState = "EMERGENCY"
    SystemStateShutdown     SystemState = "SHUTDOWN"
)

type GlobalStateMachine struct {
    currentState SystemState
    transitions  map[SystemState][]SystemState
    onEnter      map[SystemState]func()
    onExit       map[SystemState]func()
}

func NewGlobalStateMachine() *GlobalStateMachine {
    sm := &GlobalStateMachine{
        currentState: SystemStateInitializing,
        transitions: map[SystemState][]SystemState{
            SystemStateInitializing: {SystemStateOperational, SystemStateShutdown},
            SystemStateOperational:  {SystemStateDegraded, SystemStateShutdown},
            SystemStateDegraded:     {SystemStateOperational, SystemStateEmergency},
            SystemStateEmergency:   {SystemStateShutdown},
        },
        onEnter: make(map[SystemState]func()),
        onExit:  make(map[SystemState]func()),
    }
    
    sm.onEnter[SystemStateOperational] = func() {
        log.Info("System is now OPERATIONAL")
    }
    sm.onEnter[SystemStateDegraded] = func() {
        log.Warn("System degraded - limited operation")
    }
    sm.onEnter[SystemStateEmergency] = func() {
        log.Error("EMERGENCY - all drones returning to base")
    }
    
    return sm
}

func (sm *GlobalStateMachine) Transition(newState SystemState) error {
    allowedTransitions := sm.transitions[sm.currentState]
    for _, allowed := range allowedTransitions {
        if allowed == newState {
            if sm.onExit[sm.currentState] != nil {
                sm.onExit[sm.currentState]()
            }
            sm.currentState = newState
            if sm.onEnter[newState] != nil {
                sm.onEnter[newState]()
            }
            return nil
        }
    }
    return fmt.Errorf("invalid transition from %s to %s", sm.currentState, newState)
}

func (sm *GlobalStateMachine) GetState() SystemState {
    return sm.currentState
}
```

## 7. Tests d'Intégration

```go
// tests/integration_test.go
package tests

import (
    "testing"
    "time"
    "github.com/stretchr/testify/assert"
    "github.com/stretchr/testify/mock"
    "openclaw/orchestrateur/internal/core/scheduler"
    "openclaw/orchestrateur/internal/models"
)

// Mock repositories
type MockMissionRepo struct {
    mock.Mock
}

func (m *MockMissionRepo) Create(mission *models.Mission) error {
    args := m.Called(mission)
    return args.Error(0)
}

func (m *MockMissionRepo) FindByID(id uuid.UUID) (*models.Mission, error) {
    args := m.Called(id)
    if args.Get(0) == nil {
        return nil, args.Error(1)
    }
    return args.Get(0).(*models.Mission), args.Error(1)
}

type MockFleetRepo struct {
    mock.Mock
}

func (m *MockFleetRepo) FindAvailable() (*models.Drone, error) {
    args := m.Called()
    if args.Get(0) == nil {
        return nil, args.Error(1)
    }
    return args.Get(0).(*models.Drone), args.Error(1)
}

// Tests
func TestScheduler_FindsAvailableDrone(t *testing.T) {
    mockFleet := new(MockFleetRepo)
    mockMission := new(MockMissionRepo)
    
    expectedDrone := &models.Drone{
        ID: uuid.New(),
        Status: models.DroneStatusAvailable,
        BatteryPercent: 80.0,
    }
    mockFleet.On("FindAvailable").Return(expectedDrone, nil)
    
    sched := scheduler.NewScheduler(mockMission, mockFleet)
    
    drone, err := sched.findAvailableDrone()
    
    assert.NoError(t, err)
    assert.Equal(t, expectedDrone.ID, drone.ID)
    mockFleet.AssertExpectations(t)
}

func TestScheduler_ScheduleMission(t *testing.T) {
    mockFleet := new(MockFleetRepo)
    mockMission := new(MockMissionRepo)
    
    availableDrone := &models.Drone{
        ID: uuid.New(),
        Status: models.DroneStatusAvailable,
        BatteryPercent: 80.0,
    }
    
    mockFleet.On("FindAvailable").Return(availableDrone, nil)
    mockMission.On("Create", mock.Anything).Return(nil)
    mockFleet.On("Save", mock.Anything).Return(nil)
    
    sched := scheduler.NewScheduler(mockMission, mockFleet)
    
    mission, err := sched.ScheduleNextMission()
    
    assert.NoError(t, err)
    assert.NotEqual(t, uuid.Nil, mission.ID)
    assert.Equal(t, models.MissionStatusPending, mission.Status)
    mockFleet.AssertExpectations(t)
}

func TestCoordinator_CalculateRendezvousPoint(t *testing.T) {
    coord := coordinator.NewCoordinator(nil, nil, nil)
    
    trajectory := []models.PredictedPosition{
        {Latitude: 48.85, Longitude: 2.35, Timestamp: time.Now()},
        {Latitude: 48.86, Longitude: 2.36, Timestamp: time.Now().Add(10*time.Second)},
    }
    
    rvPoint := coord.calculateRendezvousPoint(
        models.Position{Latitude: 48.85, Longitude: 2.35},
        trajectory,
    )
    
    assert.InDelta(t, 48.86, rvPoint.Latitude, 0.01)
    assert.InDelta(t, 2.38, rvPoint.Longitude, 0.01) // 2m offset
}

func TestGlobalStateMachine_ValidTransitions(t *testing.T) {
    sm := state_machine.NewGlobalStateMachine()
    
    // Initializing -> Operational is valid
    err := sm.Transition(state_machine.SystemStateOperational)
    assert.NoError(t, err)
    assert.Equal(t, state_machine.SystemStateOperational, sm.GetState())
    
    // Operational -> Degraded is valid
    err = sm.Transition(state_machine.SystemStateDegraded)
    assert.NoError(t, err)
    
    // Degraded -> Shutdown is NOT valid (must go through Emergency)
    err = sm.Transition(state_machine.SystemStateShutdown)
    assert.Error(t, err)
}

func TestGlobalStateMachine_InvalidTransition(t *testing.T) {
    sm := state_machine.NewGlobalStateMachine()
    
    // Initialize to Operational first
    sm.Transition(state_machine.SystemStateOperational)
    
    // Try direct transition to Shutdown (not allowed)
    err := sm.Transition(state_machine.SystemStateShutdown)
    assert.Error(t, err)
}
```

## 8. Configuration

```yaml
# config/orchestrator.yaml
orchestrator:
  name: "main-orchestrator"
  mode: "active"  # active or standby
  
  scheduling:
    interval_seconds: 10
    max_concurrent_missions: 5
    battery_threshold: 40.0
    
  coordination:
    rendezvous_timeout: 60
    position_tolerance: 0.5  # meters
    time_tolerance: 500      # milliseconds
    
  adapters:
    truck:
      ros2_namespace: "/truck"
      topics:
        odometry: "/truck/odometry"
        cmd_rendezvous: "/truck/rendezvous_command"
    drone:
      ros2_namespace: "/drone"
      topics:
        pose: "/drone/pose"
        status: "/drone/status"
        
  kafka:
    brokers: "localhost:9092"
    topics:
      mission_events: "mission.events"
      rv_events: "rendezvous.events"
      telemetry: "telemetry.raw"
```

## 9. Main Entry Point

```go
// main.go
package main

import (
    "context"
    "log"
    "os"
    "os/signal"
    "syscall"
    
    "github.com/spf13/viper"
    "openclaw/orchestrateur/internal/core/scheduler"
    "openclaw/orchestrateur/internal/core/coordinator"
    "openclaw/orchestrateur/internal/plugins"
)

func main() {
    // Load config
    viper.SetConfigName("orchestrator")
    viper.SetConfigType("yaml")
    viper.AddConfigPath("./config")
    viper.ReadInConfig()
    
    ctx, cancel := context.WithCancel(context.Background())
    defer cancel()
    
    // Initialize adapters
    truckAdapter, err := plugins.NewTruckAdapter()
    if err != nil {
        log.Fatalf("Failed to init truck adapter: %v", err)
    }
    
    droneAdapter, err := plugins.NewDroneAdapter()
    if err != nil {
        log.Fatalf("Failed to init drone adapter: %v", err)
    }
    
    // Initialize services
    sched := scheduler.NewScheduler(repo.Mission, repo.Fleet)
    coord := coordinator.NewCoordinator(
        truckAdapter, 
        droneAdapter, 
        trajectorySvc,
    )
    
    // Start scheduler loop
    go sched.Run(ctx)
    
    // Handle shutdown
    sigChan := make(chan os.Signal, 1)
    signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
    
    <-sigChan
    log.Info("Shutting down orchestrator...")
}
```

## 10. Critères de Definition of Done

- [ ] Orchestrateur compilé
- [ ] Tests d'intégration > 80% pass
- [ ] Communication ROS2 fonctionnelle
- [ ] Peer review approuvée

---

*Document généré le 2026-04-01*
*Prochaine étape: S9 Simulation*