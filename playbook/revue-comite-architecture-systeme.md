# Comité Architecture Système - Délibération

## Date: 2026-04-01

## Documents revus
- 02-architecture-haut-niveau.md (5614 octets)
- 03-architecture-fonctionnelle.md (1531 octets)
- 04-architecture-technologique.md (946 octets)

---

## Points validés ✅

### Document 02
- Diagramme logique complet et cohérent
- Rôles clairement définis (Camion, Drones, Backend, Mobile, Orchestrateur)
- Contraintes critiques réalistes (latence <100ms, disponibilté 99.9%)
- Modules logiciels bien détaillés avec nodes ROS2
- Plan d'action séquentiel pertinent

### Document 03
- Cas d'usage couverts (5 flux principaux)
- Séquences temporelles documentées
- Pré/post conditions définis

### Document 04
- Stack technologique cohérente (ROS2, DDS, Kubernetes)
- Choix de sécurité appropriés (mTLS, JWT, HSM)

---

## Points à corriger ⚠️

### Document 03
1. **Manque de détail sur les flux** - Les 5 cas d'usage sont列表 mais pas détaillés en user stories (Given/When/Then)
2. **Pas de diagramme de séquence** - Mentionné dans la story mais absent
3. **Préconditions incomplètes** -GPS valide mentionné, mais pas réseau, autorisations de vol FAA/EASA

### Document 04
1. **Version ROS2 non justifiée** - "Humble / Foxy" →，应该 préciser la version choisie et pourquoi
2. **Choix simulateur ignoré** - "Gazebo + Ignition" listé mais Ignition est deprecated (devenu Fortress/Garden)
3. **Pas de具体部署方案** - Kubernetes mentionné mais pas de détails (GKE, EKS, on-prem?)
4. **QoS ROS2 mentionné mais pas détaillé** - QoS options non spécifiées

---

## Décisions

| Point | Décision | Action |
|-------|-----------|--------|
| ROS2 version | Utiliser ROS2 Humble (LTS, jusqu'à 2027) | Corriger 04 |
| Simulateur | Gazebo + Fortress (ou Garden) | Corriger 04 |
| User stories 03 | Ajouter 2-3 examples détaillés | Corriger 03 |
| Diagramme séquence | Ajouter à 03 | Corriger 03 |

---

## Résumé
- **Validé avec corrections mineures** pour 02, 03, 04
- Documents réutilisables après corrections listées ci-dessus