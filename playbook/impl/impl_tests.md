# S11: Tests Fiabilité/Sécurité/Performance

## Story
- En tant qu'ingénieur QA, je veux une stratégie de test complète pour valider la fiabilité, la sécurité et les performances du système.
- Critère d'acceptation : tests unitaires, intégration, performance, sécurité ≥80% coverage, CI/CD pipeline.

---

## 1. Strategy de Tests

### Types de Tests

| Type | Couverture | Outils | Fréquence |
|------|------------|--------|------------|
| Unitaires | Modules truck_navigation, drone_navigation, backend | pytest, gtest | À chaque commit |
| Intégration | Scénarios truck-drone, API | pytest, ros2launch test | À chaque PR |
| Performance | Latence, throughput, battery | locust, JMeter, custom | Hebdomadaire |
| Sécurité | Auth, encryption, GDPR | sonarqube, bandit, owasp-zap | Quotidien |
| Fiabilité | Failover, recovery, chaos | chaos-mesh, custom scripts | Mensuel |

### Stack Outils

```yaml
Test Framework:
  - Python: pytest + pytest-cov + pytest-asyncio
  - ROS2: launch_testing + ros2test
  - Backend: fastapi test client + httpx

Performance:
  - Load testing: locust
  - Profiling: cProfile, py-spy
  - Metrics: prometheus-client

Security:
  - SAST: bandit (Python), cppcheck (C++)
  - DAST: owasp-zap
  - SCA: safety, pip-audit
  - Secrets: truffleHog, git-secrets

Fiabilité:
  - Chaos: chaos-mesh (k8s)
  - Fault injection: custom scripts
  - Monitoring: grafana + prometheus
```

---

## 2. Tests Unitaires

### 2.1 truck_navigation

```python
# tests/unit/test_truck_navigation.py
import pytest
from truck_navigation.nav_node import NavNode
from truck_navigation.trajectory_predictor import TrajectoryPredictor

class TestNavNode:
    """Tests pour le node de navigation du camion"""
    
    @pytest.fixture
    def nav_node(self):
        return NavNode()
    
    def test_initialization(self, nav_node):
        """Vérifie l'initialisation du node"""
        assert nav_node.current_position == (0.0, 0.0)
        assert nav_node.is_navigating is False
    
    def test_set_trajectory(self, nav_node):
        """Test chargement d'une trajectoire"""
        trajectory = [(0, 0), (10, 0), (20, 10)]
        nav_node.set_trajectory(trajectory)
        assert len(nav_node.trajectory) == 3
        assert nav_node.is_navigating is True
    
    def test_navigation_step(self, nav_node):
        """Test progression sur la trajectoire"""
        nav_node.set_trajectory([(0, 0), (10, 0)])
        nav_node.navigate_step()
        assert nav_node.current_position[0] > 0
    
    def test_gnss_loss_handling(self, nav_node):
        """Test basculement GPS -> odométrie"""
        nav_node.simulate_gnss_loss()
        assert nav_node.position_source == "odometry"
        assert nav_node.fallback_active is True


class TestTrajectoryPredictor:
    """Tests pour le prédicteur de trajectoire"""
    
    @pytest.fixture
    def predictor(self):
        return TrajectoryPredictor()
    
    def test_ekf_prediction(self, predictor):
        """Test prédiction EKF 30-120s"""
        history = [(0, 0), (1, 1), (2, 2)]
        pred = predictor.predict(history, horizon=30)
        assert pred is not None
        assert len(pred) > 0
    
    def test_lstm_prediction(self, predictor):
        """Test prédiction LSTM"""
        input_seq = [[i, i] for i in range(10)]
        pred = predictor.lstm_predict(input_seq)
        assert pred.shape[0] > 0
    
    def test_prediction_confidence(self, predictor):
        """Test calcul de confiance"""
        history = [(i, i) for i in range(20)]
        confidence = predictor.get_confidence(history)
        assert 0 <= confidence <= 1.0
```

### 2.2 drone_navigation

```python
# tests/unit/test_drone_navigation.py
import pytest
from drone_navigation.flight_controller import FlightController
from drone_navigation.docking_system import DockingSystem

class TestFlightController:
    """Tests pour le contrôleur de vol drone"""
    
    @pytest.fixture
    def drone(self):
        return FlightController()
    
    def test_takeoff(self, drone):
        """Test décollage"""
        result = drone.takeoff(altitude=10.0)
        assert result.success is True
        assert drone.altitude == 10.0
    
    def test_goto_position(self, drone):
        """Test navigation vers position"""
        result = drone.goto_position(x=100, y=50, z=10)
        assert result.arrived is True
    
    def test_battery_threshold(self, drone):
        """Test retour automatique batterie faible"""
        drone.battery = 15
        drone.check_battery_threshold()
        assert drone.should_return is True
    
    def test_gnss_degradation(self, drone):
        """Test gestion perte GNSS"""
        drone.simulate_gnss_degradation()
        assert drone.position_accuracy > 10.0  # metres


class TestDockingSystem:
    """Tests pour le système de docking"""
    
    @pytest.fixture
    def docking(self):
        return DockingSystem()
    
    def test_initiate_docking(self, docking):
        """Test initiation docking"""
        result = docking.initiate(truck_position=(100, 0))
        assert result.ready is True
    
    def test_docking_sequence(self, docking):
        """Test séquence complète docking"""
        docking.initiate(truck_position=(100, 0))
        result = docking.execute_sequence()
        assert result.docked is True
    
    def test_docking_timeout(self, docking):
        """Test timeout docking"""
        docking.initiate(truck_position=(100, 0))
        docking.simulate_timeout()
        assert docking.docking_failed is True
        assert docking.retry_count == 1
```

### 2.3 Backend

```python
# tests/unit/test_backend.py
import pytest
from backend.api.routes import app
from backend.services.orchestrator import Orchestrator
from backend.db.models import Rendezvous, Drone, Truck

@pytest.fixture
def client():
    from httpx import TestClient
    return TestClient(app)

class TestAPI:
    """Tests API REST/gRPC"""
    
    def test_health_endpoint(self, client):
        """Test endpoint santé"""
        response = client.get("/health")
        assert response.status_code == 200
        assert response.json()["status"] == "healthy"
    
    def test_rendezvous_create(self, client):
        """Test création rendez-vous"""
        payload = {
            "truck_id": "truck-001",
            "drone_id": "drone-001",
            "target_position": {"x": 100, "y": 50}
        }
        response = client.post("/api/v1/rendezvous", json=payload)
        assert response.status_code == 201
    
    def test_telemetry_stream(self, client):
        """Test flux télémétrie"""
        response = client.get("/api/v1/telemetry/stream")
        assert response.status_code == 200


class TestOrchestrator:
    """Tests orchestrateur"""
    
    @pytest.fixture
    def orchestrator(self):
        return Orchestrator()
    
    def test_assign_drone(self, orchestrator):
        """Test assignation drone"""
        result = orchestrator.assign_drone(truck_id="truck-001")
        assert result.drone_id is not None
    
    def test_optimize_rendezvous(self, orchestrator):
        """Test optimisation"""
        rendezvous = Rendezvous(...)
        optimized = orchestrator.optimize(rendezvous)
        assert optimized.estimated_time < original_time
```

---

## 3. Tests Intégration

### 3.1 Scénarios Truck-Drone

```python
# tests/integration/test_truck_drone_scenarios.py
import pytest
import rclpy
from truck_navigation.nav_node import NavNode
from drone_navigation.flight_controller import FlightController
from backend.services.orchestrator import Orchestrator

class TestTruckDroneRendezvous:
    """Tests d'intégration scénario complet"""
    
    @pytest.fixture(autouse=True)
    def setup_ros(self):
        rclpy.init()
        yield
        rclpy.shutdown()
    
    def test_successful_rendezvous(self):
        """Scénario: rendez-vous réussi"""
        # Setup
        truck = NavNode()
        drone = FlightController()
        orchestrator = Orchestrator()
        
        # Truck en mouvement
        truck.set_trajectory([(0, 0), (100, 0)])
        
        # Drone assigné
        drone_assign = orchestrator.assign_drone(truck.id)
        
        # Rendez-vous
        result = drone.perform_rendezvous(
            truck_position=truck.current_position,
            truck_velocity=truck.velocity
        )
        
        assert result.success is True
        assert result.docking_completed is True
    
    def test_drone_early_arrival(self):
        """Scénario: drone arrive avant le camion"""
        truck = NavNode()
        drone = FlightController()
        
        # Drone attend
        drone.goto_position(x=50, y=0, z=10)
        
        # Truck arrive avec retard
        truck.set_trajectory([(0, 0), (50, 0)])
        
        # Attente active drone
        result = drone.wait_for_truck(timeout=60)
        
        assert result.truck_arrived is True
    
    def test_communication_loss_during_rendezvous(self):
        """Scénario: perte comm pendant rendez-vous"""
        truck = NavNode()
        drone = FlightController()
        
        # Perte communication
        truck.simulate_comms_loss()
        
        # Drone doit avoir un fallback
        result = drone.handle_comms_loss()
        
        assert result.safe_mode is True
        assert drone.entered_hover is True
    
    def test_failed_docking_retry(self):
        """Scénario: retry après échec docking"""
        drone = FlightController()
        
        # Premier échec
        drone.simulate_docking_failure()
        
        # Retry
        result = drone.retry_docking()
        
        assert result.attempt_count == 2
        assert result.docked is True
```

### 3.2 Tests API Intégration

```python
# tests/integration/test_api_integration.py
import pytest
from backend.api.routes import app
from backend.db.session import get_db
from backend.tests.fixtures import *


class TestAPIIntegration:
    """Tests end-to-end API"""
    
    def test_full_rendezvous_flow(self, client, db_session):
        """Test flux complet création -> suivi -> completion"""
        # Create rendezvous
        response = client.post("/api/v1/rendezvous", json={
            "truck_id": "truck-001",
            "drone_id": "drone-001"
        })
        assert response.status_code == 201
        rendezvous_id = response.json()["id"]
        
        # Follow progress
        response = client.get(f"/api/v1/rendezvous/{rendezvous_id}")
        assert response.status_code == 200
        
        # Complete
        response = client.put(
            f"/api/v1/rendezvous/{rendezvous_id}/complete",
            json={"status": "completed"}
        )
        assert response.status_code == 200
```

---

## 4. Tests Performance

### 4.1 Latence

```python
# tests/performance/test_latency.py
import pytest
import time
from datetime import datetime

class TestLatency:
    """Tests de latence bout-en-bout"""
    
    def test_telemetry_latency(self):
        """Latence télémétrie < 100ms"""
        start = time.perf_counter()
        # Publish telemetry
        result = publish_telemetry()
        end = time.perf_counter()
        
        latency_ms = (end - start) * 1000
        assert latency_ms < 100
    
    def test_rendezvous_proposal_latency(self):
        """Latence proposition rendez-vous < 500ms"""
        start = time.perf_counter()
        result = propose_rendezvous(
            truck_position=(100, 0),
            available_drones=[...]
        )
        end = time.perf_counter()
        
        latency_ms = (end - start) * 1000
        assert latency_ms < 500
    
    def test_prediction_latency(self):
        """Latence prédiction trajectoire < 50ms"""
        start = time.perf_counter()
        predictor = TrajectoryPredictor()
        result = predictor.predict(history, horizon=30)
        end = time.perf_counter()
        
        latency_ms = (end - start) * 1000
        assert latency_ms < 50
```

### 4.2 Throughput

```python
# tests/performance/test_throughput.py
import pytest
import asyncio

class TestThroughput:
    """Tests de débit"""
    
    @pytest.mark.asyncio
    async def test_concurrent_rendezvous(self):
        """Test 100+ rendez-vous simultanés"""
        tasks = []
        for i in range(100):
            task = asyncio.create_task(
                create_rendezvous(f"truck-{i}", f"drone-{i}")
            )
            tasks.append(task)
        
        results = await asyncio.gather(*tasks)
        
        success_count = sum(1 for r in results if r.success)
        assert success_count >= 95
    
    def test_telemetry_throughput(self):
        """Test 1000 msgs/sec throughput"""
        import threading
        import time
        
        msg_count = 0
        stop_event = threading.Event()
        
        def publisher():
            while not stop_event.is_set():
                publish_telemetry()
                msg_count += 1
        
        thread = threading.Thread(publisher)
        thread.start()
        
        time.sleep(1)  # 1 second
        stop_event.set()
        thread.join()
        
        assert msg_count >= 1000
```

### 4.3 Battery

```python
# tests/performance/test_battery.py
import pytest

class TestBattery:
    """Tests consommation énergie"""
    
    def test_drone_endurance(self):
        """Drone maintient 30min autonomie"""
        drone = FlightController()
        
        # Simulation vol continu
        battery_used = drone.simulate_flight(duration_minutes=30)
        
        #	doit rester > 20% batterie
        assert drone.battery > 20
    
    def test_power_optimization(self):
        """Test optimisation puissance"""
        drone = FlightController()
        
        baseline = drone.measure_power_consumption(mode="normal")
        optimized = drone.measure_power_consumption(mode="eco")
        
        # Eco mode doit réduire consommation
        assert optimized < baseline * 0.8
```

---

## 5. Tests Sécurité

### 5.1 Auth & Authorization

```python
# tests/security/test_auth.py
import pytest
import jwt
from datetime import datetime, timedelta

class TestAuthentication:
    """Tests authentification"""
    
    def test_valid_jwt(self, client):
        """JWT valide accepté"""
        token = jwt.encode(
            {"sub": "drone-001", "exp": datetime.utcnow() + timedelta(hours=1)},
            "secret",
            algorithm="HS256"
        )
        
        response = client.get(
            "/api/v1/telemetry",
            headers={"Authorization": f"Bearer {token}"}
        )
        
        assert response.status_code == 200
    
    def test_expired_jwt(self, client):
        """JWT expiré refusé"""
        token = jwt.encode(
            {"sub": "drone-001", "exp": datetime.utcnow() - timedelta(hours=1)},
            "secret",
            algorithm="HS256"
        )
        
        response = client.get(
            "/api/v1/telemetry",
            headers={"Authorization": f"Bearer {token}"}
        )
        
        assert response.status_code == 401
    
    def test_mtls_connection(self):
        """Connexion mTLS fonctionnent"""
        result = connect_mtls(
            client_cert="certs/client.crt",
            client_key="certs/client.key",
            ca_cert="certs/ca.crt"
        )
        
        assert result.verified is True
```

### 5.2 Encryption

```python
# tests/security/test_encryption.py
import pytest
from cryptography.fernet import Fernet

class TestEncryption:
    """Tests chiffrement"""
    
    def test_telemetry_encryption(self):
        """Télémétrie chiffrée E2E"""
        key = Fernet.generate_key()
        f = Fernet(key)
        
        telemetry = {"position": {"x": 100, "y": 50}, "battery": 80}
        
        encrypted = f.encrypt(telemetry)
        decrypted = f.decrypt(encrypted)
        
        assert decrypted == telemetry
    
    def test_aes_gcm_encryption(self):
        """Chiffrement AES-GCM pour données sensibles"""
        from cryptography.hazmat.primitives.ciphers.aead import AESGCM
        
        key = AESGCM.generate_key(bit_length=256)
        aesgcm = AESGCM(key)
        
        data = b"sensitive_position_data"
        nonce = os.urandom(12)
        
        ciphertext = aesgcm.encrypt(nonce, data, None)
        plaintext = aesgcm.decrypt(nonce, ciphertext, None)
        
        assert plaintext == data
```

### 5.3 GDPR Compliance

```python
# tests/security/test_gdpr.py
import pytest

class TestGDPR:
    """Tests conformité RGPD"""
    
    def test_data_deletion_request(self, client):
        """Droit à l'effacement"""
        response = client.delete(
            "/api/v1/users/user-123/data",
            headers={"X-Request-ID": "deletion-001"}
        )
        
        assert response.status_code == 200
        
        # Vérifie suppression
        response = client.get("/api/v1/users/user-123/data")
        assert response.status_code == 404
    
    def test_data_anonymization(self):
        """Anonymisation données"""
        result = anonymize_telemetry(
            user_id="user-123",
            data=[...]
        )
        
        assert result.user_id is None
        assert result.position is_generalized is True
    
    def test_consent_tracking(self):
        """Consentement.tracké"""
        consent = get_consent(user_id="user-123")
        
        assert consent.marketing is False
        assert consent.analytics is True
```

---

## 6. Tests Fiabilité

### 6.1 Failover

```python
# tests/reliability/test_failover.py
import pytest
from unittest.mock import patch

class TestFailover:
    """Tests basculement"""
    
    def test_primary_orchestrator_failure(self):
        """Basculement vers backup orchestrateur"""
        orchestrator = Orchestrator()
        
        # Primary fail
        orchestrator.primary.fail()
        
        # Vérifie basculement
        assert orchestrator.active.name == "backup"
        assert orchestrator.is_running is True
    
    def test_drone_redundancy(self):
        """Drone backup prend le relais"""
        fleet = DroneFleet()
        
        # Primary drone fail
        primary = fleet.get_drone("drone-001")
        primary.simulate_failure()
        
        # Backup prend la mission
        backup = fleet.get_backup(primary.mission_id)
        
        assert backup.has_mission is True
        assert backup.id != "drone-001"
    
    def test_network_failover(self):
        """Basculement réseau 4G/5G/Satellite"""
        comms = CommunicationSystem()
        
        # Primary (5G) fail
        comms.primary_network.fail()
        
        # Basculement
        assert comms.active_network in ["4g", "satellite"]
        assert comms.is_connected is True
```

### 6.2 Recovery

```python
# tests/reliability/test_recovery.py
import pytest

class TestRecovery:
    """Tests récupération"""
    
    def test_crash_recovery(self):
        """Récupération après crash processus"""
        node = NavNode()
        
        # Crash simulation
        node.crash()
        
        # Recovery
        result = node.recover()
        
        assert result.successful is True
        assert node.state == node.last_known_state
    
    def test_database_recovery(self):
        """Récupération BDD après failure"""
        db = PostgreSQLDatabase()
        
        # Simulate corruption
        db.simulate_failure()
        
        # Recovery from replica
        result = db.recover_from_replica()
        
        assert result.data_integrity == 100%
    
    def test_state_reconciliation(self):
        """Réconciliation état après partition"""
        truck = NavNode()
        
        # Partition network
        truck.partition_network()
        
        # Reconcile
        result = truck.reconcile_state()
        
        assert result.consistent is True
```

---

## 7. Couverture Cible

### Objectifs Coverage

| Module | Cible Coverage | Actuel |
|--------|----------------|--------|
| truck_navigation | ≥85% | - |
| drone_navigation | ≥85% | - |
| backend/api | ≥80% | - |
| backend/services | ≥80% | - |
| backend/db | ≥80% | - |
| **Global** | **≥80%** | - |

### Configuration pytest-cov

```ini
# pyproject.toml
[tool.pytest.ini_options]
addopts = [
    "--cov=src/truck_navigation",
    "--cov=src/drone_navigation",
    "--cov=src/backend",
    "--cov-report=term-missing",
    "--cov-report=html:coverage_html",
    "--cov-fail-under=80",
]

[tool.coverage.run]
source = ["src"]
omit = ["tests/*", "*/conftest.py"]

[tool.coverage.report]
exclude_lines = [
    "pragma: no cover",
    "if TYPE_CHECKING:",
    "@abstractmethod",
]
```

---

## 8. CI/CD Pipeline

### GitHub Actions Workflow

```yaml
# .github/workflows/test.yml
name: Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      
      - name: Install dependencies
        run: |
          pip install -r requirements-test.txt
          apt-get install -y ros-humble-*
      
      - name: Run unit tests
        run: |
          pytest tests/unit/ \
            --cov=src \
            --cov-report=xml \
            --cov-fail-under=80
      
      - name: Upload coverage
        uses: codecov/codecov-action@v4
        with:
          files: ./coverage.xml

  integration-tests:
    runs-on: ubuntu-latest
    needs: unit-tests
    steps:
      - uses: actions/checkout@v4
      
      - name: Start services
        run: docker-compose up -d
      
      - name: Run integration tests
        run: pytest tests/integration/ -v
      
      - name: Run E2E tests
        run: pytest tests/e2e/ -v

  security-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Run Bandit
        run: pip install bandit && bandit -r src/
      
      - name: Run safety
        run: pip install safety && safety check
      
      - name: OWASP ZAP scan
        uses: zaproxy/action-api-scan@v0.8.0
        with:
          target: 'https://api.example.com/openapi.json'

  performance-tests:
    runs-on: ubuntu-latest
    needs: integration-tests
    steps:
      - uses: actions/checkout@v4
      
      - name: Run locust
        run: |
          locust -f tests/performance/locustfile.py \
            --headless \
            --users 100 \
            --spawn-rate 10 \
            --run-time 60s \
            --html=report.html

  docker-build:
    runs-on: ubuntu-latest
    needs: [unit-tests, security-tests]
    steps:
      - uses: actions/checkout@v4
      
      - name: Build Docker image
        run: docker build -t truck-drone-system:${{ github.sha }} .
      
      - name: Run container tests
        run: |
          docker run -d --name test-container truck-drone-system:${{ github.sha }}
          docker exec test-container pytest tests/smoke/
```

### Docker Compose Test

```yaml
# docker-compose.test.yml
version: '3.8'

services:
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: truckdrone_test
  
  redis:
    image: redis:7-alpine
  
  kafka:
    image: confluentinc/cp-kafka:7.5.0
  
  app:
    build: .
    depends_on:
      - postgres
      - redis
      - kafka
    environment:
      DATABASE_URL: postgresql://test:test@postgres:5432/truckdrone_test
      REDIS_URL: redis://redis:6379
      KAFKA_BOOTSTRAP_SERVERS: kafka:9092
```

---

## 9. Definition of Done

### Critères S11

- [ ] **Couverture tests ≥80%** sur tous modules
- [ ] **Tests unitaires** passent avec succès (≥150 tests)
- [ ] **Tests intégration** scénarios truck-drone validés (≥20 tests)
- [ ] **Tests performance** latence/throughput within SLA
- [ ] **Tests sécurité** aucun vulnérabilité HIGH/Critical
- [ ] **Tests fiabilité** failover/recovery validés
- [ ] **CI/CD pipeline** fonctionnel avec auto-run
- [ ] **Peer review** approuvée (`review_tests.md`)
- [ ] **Rapport coverage** généré et disponible

### Métriques Quality Gate

| Métrique | Seuil | Actuel |
|----------|-------|--------|
| Code Coverage | ≥80% | - |
| Test Pass Rate | 100% | - |
| Security Issues | 0 HIGH | - |
| Performance SLA | 100% | - |
| Flaky Tests | <1% | - |

---

## Livrables

| Fichier | Description |
|---------|-------------|
| `impl_tests.md` | Ce document |
| `review_tests.md` | Peer review |
| `tests/` | Suite tests complète |
| `.github/workflows/test.yml` | CI/CD pipeline |

---

## Références

- Architecture technologique: `playbook/04-architecture-technologique.md`
- Plan simulation: `playbook/10-simulation-plan.md`
- Tests ROS2: https://docs.ros.org/en/humble/Tutorials/Testing.html