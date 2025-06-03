# DrumBox Multiplayer - Boîte à rythmes collaborative avec système de salons

## Description

Application desktop de boîte à rythmes multijoueur développée avec Qt 6 et C++. Permet de créer des séquences rythmiques collaboratives en temps réel via le réseau local avec un système complet de salons (rooms) pouvant accueillir jusqu'à 4 joueurs par défaut.

## Fonctionnalités

### ✅ Implémentées (v2.0)
- **Séquenceur par grille** : Interface 8 instruments × 16 steps
- **Contrôle de lecture** : Play/Pause/Stop avec réglage du tempo
- **Système de salons** : Création/rejoin de rooms avec mot de passe optionnel
- **Gestion d'utilisateurs** : Jusqu'à 8 joueurs par salon avec identification couleur
- **Hôte de salon** : Transfert de rôle, expulsion d'utilisateurs
- **Réseau multijoueur** : Sessions client/serveur via TCP
- **Synchronisation temps réel** : Grilles synchronisées entre tous les clients
- **Interface adaptative** : Mode lobby et mode jeu

### 🚧 À implémenter (bonus)
- Chat intégré dans les salons
- Sauvegarde/chargement de sessions
- Support audio amélioré avec samples personnalisés
- Système de permissions avancé
- Historique des sessions

## Nouveautés v2.0 - Système de Salons

### Fonctionnalités des salons
- **Création de salon** : Nom personnalisé, mot de passe optionnel, limite d'utilisateurs (2-8)
- **Navigation** : Liste des salons publics avec indicateurs de statut
- **Gestion d'hôte** : L'hôte peut expulser des utilisateurs et transférer son rôle
- **Identification visuelle** : Couleurs uniques pour chaque utilisateur
- **Statut en ligne** : Indication des utilisateurs connectés/déconnectés

### Interface utilisateur
- **Mode Lobby** : Connexion réseau et navigation des salons
- **Mode Jeu** : Interface de création musicale collaborative
- **Liste d'utilisateurs** : Affichage temps réel des participants
- **Contrôles d'hôte** : Menu contextuel pour la gestion du salon

## Structure du projet

```
DrumBoxMultiplayer/
├── CMakeLists.txt
├── include/
│   ├── MainWindow.h          # Interface principale avec modes lobby/jeu
│   ├── DrumGrid.h           # Grille de séquenceur
│   ├── AudioEngine.h        # Moteur audio
│   ├── NetworkManager.h     # Gestionnaire réseau abstrait
│   ├── DrumServer.h         # Serveur TCP
│   ├── DrumClient.h         # Client TCP
│   ├── Protocol.h           # Protocole de communication
│   ├── Room.h               # Modèle de salon
│   # DrumBox Multiplayer - Boîte à rythmes collaborative

## Description

Application desktop de boîte à rythmes multijoueur développée avec Qt 6 et C++. Permet de créer des séquences rythmiques collaboratives en temps réel via le réseau local.

## Fonctionnalités

### ✅ Implémentées (v1.0)
- **Séquenceur par grille** : Interface 8 instruments × 16 steps
- **Contrôle de lecture** : Play/Pause/Stop avec réglage du tempo
- **Réseau multijoueur** : Sessions client/serveur via TCP
- **Synchronisation temps réel** : Grilles synchronisées entre clients
- **Identification utilisateur** : Nom et couleurs par utilisateur

### 🚧 À implémenter (bonus)
- Chat intégré
- Sauvegarde/chargement de sessions
- Support audio amélioré
- Interface utilisateur plus avancée

## Structure du projet

```
DrumBoxMultiplayer/
├── CMakeLists.txt
├── include/
│   ├── MainWindow.h
│   ├── DrumGrid.h
│   ├── AudioEngine.h
│   ├── NetworkManager.h
│   ├── DrumServer.h
│   ├── DrumClient.h
│   └── Protocol.h
├── src/
│   ├── main.cpp
│   ├── MainWindow.cpp
│   ├── DrumGrid.cpp
│   ├── AudioEngine.cpp
│   ├── NetworkManager.cpp
│   ├── DrumServer.cpp
│   ├── DrumClient.cpp
│   └── Protocol.cpp
└── samples/ (optionnel)
    ├── kick.wav
    ├── snare.wav
    ├── hihat.wav
    └── ...
```

## Architecture

### Classes principales

1. **MainWindow** : Interface utilisateur principale
2. **DrumGrid** : Grille de séquenceur avec gestion du timing
3. **AudioEngine** : Gestion de la lecture audio
4. **NetworkManager** : Abstraction client/serveur
5. **Protocol** : Sérialisation des