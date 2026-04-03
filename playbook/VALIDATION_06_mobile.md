# Délibération - Architecture Mobile

**Document évalué :** `08-application-mobile.md`  
**Date :** 2026-03-31  
**Comité :** 3 experts (Cross-platform, UX, Sécurité)

---

## Expert 1 : Développeur Flutter/React Native (Performances Cross-platform)

### Points Forts
- **Stack technique complète et cohérente** : Choix de Flutter ou React Native avec écosystème moderne (Redux Toolkit, Riverpod, GoRouter). Bonne couverture des besoins.
- **WebSocket bien intégré** : Architecture temps réel prévue avec Socket.io pour le tracking. Bon choix pour les mises à jour fréquentes.
- **Architecture API REST + WebSocket** : Séparation claire entre endpoints REST (données statiques) et flux temps réel (tracking).
- **Gestion d'état prévue** : Redux Toolkit et Riverpod sont des solutions éprouvées pour les applications complexes.
- **Stockage sécurisé** : `react-native-keychain` et `flutter_secure_storage` sont les bonnes pratiques.

### Points Faibles
- **Absence de mention de performance** : Pas de stratégie de lazy loading, virtualization des listes, ou optimisation des images.
- **Choix non tranché Flutter/React Native** : Document suggère "ou" sans justification. Impacte architecture et maintenance.
- **Pas de stratégie offline-first** : Fonctionnement en mode dégradé non mentionné (zones rurales, réseau faible).
- **Maps provider non tranché** : Mapbox vs Google Maps a des implications de coût et performance à considérer.
- **Absence de code splitting** : Pas de mention de bundle splitting ou charged dynamic imports pour réduire le poids de l'app.

### Recommandations
1. **Choisir définitivement Flutter ou React Native** en fonction de l'expertise interne et des besoins de performance spécifiques.
2. **Implémenter une stratégie offline-first** : SQLite/Realm pour cache local, synchronisation différée.
3. **Ajouter virtualization** pour les listes (FlatList optimisé, ListView.builder Flutter) avec pagination.
4. **Optimiser les images** : Utiliser WebP, lazy loading, et cache local avec `fast_image` (RN) ou `cached_network_image` (Flutter).
5. **Configurer code splitting** : React Native lazy require ou Flutter deferred libraries.

### Verdict Expert 1
> **APPROUVÉ AVEC RÉSERVES** ✅  
> L'architecture globale est solide et moderne. Cependant, le manque de choix tranché entre Flutter/React Native et l'absence de stratégie offline-first sont des lacunes critiques pour une application grand public.

---

## Expert 2 : Designer UX Mobile (Ergonomie, Material Design/Cupertino)

### Points Forts
- **Couverture fonctionnelle exhaustive** : 7 écrans principaux couvrant tout le parcours utilisateur. Flow logique et complet.
- **Navigation bien pensée** : Deep links définis (`myapp://tracking/{missionId}`), structure de navigation claire.
- **FAB bien utilisé** : Le bouton flottant "Demander une collecte" est à bonne position (bottom-right, zone de confiance pour thumb).
- **Timeline visuelle** : Suivi par étapes (validée → en route → arrivée → collecte → terminée) - excellent pour l'UX.
- **Récapitulatif avant confirmation** : Étape de validation intermédiaire - bonne pratique pour réduire les erreurs.
- **Système de notification structuré** : Push, email, SMS avec préférences modifiables - respect du choix utilisateur.

### Points Faibles
- **Écran d'inscription surchargé** : 5 champs (nom, email, téléphone, mot de passe, confirmation) = friction excessive. Séparer en étapes (multi-step form).
- **Pas de skeleton loading** : Les états de chargement ne sont pas mentionnés. Ajout de skeleton UI essentiel pour la perception de vitesse.
- **Carte interactive sans fallback** : Si la map ne charge pas ou en mode offline, pas de solution de repli (input adresse manuel).
- **Feedback haptic manquant** : Pas de mention de retour haptique pour les actions importantes (confirmation paiement, validation).
- **Accessibilité négligée** : Pas de mention WCAG, contraste, tailles de texte ajustables, support TalkBack/VoiceOver.
- **Motion design absent** : Transitions entre écrans non définies. Laisse le développeur sans référence.

### Recommandations
1. **Refaire l'inscription en wizard** : 2-3 étapes (identifiants → infos personnelles → validation).
2. **Ajouter skeleton loaders** sur Dashboard, Historique et Tracking pendant le chargement API.
3. **Implémenter fallback adresse** : Si map fails, proposer saisie manuelle avec autocomplete (Google Places API).
4. **Intégrer haptic feedback** : `HapticFeedback.mediumImpact()` sur confirmations critiques.
5. **Audit accessibilité** : Contraste AA minimum, support TalkBack/VoiceOver, texte redimensionnable.
6. **Définir transitions** : Fade pour navigation secondaire, slide pour modales, shared element transitions pour карт.

### Verdict Expert 2
> **APPROUVÉ** ✅  
> L'architecture fonctionnelle est excellente et les flux sont bien pensés. Les améliorations préconisées concernent l'accessibilité, la performance perçue (skeletons), et l'ergonomie des formulaires - tous des classiques de l'optimisation mobile.

---

## Expert 3 : Expert Sécurité Mobile (Biométrie, Certificate Pinning, Données)

### Points Forts
- **Architecture JWT robuste** : Access token 15min + refresh token 7 jours avec stockage sécurisé. Bonne pratiques oauth.
- **Biométrie prévue** : Face ID/Touch ID et Android BiometricPrompt mentionnés. Protection additionnelle forte.
- **Certificate pinning prévu** : Bonne initiative pour les endpoints critiques. Devrait être implémenté avec pins durcis.
- **Chiffrement local** : AES-256 pour données sensibles, Keychain/Keystore pour tokens. Niveau entreprise.
- **TLS 1.3 obligatoire** : Standard moderne, bien spécifié.
- **PCI DSS compliant** : Délégation à Stripe/Adyen - bon choix, pas de stockage local de données carte.
- **Rate limiting et détection fraude** : Limitation côté API etDeviceID - protection contre abus.

### Points Faibles
- **Biométrie "optionnelle"** : Dangerous. La biométrie devrait être un complément, pas une alternative au mot de passe. Risque: bypass facile.
- **Pas de jailbreak/root detection** : Application vulnérable sur appareils compromis. Aucune protection mentionnée.
- **Certificate pinning details manquants** : "Prévu" sans implementation (pinsets, backup pins, rotation).
- **Pas de MFA besides TOTP** : Pas de mention d'email/SMS verification besides 2FA code. Considerer MFA strength.
- **Logs de sécurité non spécifiés** : Pas de stratégie de logging des events sécurité (échecs login, tentatives accès).
- **WebSocket WSS confirmé mais pas de validation** : Pas de mention de heartbeats, reconnection logic, ou message validation.
- **Absence de secure coding guidelines** : Pas de mention d'obfuscation, anti-tampering, ou runtime protection.

### Recommandations
1. **Renforcer biométrie** : L'utiliser comme second facteur (après mot de passe), jamais comme seul facteur. Ajouter "biometric + PIN" fallback.
2. **Implémenter jailbreak/root detection** : Librairies comme `react-native-root檢查` ou plugin Flutter equivalent. Bloquer ou warning.
3. **Détailler certificate pinning** :
   - Pinsets avec clés publiques certificat (SPKI)
   - Backup pins pour rotation
   - Validation hostname + certificate
   - HPKP ou Custom TrustManager
4. **Ajouter detection d'émulateur** : Prévention d'exécution en émulateur (Fraude, testing unauthorized).
5. **Implémenter secure logging** : Log des événements sécurité sans exposer données sensibles (timestamp, event type, anonymized user ID).
6. **WebSocket hardening** :
   - Heartbeat every 30s
   - Reconnection exponential backoff
   - Message signing pour éviter injection
   - Max reconnections avant lockout

### Verdict Expert 3
> **APPROUVÉ AVEC RÉSERVES** ✅  
> Les fondations sécurité sont bonnes (JWT, TLS, chiffrement). Cependant, la biométrie "optionnelle seule" est un anti-pattern dangereux et le pinning manque de détails d'implémentation. Ces points doivent être renforcés avant production.

---

## Délibération Commune

### Synthèse des Verdicts

| Expert | Verdict | Condition |
|--------|---------|-----------|
| Cross-platform | ✅ Approouvé avec réserves | Offline-first + choix tech tranché |
| UX | ✅ Approoué | Accessibilité + skeletons |
| Sécurité | ✅ Approoué avec réserves | Biométrie + pinning |

### Points Forts Communs
- Architecture fonctionnelle complète et cohérente
- Stack technique moderne et pertinente
- Sécurité des données utilisateurs bien pensée
- Expérience utilisateur fluide (deep links, notifications)

### Points Faibles Communs
- Absence de stratégie offline-first
- Choix technique non tranché (Flutter vs React Native)
- Détails d'implémentation sécurité manquants
- Accessibilité non traitée

### Recommandations Prioritaires (Roadmap)

**Phase 1 (Critique - Avant Prod)**
1. Choisir définitivement Flutter ou React Native
2. Implémenter certificate pinning détaillé
3. Renforcer biométrie (second facteur, pas alternatif)
4. Ajouter jailbreak/root detection

**Phase 2 (Amélioration UX)**
5. Stratégie offline-first avec SQLite/Realm
6. Skeleton loaders sur tous les écrans
7. Accessibilité WCAG AA
8. Formulaire inscription multi-étapes

**Phase 3 (Optimisation)**
9. Virtualisation des listes
10. Haptic feedback
11. Transitions et animations fluides
12. Test performance sur appareils bas de gamme

### Verdict Final

> **APPROUVÉ** ✅  
> 
> Le document d'architecture mobile est **solide et bien structuré**. Il couvre l'ensemble des besoins fonctionnels avec une stack technique appropriée. Les 3 experts valide le document avec des réserves mineures qui concernent principalement des détails d'implémentation et non la architecture globale.
>
> **Action requise :** Valider le choix Flutter/React Native rapidement pour permettre l'implémentation. Les réserves de sécurité doivent être adressées avant mise en production.

---

*Préparé par le comité d'évaluation mobile*