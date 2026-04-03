# Implémentation S10: Application Mobile MVP

## Vue d'ensemble

| Critère | Valeur |
|---------|--------|
| **Framework** | Flutter 3.x |
| **Language** | Dart 3.x |
| **State Management** | Riverpod |
| **Architecture** | Clean Architecture (data/domain/presentation) |
| **Map Provider** | Mapbox GL |
| **Target Platforms** | iOS 14+, Android 8+ |

---

## 1. Spécification Technique Complète

### Dépendances Principales (`pubspec.yaml`)

```yaml
dependencies:
  flutter:
    sdk: flutter
  
  # State Management
  flutter_riverpod: ^2.4.9
  riverpod_annotation: ^2.3.3
  
  # Networking
  dio: ^5.3.3
  web_socket_channel: ^2.4.0
  connectivity_plus: ^5.0.2
  
  # Maps
  mapbox_gl: ^0.16.0
  flutter_mapbox_navigation: ^1.0.2
  
  # Storage
  sqflite: ^2.3.0
  flutter_secure_storage: ^9.0.0
  shared_preferences: ^2.2.2
  
  # Auth
  firebase_auth: ^4.7.3
  google_sign_in: ^6.1.5
  apple_sign_in: ^0.1.1
  otp: ^3.1.4
  
  # Push Notifications
  firebase_messaging: ^14.6.5
  flutter_local_notifications: ^16.1.0
  
  # UI
  flutter_map_marker_cluster: ^1.3.6
  cached_network_image: ^3.3.0
  shimmer: ^3.0.0
  skeleton_loader: ^0.5.0
  
  # Utils
  intl: ^0.18.1
  freezed_annotation: ^2.4.1
  json_annotation: ^4.8.1
  uuid: ^4.2.1
  geolocator: ^10.1.0
  permission_handler: ^11.1.0

dev_dependencies:
  flutter_test:
    sdk: flutter
  build_runner: ^2.4.7
  freezed: ^2.4.5
  json_serializable: ^6.7.1
  riverpod_generator: ^2.3.9
  flutter_lints: ^3.0.1
  mocktail: ^1.0.1
```

### Structure du Projet

```
lib/
├── main.dart
├── app.dart
├── core/
│   ├── constants/
│   │   ├── api_constants.dart
│   │   ├── app_constants.dart
│   │   └── storage_keys.dart
│   ├── errors/
│   │   ├── exceptions.dart
│   │   └── failures.dart
│   ├── network/
│   │   ├── api_client.dart
│   │   ├── api_interceptors.dart
│   │   └── websocket_client.dart
│   ├── security/
│   │   ├── certificate_pinning.dart
│   │   ├── jailbreak_detection.dart
│   │   └── encryption_helper.dart
│   └── utils/
│       ├── date_utils.dart
│       └── location_utils.dart
├── data/
│   ├── datasources/
│   │   ├── local/
│   │   │   ├── local_storage_datasource.dart
│   │   │   └── offline_cache_datasource.dart
│   │   └── remote/
│   │       ├── auth_remote_datasource.dart
│   │       ├── mission_remote_datasource.dart
│   │       ├── telemetry_remote_datasource.dart
│   │       ├── fleet_remote_datasource.dart
│   │       └── alerts_remote_datasource.dart
│   ├── models/
│   │   ├── user_model.dart
│   │   ├── mission_model.dart
│   │   ├── drone_model.dart
│   │   ├── truck_model.dart
│   │   ├── alert_model.dart
│   │   └── telemetry_model.dart
│   └── repositories/
│       ├── auth_repository_impl.dart
│       ├── mission_repository_impl.dart
│       ├── telemetry_repository_impl.dart
│       └── fleet_repository_impl.dart
├── domain/
│   ├── entities/
│   │   ├── user.dart
│   │   ├── mission.dart
│   │   ├── drone.dart
│   │   ├── truck.dart
│   │   ├── alert.dart
│   │   └── telemetry.dart
│   ├── repositories/
│   │   ├── auth_repository.dart
│   │   ├── mission_repository.dart
│   │   ├── telemetry_repository.dart
│   │   └── fleet_repository.dart
│   └── usecases/
│       ├── login_usecase.dart
│       ├── logout_usecase.dart
│       ├── get_missions_usecase.dart
│       ├── start_mission_usecase.dart
│       └── get_telemetry_usecase.dart
└── presentation/
    ├── providers/
    │   ├── auth_provider.dart
    │   ├── mission_provider.dart
    │   ├── telemetry_provider.dart
    │   ├── fleet_provider.dart
    │   └── connectivity_provider.dart
    ├── screens/
    │   ├── splash/
    │   │   └── splash_screen.dart
    │   ├── auth/
    │   │   ├── login_screen.dart
    │   │   ├── oauth_screen.dart
    │   │   └── totp_setup_screen.dart
    │   ├── dashboard/
    │   │   └── dashboard_screen.dart
    │   ├── map/
    │   │   └── map_screen.dart
    │   ├── history/
    │   │   └── history_screen.dart
    │   ├── alerts/
    │   │   └── alerts_screen.dart
    │   └── profile/
    │       └── profile_screen.dart
    └── widgets/
        ├── common/
        │   ├── app_button.dart
        │   ├── app_text_field.dart
        │   ├── loading_overlay.dart
        │   └── error_view.dart
        ├── map/
        │   ├── drone_marker.dart
        │   ├── truck_marker.dart
        │   ├── zone_marker.dart
        │   └── mission_cluster.dart
        ├── mission/
        │   ├── mission_card.dart
        │   ├── mission_progress.dart
        │   └── mission_stats.dart
        └── alerts/
            ├── alert_tile.dart
            └── alert_details.dart
```

---

## 2. Architecture UI

### Navigation (GoRouter)

```dart
// Router configuration
final router = GoRouter(
  initialLocation: '/splash',
  routes: [
    // Splash - no auth required
    GoRoute(path: '/splash', builder: (_, __) => const SplashScreen()),
    
    // Auth flow
    GoRoute(path: '/login', builder: (_, __) => const LoginScreen()),
    GoRoute(path: '/oauth/:provider', builder: (context, params) => 
      OAuthScreen(provider: params['provider']!)),
    GoRoute(path: '/totp-setup', builder: (_, __) => const TotpSetupScreen()),
    
    // Main app - auth required
    ShellRoute(
      builder: (context, state, child) => MainShell(child: child),
      routes: [
        GoRoute(path: '/dashboard', builder: (_, __) => const DashboardScreen()),
        GoRoute(path: '/map', builder: (_, __) => const MapScreen()),
        GoRoute(path: '/history', builder: (_, __) => const HistoryScreen()),
        GoRoute(path: '/alerts', builder: (_, __) => const AlertsScreen()),
        GoRoute(path: '/profile', builder: (_, __) => const ProfileScreen()),
      ],
    ),
  ],
  redirect: (context, state) {
    // Auth check redirect logic
  },
);
```

### Structure des Écrans

#### 2.1 Dashboard Screen
```
┌─────────────────────────────────┐
│ AppBar: "Mission en cours"  🔔  │
├─────────────────────────────────┤
│                                 │
│    [Interactive Map - 60%]     │
│    - Truck position            │
│    - Drone positions           │
│    - Collection zones          │
│                                 │
├─────────────────────────────────┤
│ Progress: ████████░░ 8/12 sacs │
├─────────────────────────────────┤
│ Stats Bar                       │
│ ⏱ 12:34  │  🔋 78%  │  📍 2.3km│
├─────────────────────────────────┤
│ Action Buttons                 │
│ [Pause]  [Cancel]  [Assist]    │
└─────────────────────────────────┘
```

#### 2.2 Map Screen
```
┌─────────────────────────────────┐
│ Search Bar + Filters      ⋮≡   │
├─────────────────────────────────┤
│                                 │
│    [Full Map View]              │
│    - Cluster markers            │
│    - Layer toggles              │
│    - Zoom controls              │
│                                 │
├─────────────────────────────────┤
│ Bottom Sheet: Selected Item     │
│ - Details + Actions             │
└─────────────────────────────────┘
```

#### 2.3 History Screen
```
┌─────────────────────────────────┐
│ Header: "Historique"      🔍    │
├─────────────────────────────────┤
│ Filters: [Date] [Drone] [Status]│
├─────────────────────────────────┤
│ ┌─────────────────────────────┐ │
│ │ Mission #142                │ │
│ │ ✅ Terminé • 45kg • 23min   │ │
│ └─────────────────────────────┘ │
│ ┌─────────────────────────────┐ │
│ │ Mission #141                │ │
│ │ ⚠️ Interrompu • 12kg • 15min│ │
│ └─────────────────────────────┘ │
│ ... (virtualized list)         │
└─────────────────────────────────┘
```

#### 2.4 Alerts Screen
```
┌─────────────────────────────────┐
│ Header: "Alertes"               │
├─────────────────────────────────┤
│ [All] [Critical] [Medium] [Low] │
├─────────────────────────────────┤
│ ┌─────────────────────────────┐ │
│ │ 🔴 Incident: Collision       │ │
│ │ Drone #7 • Il y a 2min      │ │
│ └─────────────────────────────┘ │
│ ┌─────────────────────────────┐ │
│ │ 🟡 Batterie faible          │ │
│ │ Drone #3 • 15% • Il y a 5min│ │
│ └─────────────────────────────┘ │
│ ... (timeline view)            │
└─────────────────────────────────┘
```

#### 2.5 Profile Screen
```
┌─────────────────────────────────┐
│ Header: "Profil"                │
├─────────────────────────────────┤
│     [Avatar]                    │
│     john.doe@company.com        │
│     Opérateur                   │
├─────────────────────────────────┤
│                                 │
│ 🔐 Sécurité                     │
│    2FA: Activé ✓                │
│    [Configurer]                │
│                                 │
│ 🔔 Notifications                │
│    [Mission] [Alertes] [Drone]  │
│                                 │
│ ℹ️ À propos                     │
│    Version 1.0.0                │
│                                 │
│ [Déconnexion]                   │
└─────────────────────────────────┘
```

---

## 3. API Integration

### Endpoints

#### 3.1 Auth API

```dart
// API client configuration
class ApiConstants {
  static const String baseUrl = 'https://api.drone-collect.io/api/v1';
  static const String wsUrl = 'wss://api.drone-collect.io/ws';
  
  // Auth endpoints
  static const String login = '/auth/login';
  static const String oauth = '/auth/oauth';
  static const String refresh = '/auth/refresh';
  static const String totpSetup = '/auth/2fa/setup';
  static const String totpVerify = '/auth/2fa/verify';
  
  // Mission endpoints
  static const String missions = '/missions';
  static const String missionPause = '/missions/{id}/pause';
  static const String missionResume = '/missions/{id}/resume';
  
  // Telemetry
  static const String telemetryLive = '/telemetry/live';
  static const String telemetryHistory = '/telemetry/history';
  
  // Fleet
  static const String drones = '/fleet/drones';
  static const String trucks = '/fleet/trucks';
  static const String droneStatus = '/fleet/status/{id}';
  
  // Alerts
  static const String alerts = '/alerts';
  static const String alertAck = '/alerts/{id}/ack';
  static const String alertEscalate = '/alerts/{id}/escalate';
}
```

#### 3.2 WebSocket Live Telemetry

```dart
class TelemetryWebSocket {
  final WebSocketChannel _channel;
  
  TelemetryWebSocket() : _channel = WebSocketChannel.connect(
    Uri.parse(ApiConstants.wsUrl),
  );
  
  Stream<TelemetryModel> get telemetryStream => _channel.stream
    .map((data) => TelemetryModel.fromJson(jsonDecode(data)));
  
  void subscribeToMission(String missionId) {
    _channel.sink.add(jsonEncode({
      'action': 'subscribe',
      'missionId': missionId,
    }));
  }
  
  void dispose() => _channel.sink.close();
}
```

#### 3.3 Repository Implementation

```dart
class MissionRepositoryImpl implements MissionRepository {
  final ApiClient _apiClient;
  final LocalStorage _localStorage;
  
  MissionRepositoryImpl(this._apiClient, this._localStorage);
  
  @override
  Future<List<Mission>> getMissions({
    int page = 1,
    int limit = 20,
    MissionStatus? status,
    DateTime? fromDate,
    DateTime? toDate,
  }) async {
    final queryParams = {
      'page': page.toString(),
      'limit': limit.toString(),
      if (status != null) 'status': status.name,
      if (fromDate != null) 'from': fromDate.toIso8601String(),
      if (toDate != null) 'to': toDate.toIso8601String(),
    };
    
    final response = await _apiClient.get(
      ApiConstants.missions,
      queryParams: queryParams,
    );
    
    return (response.data['missions'] as List)
        .map((json) => MissionModel.fromJson(json))
        .toList();
  }
  
  @override
  Future<Mission> createMission(CreateMissionParams params) async {
    final response = await _apiClient.post(
      ApiConstants.missions,
      data: params.toJson(),
    );
    
    return MissionModel.fromJson(response.data);
  }
  
  @override
  Future<void> pauseMission(String missionId) async {
    await _apiClient.post(
      ApiConstants.missionPause.replaceAll('{id}', missionId),
    );
  }
  
  @override
  Future<void> resumeMission(String missionId) async {
    await _apiClient.post(
      ApiConstants.missionResume.replaceAll('{id}', missionId),
    );
  }
}
```

---

## 4. Code Snippets des Principaux Composants

### 4.1 Riverpod Providers

```dart
// Auth State
@riverpod
class AuthNotifier extends _$AuthNotifier {
  @override
  AsyncValue<User?> build() {
    return const AsyncValue.loading();
  }
  
  Future<void> login(String email, String password) async {
    state = const AsyncValue.loading();
    try {
      final authRepo = ref.read(authRepositoryProvider);
      final user = await authRepo.login(email, password);
      state = AsyncValue.data(user);
    } catch (e, st) {
      state = AsyncValue.error(e, st);
    }
  }
  
  Future<void> logout() async {
    final authRepo = ref.read(authRepositoryProvider);
    await authRepo.logout();
    state = const AsyncValue.data(null);
  }
}

// Mission State
@riverpod
class MissionNotifier extends _$MissionNotifier {
  @override
  AsyncValue<Mission?> build() {
    return const AsyncValue.data(null);
  }
  
  Future<void> startMission(String zoneId, int droneCount) async {
    state = const AsyncValue.loading();
    try {
      final missionRepo = ref.read(missionRepositoryProvider);
      final mission = await missionRepo.createMission(
        CreateMissionParams(zoneId: zoneId, droneCount: droneCount),
      );
      state = AsyncValue.data(mission);
    } catch (e, st) {
      state = AsyncValue.error(e, st);
    }
  }
  
  Future<void> pause() async {
    final current = state.value;
    if (current == null) return;
    
    final missionRepo = ref.read(missionRepositoryProvider);
    await missionRepo.pauseMission(current.id);
    state = AsyncValue.data(current.copyWith(status: MissionStatus.paused));
  }
}

// Telemetry Stream
@riverpod
Stream<TelemetryModel> telemetryStream(TelemetryStreamRef ref) {
  final ws = TelemetryWebSocket();
  ref.onDispose(() => ws.dispose());
  return ws.telemetryStream;
}
```

### 4.2 Map Widget

```dart
class MissionMapWidget extends ConsumerStatefulWidget {
  final Mission? activeMission;
  
  const MissionMapWidget({super.key, this.activeMission});
  
  @override
  ConsumerState<MissionMapWidget> createState() => _MissionMapWidgetState();
}

class _MissionMapWidgetState extends ConsumerState<MissionMapWidget> {
  MapboxMapController? _mapController;
  
  @override
  Widget build(BuildContext context) {
    final telemetry = ref.watch(telemetryStreamProvider);
    
    return MapboxMap(
      accessToken: Environment.mapboxToken,
      initialCameraPosition: const CameraPosition(
        target: LatLng(48.8566, 2.3522), // Paris
        zoom: 14,
      ),
      onMapCreated: (controller) => _mapController = controller,
      markers: _buildMarkers(telemetry.value),
      onStyleLoadedCallback: () => _setupMapLayers(),
    );
  }
  
  Set<Marker> _buildMarkers(AsyncValue<TelemetryModel?> telemetry) {
    final markers = <Marker>{};
    
    // Truck marker
    if (widget.activeMission?.truckPosition != null) {
      markers.add(Marker(
        id: 'truck',
        position: widget.activeMission!.truckPosition!,
        child: const TruckMarkerWidget(),
      ));
    }
    
    // Drone markers
    if (telemetry.hasValue) {
      for (final drone in telemetry.value!.drones) {
        markers.add(Marker(
          id: 'drone-${drone.id}',
          position: drone.position,
          child: DroneMarkerWidget(
            drone: drone,
            isSelected: _selectedDroneId == drone.id,
          ),
        ));
      }
    }
    
    return markers;
  }
}
```

### 4.3 Offline Support

```dart
class OfflineManager {
  final LocalStorage _storage;
  final Connectivity _connectivity;
  
  OfflineManager(this._storage, this._connectivity);
  
  Future<void> cacheMission(Mission mission) async {
    await _storage.write(
      key: 'cached_mission_${mission.id}',
      value: jsonEncode(mission.toJson()),
    );
  }
  
  Future<void> queueAction(OfflineAction action) async {
    final queue = await _getActionQueue();
    queue.add(action);
    await _storage.write(
      key: 'offline_queue',
      value: jsonEncode(queue.map((a) => a.toJson()).toList()),
    );
  }
  
  Future<void> syncQueue() async {
    final isOnline = await _connectivity.checkConnectivity();
    if (!isOnline.contains(ConnectivityResult.none)) return;
    
    final queue = await _getActionQueue();
    final apiClient = ApiClient();
    
    for (final action in queue) {
      try {
        await apiClient.request(action.endpoint, action.method, action.data);
        queue.remove(action);
      } catch (e) {
        // Keep in queue for retry
      }
    }
    
    await _storage.write(
      key: 'offline_queue',
      value: jsonEncode(queue.map((a) => a.toJson()).toList()),
    );
  }
}
```

### 4.4 Push Notifications Handler

```dart
class NotificationHandler {
  final FirebaseMessaging _messaging;
  final NavigationService _navigation;
  
  NotificationHandler(this._messaging, this._navigation);
  
  Future<void> initialize() async {
    await _messaging.requestPermission();
    final token = await _messaging.getToken();
    await _sendTokenToServer(token);
    
    _messaging.onMessage.listen(_handleForegroundNotification);
    _messaging.onBackgroundMessage.handle(_handleBackgroundNotification);
  }
  
  void _handleForegroundNotification(RemoteMessage message) {
    final type = message.data['type'];
    switch (type) {
      case 'mission_start':
        _navigation.showSnackBar('Mission démarrée');
        break;
      case 'mission_complete':
        _navigation.showSnackBar('Mission terminée');
        break;
      case 'incident':
        _navigation.navigateTo('/alerts', extra: message.data['alertId']);
        break;
    }
  }
}
```

---

## 5. Configuration

### 5.1 Environment Configuration

```dart
class Environment {
  static const String apiBaseUrl = String.fromEnvironment(
    'API_BASE_URL',
    defaultValue: 'https://api.drone-collect.io/api/v1',
  );
  
  static const String mapboxToken = String.fromEnvironment(
    'MAPBOX_TOKEN',
    defaultValue: '',
  );
  
  static const String firebaseProjectId = String.fromEnvironment(
    'FIREBASE_PROJECT_ID',
    defaultValue: 'drone-collect-prod',
  );
  
  static const Duration tokenExpiry = Duration(minutes: 15);
  static const Duration refreshTokenExpiry = Duration(days: 7);
}
```

### 5.2 Platform-Specific Configuration

#### iOS (`ios/Runner/Info.plist`)
```xml
<key>NSLocationWhenInUseUsageDescription</key>
<string>Pour afficher votre position sur la carte</string>
<key>UIBackgroundModes</key>
<array>
  <string>fetch</string>
  <string>remote-notification</string>
</array>
```

#### Android (`android/app/src/main/AndroidManifest.xml`)
```xml
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
```

### 5.3 Security Configuration

```dart
// Certificate Pinning
class CertificatePinningConfig {
  static final List<Pin> primaryPins = [
    Pin(sha256: 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA='),
  ];
  
  static final List<Pin> backupPins = [
    Pin(sha256: 'BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB='),
  ];
}

// Jailbreak Detection
class DeviceSecurity {
  static Future<bool> isDeviceSecure() async {
    if (Platform.isIOS) {
      return await jailbreakCheck();
    } else if (Platform.isAndroid) {
      return await rootCheck();
    }
    return false;
  }
}
```

---

## 6. Critères Definition of Done

### Checklist

| # | Critère | Description |
|---|---------|-------------|
| 1 | **Build succès** | `flutter build ios` et `flutter build apk` compilent sans erreur |
| 2 | **Auth fonctionnelle** | Login email/password + OAuth Google/Apple fonctionnent |
| 3 | **Dashboard opérationnel** | Carte, progression, stats temps réel affichés |
| 4 | **Map interactive** | Marqueurs drones/camions, clustering, filtres fonctionnels |
| 5 | **Historique chargé** | Liste missions avec pagination, filtres date/status |
| 6 | **Alertes reçues** | Notifications push + affichage liste avec actions |
| 7 | **Profil editable** | Infos utilisateur, 2FA setup, préférences notifications |
| 8 | **Mode offline** | Cache positions, queue actions, indicateur offline |
| 9 | **Tests unitaires** | >70% coverage sur core et presentation layers |
| 10 | **Peer review** | Code review par second développeur approuvée |
| 11 | **Documentation** | README avec setup instructions et architecture |
| 12 | **Security audit** | Certificate pinning, secure storage, no hardcoded secrets |

### Critères d'Acceptation Utilisateur

- [ ] L'utilisateur peut se connecter avec email/password
- [ ] L'utilisateur peut se connecter avec Google/Apple OAuth
- [ ] L'utilisateur voit la position du camion en temps réel sur une carte
- [ ] L'utilisateur voit les drones actifs sur la carte
- [ ] L'utilisateur voit la progression de la mission (sacs collectés/total)
- [ ] L'utilisateur peut démarrer une nouvelle mission
- [ ] L'utilisateur peut mettre en pause/reprendre une mission
- [ ] L'utilisateur peut voir l'historique des missions
- [ ] L'utilisateur reçoit des notifications push pour les incidents
- [ ] L'utilisateur peut configurer 2FA
- [ ] L'application fonctionne hors ligne (mode dégradé)
- [ ] L'application est accessible (support TalkBack/VoiceOver)

---

## 7.Livrables

| Fichier | Description |
|---------|-------------|
| `src/mobile_app/` | Projet Flutter complet |
| `playbook/impl/impl_mobile_mvp.md` | Cette spécification |
| `playbook/impl/review_mobile_mvp.md` | Peer review |
| `README.md` | Documentation utilisateur/ développeur |

---

## 8. Prochaines Étapes (S11)

- Tests de fiabilité (stress tests, edge cases)
- Tests de sécurité (penetration testing)
- Tests de performance (lazy loading, memory usage)
- Documentation utilisateur finale
- Préparation release 1.0