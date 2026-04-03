# Comité Ingénierie Véhicule & Drones - Délibération

## Date: 2026-04-01

## Documents revus
- 05-camion.md (1340 octets)
- 06-drone.md (1138 octets)

---

## Points validés ✅

### Document 05 - Camion
- Architecture mécanique cohérente (châssis, trémie, capteurs)
- Stack ROS2 bien définie (nodes, topics, services/actions)
- Liste exhaustive des capteurs (LiDAR, stéréo, radar, RTK, IMU)
- Système de sécurité (E-stop, anti-obstacle) présent

### Document 06 - Drone
- Concept quad/octocopter cargo approprié
- Communication 5G + Wi-Fi mesh + LoRa (tri-bande intelligent)
- Nodes ROS2 complets
- Logique de docking et failsafe documentée

---

## Points à corriger ⚠️

### Document 05 - Camion
1. **Capteurs redondants non mentionnés** - Pas de fallback si LiDAR échoue
2. **Autonomie/energie manquante** - Consumption estimée? Capacité réservoir?
3. **Interface trémie vague** - Comment les drones indiquent qu'ils sont prêts?
4. **Vitesse max/non** - "navigation en mouvement continu" mais pas de vitesse cible

### Document 06 - Drone
1. **Masse/charge utile manquante** - Contraintes "masse max + charge utile + autonomie" sans chiffres
2. **Nombre optimal de rotors** - Quad vs Octo non justifié (octo = plus stable mais plus lourd)
3. **Capacité de capture** - Un drone peut transporter combien de sacs? Un seul?
4. **Gestion thermique** - Pas mentionné (vol stationnaire au-dessus du camion = échauffement)

---

## Décisions

| Point | Décision | Action |
|-------|-----------|--------|
| Camion: redondance | Ajouter LiDAR secondaire + caméra360 comme backup | Corriger 05 |
| Drone: capacité | Spécifier 1 sac de 50L max par voyage | Corriger 06 |
| Drone: autonomie | Estimer 15-20 min avec charge | Corriger 06 |
| Vitesse camion | Définir 30-50 km/h cible | Corriger 05 |

---

## Résumé
- **Validé avec corrections** pour 05 et 06
- Les deux documents nécessitent des spécifications plus précises
- L'interface camion-drone (docking) doit être mieux définie