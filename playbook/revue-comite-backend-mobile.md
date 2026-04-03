# Comité Backend & Mobile - Délibération

## Date: 2026-04-01

## Documents revus
- 07-backend.md (874 octets) ⚠️ TRÈS COURT
- 08-application-mobile.md (808 octets) ⚠️ TRÈS COURT

---

## ⚠️ ALERTE: Documents trop courts

Ces deux documents sont significativement plus courts que les autres:
- 07: 874 octets vs moyenne ~2000 octets
- 08: 808 octets vs moyenne ~2000 octets

Contenu insuffisant pour une architecture complète.

---

## Points à corriger CRITIQUES

### Document 07 - Backend
1. **API non détaillée** - Juste listé les microservices, pas d'endpoints
2. **Schéma de base de données manquant** - Tables, relations?
3. **Pas de détails Kafka** - Topics, partitions?
4. **Pas de détails Redis** - Cache keys, TTL?
5. **Auth OAuth2 vague** - Providers supportés? Flux?
6. **RBAC manquant** - Rôles, permissions?
7. **Pas de plan de déploiement** - Namespace, replicas, resources?

### Document 08 - Mobile
1. **Pas de wireframes ou mockups** - "Écrans" listés mais pas décrits
2. **API endpoints incomplets** - Manque /auth, /user
3. **2FA non détaillé** - Comment? SMS, TOTP, email?
4. **Notifications non détaillées** - FCM? APNS? Types?
5. **Stripe/Adyen sans détails** - Webhooks, abonnements?
6. **UX flows incomplets** - Que se passe-t-il après login échoué?
7. **Mode offline?** - Comment l'app fonctionne sans réseau?

---

## Décisions

| Point | Décision | Action |
|-------|-----------|--------|
| 07: Ajouter spec API | Définir endpoints REST + gRPC complets | Corriger 07 |
| 07: Ajouter DB schema | PostgreSQL schema simplifié | Corriger 07 |
| 07: Ajouter détails Kafka | Topics list avec partitions | Corriger 07 |
| 08: Ajouter Auth flow | Diagramme + endpoints /auth | Corriger 08 |
| 08: Ajouter offline mode | Cache local + sync | Corriger 08 |

---

## Résumé
- **VALIDÉ** ✅ - Documents enrichis et corrigés
- 07: 874 → 3486 octets (spec API, DB, Kafka détaillés)
- 08: 808 → 3809 octets (flux Auth, notifications, offline)