#include "MainWindow.h"
#include "RoomManager.h"
#include "RoomListWidget.h"
#include "UserListWidget.h"

#include <QTimer>
#include "DrumServer.h"
#include <QJsonArray>
#include <QJsonObject>
#include "Protocol.h"
#include "DrumClient.h"
#include "Room.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_drumGrid(new DrumGrid(this))
    , m_audioEngine(new AudioEngine(this))
    , m_networkManager(new NetworkManager(this))
    , m_roomManager(new RoomManager(this))
    , m_roomListWidget(new RoomListWidget(this))
    , m_userListWidget(new UserListWidget(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_isPlaying(false)
    , m_inGameMode(false)
{
    // Générer un ID utilisateur unique
    m_currentUserId = QUuid::createUuid().toString();
    m_currentUserName = "User";

    setupUI();
    setupMenus();
    setupStatusBar();

    // Connexions audio
    connect(m_drumGrid, &DrumGrid::stepTriggered, this, [this](int step, const QList<int> &instruments) {
        m_audioEngine->playMultipleInstruments(instruments);
    });
    connect(m_drumGrid, &DrumGrid::cellClicked, this, &MainWindow::onGridCellClicked);
    connect(m_drumGrid, &DrumGrid::stepTriggered, this, &MainWindow::onStepTriggered);

    // Connexions réseau
    connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_networkManager, &NetworkManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(m_networkManager, &NetworkManager::clientDisconnected, this, &MainWindow::onClientDisconnected);
    connect(m_networkManager, &NetworkManager::connectionEstablished, this, &MainWindow::onConnectionEstablished);
    connect(m_networkManager, &NetworkManager::connectionLost, this, &MainWindow::onConnectionLost);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);

    // Connexions room list
    connect(m_roomListWidget, &RoomListWidget::createRoomRequested, this, &MainWindow::onCreateRoomRequested);
    connect(m_roomListWidget, &RoomListWidget::joinRoomRequested, this, &MainWindow::onJoinRoomRequested);
    connect(m_roomListWidget, &RoomListWidget::refreshRequested, this, &MainWindow::onRefreshRoomsRequested);

    // Connexions user list
    connect(m_userListWidget, &UserListWidget::leaveRoomRequested, this, &MainWindow::onLeaveRoomRequested);
    connect(m_userListWidget, &UserListWidget::kickUserRequested, this, &MainWindow::onKickUserRequested);
    connect(m_userListWidget, &UserListWidget::transferHostRequested, this, &MainWindow::onTransferHostRequested);

    // Configuration audio
    m_audioEngine->loadSamples();

    // NE PAS démarrer le serveur automatiquement ici
    // Le serveur sera démarré quand l'utilisateur choisit "Héberger"
    if (m_networkManager->getClient()) {
        connect(m_networkManager->getClient(), &DrumClient::gridCellUpdated,
                m_drumGrid, &DrumGrid::applyGridUpdate, Qt::UniqueConnection);
    }
    // État initial
    switchToLobbyMode();
    updateNetworkStatus();
}

void MainWindow::startServer() {
    if (m_networkManager->startServer(8888)) {
        qDebug() << "[MAINWINDOW] Serveur démarré avec succès";
        if (m_networkManager->getServer()) {
            m_networkManager->getServer()->setRoomManager(m_roomManager);
            qDebug() << "[MAINWINDOW] RoomManager partagé avec le serveur";
            // Test immédiat
            QList<Room*> rooms = m_roomManager->getPublicRooms();
            qDebug() << "[MAINWINDOW] Salles visibles après partage:" << rooms.size();
        } else {
            qWarning() << "[MAINWINDOW] Erreur : serveur non trouvé après démarrage";
        }
    } else {
        qWarning() << "[MAINWINDOW] Échec du démarrage du serveur";
    }
}




MainWindow::~MainWindow()
{
    // Nettoyer les connexions
    if (m_networkManager->isServerRunning())
    {
        m_networkManager->stopServer();
    }
    else if (m_networkManager->isClientConnected())
    {
        m_networkManager->disconnectFromServer();
    }
}


void MainWindow::onConnectionEstablished()
{
    qDebug() << "[MAINWINDOW] Connexion établie";
    updateNetworkStatus();

    if (m_networkManager->isClientConnected() && m_networkManager->getClient()) {
        // Connect the signal for receiving the room list
        connect(m_networkManager->getClient(), &DrumClient::roomListReceived,
                this, &MainWindow::onRoomListReceived, Qt::UniqueConnection);

        // Connect the signal for receiving the room state
        connect(m_networkManager->getClient(), &DrumClient::roomStateReceived,
                this, &MainWindow::onRoomStateReceived, Qt::UniqueConnection);

        QTimer::singleShot(200, this, [this]() {
            if (m_networkManager->getClient()) {
                m_networkManager->getClient()->requestRoomList();
            }
        });
    }
}




void MainWindow::setupUI()
{
    // Widget central
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Groupe réseau (mode lobby)
    m_networkGroup = new QGroupBox("Connexion Réseau", this);
    QGridLayout *networkLayout = new QGridLayout(m_networkGroup);

    // Contrôles réseau
    m_userNameEdit = new QLineEdit("Joueur", this);
    m_serverAddressEdit = new QLineEdit("localhost", this);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(8888);

    m_startServerBtn = new QPushButton("Héberger", this);
    m_connectBtn = new QPushButton("Se Connecter", this);
    m_disconnectBtn = new QPushButton("Déconnecter", this);
    m_disconnectBtn->setEnabled(false);

    m_networkStatusLabel = new QLabel("Déconnecté", this);

    // Layout réseau
    networkLayout->addWidget(new QLabel("Nom d'utilisateur:", this), 0, 0);
    networkLayout->addWidget(m_userNameEdit, 0, 1, 1, 2);
    networkLayout->addWidget(new QLabel("Adresse serveur:", this), 1, 0);
    networkLayout->addWidget(m_serverAddressEdit, 1, 1);
    networkLayout->addWidget(m_portSpin, 1, 2);
    networkLayout->addWidget(m_startServerBtn, 2, 0);
    networkLayout->addWidget(m_connectBtn, 2, 1);
    networkLayout->addWidget(m_disconnectBtn, 2, 2);
    networkLayout->addWidget(new QLabel("Statut:", this), 3, 0);
    networkLayout->addWidget(m_networkStatusLabel, 3, 1, 1, 2);

    // Connexions réseau
    connect(m_startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectToServerClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_userNameEdit, &QLineEdit::textChanged, [this](const QString &text)
            { m_currentUserName = text.isEmpty() ? "Joueur" : text; });

    // Page lobby
    QWidget *lobbyPage = new QWidget(this);
    QVBoxLayout *lobbyLayout = new QVBoxLayout(lobbyPage);
    lobbyLayout->addWidget(m_networkGroup);

    QHBoxLayout *roomsLayout = new QHBoxLayout();
    roomsLayout->addWidget(m_roomListWidget, 2);
    roomsLayout->addWidget(m_userListWidget, 1);
    lobbyLayout->addLayout(roomsLayout);

    // Page de jeu
    QWidget *gamePage = new QWidget(this);
    QHBoxLayout *gameLayout = new QHBoxLayout(gamePage);

    // Panneau de gauche (contrôles)
    QWidget *controlPanel = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlPanel->setMaximumWidth(250);

    // Contrôles de lecture
    QGroupBox *playGroup = new QGroupBox("Lecture", this);
    QVBoxLayout *playLayout = new QVBoxLayout(playGroup);

    m_playPauseBtn = new QPushButton("Play", this);
    m_stopBtn = new QPushButton("Stop", this);

    QHBoxLayout *playBtnLayout = new QHBoxLayout();
    playBtnLayout->addWidget(m_playPauseBtn);
    playBtnLayout->addWidget(m_stopBtn);
    playLayout->addLayout(playBtnLayout);

    // Tempo
    QHBoxLayout *tempoLayout = new QHBoxLayout();
    m_tempoLabel = new QLabel("Tempo:", this);
    m_tempoSpin = new QSpinBox(this);
    m_tempoSpin->setRange(60, 200);
    m_tempoSpin->setValue(120);
    m_tempoSpin->setSuffix(" BPM");
    tempoLayout->addWidget(m_tempoLabel);
    tempoLayout->addWidget(m_tempoSpin);
    playLayout->addLayout(tempoLayout);

    // Volume
    QHBoxLayout *volumeLayout = new QHBoxLayout();
    m_volumeLabel = new QLabel("Volume:", this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(70);
    volumeLayout->addWidget(m_volumeLabel);
    volumeLayout->addWidget(m_volumeSlider);
    playLayout->addLayout(volumeLayout);

    // Contrôles pour les colonnes
    QGroupBox *gridControlGroup = new QGroupBox("Grille", this);
    QVBoxLayout *gridControlLayout = new QVBoxLayout(gridControlGroup);

    // Contrôles de colonnes
    QHBoxLayout *columnLayout = new QHBoxLayout();
    QLabel *columnLabel = new QLabel("Colonnes:", this);
    m_removeColumnBtn = new QPushButton("-", this);
    m_removeColumnBtn->setMaximumWidth(30);
    m_removeColumnBtn->setToolTip("Enlever une colonne");

    m_stepCountLabel = new QLabel("16", this);
    m_stepCountLabel->setAlignment(Qt::AlignCenter);
    m_stepCountLabel->setMinimumWidth(30);
    m_stepCountLabel->setStyleSheet("font-weight: bold;");

    m_addColumnBtn = new QPushButton("+", this);
    m_addColumnBtn->setMaximumWidth(30);
    m_addColumnBtn->setToolTip("Ajouter une colonne");

    columnLayout->addWidget(columnLabel);
    columnLayout->addWidget(m_removeColumnBtn);
    columnLayout->addWidget(m_stepCountLabel);
    columnLayout->addWidget(m_addColumnBtn);
    columnLayout->addStretch();

    gridControlLayout->addLayout(columnLayout);

    // Connexions pour les boutons de colonnes
    connect(m_addColumnBtn, &QPushButton::clicked, this, &MainWindow::onAddColumnClicked);
    connect(m_removeColumnBtn, &QPushButton::clicked, this, &MainWindow::onRemoveColumnClicked);
    connect(m_drumGrid, &DrumGrid::stepCountChanged, this, &MainWindow::onStepCountChanged);

    // Connexions audio
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_tempoSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

    // Assemblage du panneau de contrôle
    controlLayout->addWidget(playGroup);
    controlLayout->addWidget(gridControlGroup);
    controlLayout->addWidget(m_userListWidget);
    controlLayout->addStretch();

    // Assemblage de la page de jeu
    gameLayout->addWidget(controlPanel);
    gameLayout->addWidget(m_drumGrid, 1);

    // Configuration du stacked widget
    m_stackedWidget->addWidget(lobbyPage);
    m_stackedWidget->addWidget(gamePage);

    mainLayout->addWidget(m_stackedWidget);

    // Configuration initiale
    m_roomListWidget->setCurrentUser(m_currentUserId, m_currentUserName);
    m_userListWidget->setCurrentUser(m_currentUserId);

    if (m_networkManager->getClient()) {
        connect(m_networkManager->getClient(), &DrumClient::roomStateReceived,
                this, &MainWindow::onRoomStateReceived);
    }

    // Titre de la fenêtre
    setWindowTitle("DrumBox Multiplayer - Lobby");
    resize(1200, 700);
}

void MainWindow::onRoomStateReceived(const QJsonObject& roomInfo) {
    m_currentRoomId = roomInfo["id"].toString();
    m_userListWidget->setCurrentRoom(m_currentRoomId, roomInfo["name"].toString());
    m_userListWidget->updateUserList(User::listFromJson(roomInfo["users"].toArray()));
    switchToGameMode();
    if (roomInfo.contains("grid")) {
        m_drumGrid->setGridState(roomInfo["grid"].toObject());
    }
    statusBar()->showMessage(QString("Rejoint le salon '%1'").arg(roomInfo["name"].toString()));
}

void MainWindow::onRoomListReceived(const QJsonArray& roomsArray) {
    qDebug() << "[MAINWINDOW] Room list received:" << roomsArray.size();
    m_roomListWidget->updateRoomList(roomsArray);
}

void MainWindow::setupMenus()
{
    QMenuBar *menuBar = this->menuBar();

    // Menu Fichier
    QMenu *fileMenu = menuBar->addMenu("&Fichier");
    fileMenu->addAction("&Nouveau", QKeySequence::New, [this]() { /* TODO */ });
    fileMenu->addAction("&Ouvrir", QKeySequence::Open, [this]() { /* TODO */ });
    fileMenu->addAction("&Sauvegarder", QKeySequence::Save, [this]() { /* TODO */ });
    fileMenu->addSeparator();
    fileMenu->addAction("&Quitter", QKeySequence::Quit, this, &QWidget::close);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Bienvenue dans DrumBox Multiplayer !");
}

void MainWindow::switchToLobbyMode() {
    m_inGameMode = false;
    m_stackedWidget->setCurrentIndex(0);
    setWindowTitle("DrumBox Multiplayer - Lobby");

    // Arrêter la lecture si en cours
    if (m_isPlaying) {
        onStopClicked();
    }

    // Mettre à jour la liste des salons
    QTimer::singleShot(100, this, [this]() {
        onRefreshRoomsRequested();
    });
}


void MainWindow::switchToGameMode()
{
    m_inGameMode = true;
    m_stackedWidget->setCurrentIndex(1);
    setWindowTitle(QString("DrumBox Multiplayer - %1").arg(m_userListWidget->findChild<QLabel *>()->text()));
    updateRoomDisplay();
}

// Suite avec toutes les autres méthodes de votre fichier original...

// Implémentation de toutes les autres méthodes slots...
void MainWindow::onPlayPauseClicked()
{
    m_isPlaying = !m_isPlaying;
    m_drumGrid->setPlaying(m_isPlaying);
    updatePlayButton();

    // Synchronisation réseau
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected())
    {
        QByteArray message = Protocol::createPlayStateMessage(m_isPlaying);
        if (m_networkManager->isServer())
        {
            m_networkManager->broadcastMessage(message);
        }
        else
        {
            m_networkManager->sendMessage(message);
        }
    }
}

void MainWindow::onStopClicked()
{
    m_isPlaying = false;
    m_drumGrid->setPlaying(false);
    m_drumGrid->setCurrentStep(0);
    updatePlayButton();

    // Synchronisation réseau
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected())
    {
        QByteArray message = Protocol::createPlayStateMessage(false);
        if (m_networkManager->isServer())
        {
            m_networkManager->broadcastMessage(message);
        }
        else
        {
            m_networkManager->sendMessage(message);
        }
    }
}

void MainWindow::onTempoChanged(int bpm)
{
    m_drumGrid->setTempo(bpm);

    // Synchronisation réseau
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected())
    {
        QByteArray message = Protocol::createTempoMessage(bpm);
        if (m_networkManager->isServer())
        {
            m_networkManager->broadcastMessage(message);
        }
        else
        {
            m_networkManager->sendMessage(message);
        }
    }
}

void MainWindow::onVolumeChanged(int volume)
{
    float normalizedVolume = volume / 100.0f;
    m_audioEngine->setVolume(normalizedVolume);
}

void MainWindow::onGridCellClicked(int row, int col, bool active)
{
    GridCell cell;
    cell.row = row;
    cell.col = col;
    cell.active = active;
    cell.userId = m_currentUserId;

    QByteArray message = Protocol::createGridUpdateMessage(cell);
    if (m_networkManager->isServer())
        m_networkManager->broadcastMessage(message);
    else
        m_networkManager->sendMessage(message);
}

void MainWindow::onStepTriggered(int step, const QList<int> &activeInstruments)
{
    // L'AudioEngine gère déjà la lecture via la connexion directe
}

// Méthodes réseau
void MainWindow::onStartServerClicked()
{
    if (m_networkManager->isServerRunning())
    {
        m_networkManager->stopServer();
        m_startServerBtn->setText("Héberger");
        switchToLobbyMode();
        return;
    }

    quint16 port = m_portSpin->value();
    if (m_networkManager->startServer(port))
    {
        // AJOUTER ICI :
        if (m_networkManager->getServer()) {
            m_networkManager->getServer()->setRoomManager(m_roomManager);
            qDebug() << "[MAINWINDOW] RoomManager partagé avec le serveur";
        }
        m_startServerBtn->setText("Arrêter Serveur");
        statusBar()->showMessage(QString("Serveur démarré sur le port %1").arg(port));
        onRefreshRoomsRequested();
    }
    else
    {
        QMessageBox::warning(this, "Erreur", "Impossible de démarrer le serveur");
    }
    if (m_networkManager->getServer()) {
        m_networkManager->getServer()->setRoomManager(m_roomManager);
        m_networkManager->getServer()->setHostWindow(this); // AJOUT ICI
        qDebug() << "[MAINWINDOW] RoomManager et host window partagés avec le serveur";
    }
    updateNetworkStatus();
}


void MainWindow::onConnectToServerClicked()
{
    QString host = m_serverAddressEdit->text();
    quint16 port = m_portSpin->value();

    if (m_networkManager->connectToServer(host, port))
    {
        statusBar()->showMessage(QString("Connexion à %1:%2...").arg(host).arg(port));
    }
    else
    {
        QMessageBox::warning(this, "Erreur", "Impossible de se connecter au serveur");
    }
    updateNetworkStatus();
}

void MainWindow::onDisconnectClicked()
{
    // Quitter le salon actuel si nécessaire
    if (!m_currentRoomId.isEmpty())
    {
        onLeaveRoomRequested();
    }

    if (m_networkManager->isServerRunning())
    {
        m_networkManager->stopServer();
        m_startServerBtn->setText("Héberger");
    }
    else if (m_networkManager->isClientConnected())
    {
        m_networkManager->disconnectFromServer();
    }

    switchToLobbyMode();
    updateNetworkStatus();
}

// Gestion des salons
void MainWindow::onCreateRoomRequested(const QString &name, const QString &password, int maxUsers)
{
    if (!m_networkManager->isServerRunning())
    {
        if (!m_networkManager->startServer(m_portSpin->value()))
        {
            QMessageBox::warning(this, "Erreur", "Impossible de démarrer le serveur pour héberger le salon.");
            return;
        }
        // AJOUTER ICI :
        if (m_networkManager->getServer()) {
            m_networkManager->getServer()->setRoomManager(m_roomManager);
            qDebug() << "[MAINWINDOW] RoomManager partagé avec le serveur (auto)";
        }
    }

    // Créer le salon
    QString roomId = m_roomManager->createRoom(name, m_currentUserId, m_currentUserName, password);
    m_roomManager->getRoom(roomId)->setMaxUsers(maxUsers);

    // Rejoindre automatiquement le salon créé
    m_currentRoomId = roomId;
    m_userListWidget->setCurrentRoom(roomId, name);

    // Passer en mode jeu
    switchToGameMode();

    // Mettre à jour l'affichage
    updateRoomDisplay();
    statusBar()->showMessage(QString("Salon '%1' créé avec succès !").arg(name));
}

void MainWindow::onJoinRoomRequested(const QString &roomId, const QString &password)
{
    if (m_networkManager->isServerRunning())
    {
        // Rejoindre un salon local
        if (m_roomManager->joinRoom(roomId, m_currentUserId, m_currentUserName, password))
        {
            m_currentRoomId = roomId;
            Room *room = m_roomManager->getRoom(roomId);
            m_userListWidget->setCurrentRoom(roomId, room->getName());
            switchToGameMode();
            statusBar()->showMessage(QString("Rejoint le salon '%1'").arg(room->getName()));
        }
        else
        {
            QMessageBox::warning(this, "Erreur", "Impossible de rejoindre le salon. Vérifiez le mot de passe.");
        }
    }
    else if (m_networkManager->isClientConnected())
    {
        // Envoyer une demande de join au serveur
        m_networkManager->getClient()->joinRoom(
            roomId,
            m_currentUserId,
            m_currentUserName,
            password
            );
    }
    else
    {
        QMessageBox::warning(this, "Erreur", "Vous devez être connecté à un serveur pour rejoindre un salon.");
    }
}

void MainWindow::onLeaveRoomRequested()
{
    if (m_currentRoomId.isEmpty())
        return;

    if (m_networkManager->isServerRunning())
    {
        // Quitter un salon local (serveur hébergé en local)
        m_roomManager->leaveRoom(m_currentRoomId, m_currentUserId);
    }
    else if (m_networkManager->isClientConnected())
    {
        // Client connecté à un serveur distant : envoie une requête LeaveRoom
        QByteArray message = Protocol::createLeaveRoomMessage(m_currentRoomId);
        m_networkManager->sendMessage(message);
    }
    else
    {
        QMessageBox::warning(this, "Erreur", "Vous n'êtes connecté à aucun serveur.");
        return;
    }

    // Réinitialisation de l'état local
    m_currentRoomId.clear();
    m_userListWidget->setCurrentRoom(QString(), QString());
    switchToLobbyMode(); // retourne à la vue "lobby"
    statusBar()->showMessage("Salon quitté");
}

void MainWindow::onRefreshRoomsRequested() {
    qDebug() << "[MAINWINDOW] Demande d'actualisation des salles";

    if (m_networkManager->isServerRunning()) {
        // Afficher les salons locaux
        QJsonArray roomsArray;
        for (Room* room : m_roomManager->getPublicRooms()) {
            roomsArray.append(room->toJson());
        }
        m_roomListWidget->updateRoomList(roomsArray);
    } else if (m_networkManager->isClientConnected() && m_networkManager->getClient()) {
        // Demander la liste au serveur
        m_networkManager->getClient()->requestRoomList();
    } else {
        qWarning() << "[MAINWINDOW] Aucune connexion active pour actualiser";
        // Vider la liste
        QJsonArray emptyArray;
        m_roomListWidget->updateRoomList(emptyArray);
    }
}


void MainWindow::onKickUserRequested(const QString &userId)
{
    if (!m_networkManager->isServerRunning() || m_currentRoomId.isEmpty())
        return;

    Room *room = m_roomManager->getRoom(m_currentRoomId);
    if (room && room->getHostId() == m_currentUserId)
    {
        if (m_roomManager->leaveRoom(m_currentRoomId, userId))
        {
            statusBar()->showMessage("Utilisateur expulsé");
        }
    }
}

void MainWindow::onTransferHostRequested(const QString &userId)
{
    if (!m_networkManager->isServerRunning() || m_currentRoomId.isEmpty())
        return;

    Room *room = m_roomManager->getRoom(m_currentRoomId);
    if (room && room->getHostId() == m_currentUserId)
    {
        if (room->transferHost(userId))
        {
            statusBar()->showMessage("Hôte transféré");
            updateRoomDisplay();
        }
    }
}

// Événements réseau
void MainWindow::onMessageReceived(const QByteArray &message)
{
    MessageType type;
    QJsonObject data;

    if (Protocol::parseMessage(message, type, data))
    {
        handleNetworkMessage(type, data);
    }
}

void MainWindow::onClientConnected(const QString &clientId)
{
    statusBar()->showMessage(QString("Client connecté: %1").arg(clientId));
}

void MainWindow::onClientDisconnected(const QString &clientId)
{
    statusBar()->showMessage(QString("Client déconnecté: %1").arg(clientId));

    // Marquer l'utilisateur comme hors ligne dans tous les salons
    m_roomManager->setUserOffline(clientId);
    updateRoomDisplay();
}


void MainWindow::onConnectionLost()
{
    statusBar()->showMessage("Connexion perdue");

    // Quitter le salon actuel
    if (!m_currentRoomId.isEmpty())
    {
        m_currentRoomId.clear();
        m_userListWidget->setCurrentRoom(QString(), QString());
        switchToLobbyMode();
    }

    updateNetworkStatus();
}

void MainWindow::onNetworkError(const QString &error)
{
    QMessageBox::warning(this, "Erreur réseau", error);
    updateNetworkStatus();
}

// Méthodes utilitaires
void MainWindow::updatePlayButton()
{
    m_playPauseBtn->setText(m_isPlaying ? "Pause" : "Play");
}

void MainWindow::updateNetworkStatus()
{
    QString status;
    bool connected = false;

    if (m_networkManager->isServerRunning())
    {
        status = QString("Serveur actif (Port %1)").arg(m_portSpin->value());
        connected = true;
        m_connectBtn->setEnabled(false);
        m_serverAddressEdit->setEnabled(false);
    }
    else if (m_networkManager->isClientConnected())
    {
        status = QString("Connecté à %1:%2").arg(m_serverAddressEdit->text()).arg(m_portSpin->value());
        connected = true;
        m_startServerBtn->setEnabled(false);
    }
    else
    {
        status = "Déconnecté";
        m_startServerBtn->setEnabled(true);
        m_connectBtn->setEnabled(true);
        m_serverAddressEdit->setEnabled(true);
        m_startServerBtn->setText("Héberger");
    }

    m_networkStatusLabel->setText(status);
    m_disconnectBtn->setEnabled(connected);
    m_portSpin->setEnabled(!connected);
}

void MainWindow::updateRoomDisplay()
{
    if (m_currentRoomId.isEmpty())
        return;

    if (m_networkManager->isServerRunning())
    {
        Room *room = m_roomManager->getRoom(m_currentRoomId);
        if (room)
        {
            m_userListWidget->updateUserList(room->getUsers());

            // Mettre à jour les couleurs des utilisateurs dans la grille
            for (const User &user : room->getUsers())
            {
                m_drumGrid->setUserColor(user.id, user.color);
            }
        }
    }
}

void MainWindow::syncGridWithNetwork()
{
    if (m_networkManager->isServerRunning())
    {
        // Le serveur diffuse son état
        QJsonObject gridState = m_drumGrid->getGridState();
        QByteArray message = Protocol::createSyncResponseMessage(gridState);
        m_networkManager->broadcastMessage(message);
    }
    else if (m_networkManager->isClientConnected())
    {
        // Le client demande une synchronisation
        QByteArray message = Protocol::createSyncRequestMessage();
        m_networkManager->sendMessage(message);
    }
}

void MainWindow::handleNetworkMessage(MessageType type, const QJsonObject &data)
{
    switch (type)
    {
    case MessageType::CREATE_ROOM:
    {
        if (m_networkManager->isServerRunning())
        {
            QString name = data["name"].toString();
            QString password = data["password"].toString();
            int maxUsers = data["maxUsers"].toInt(4);
            QString userId = data["userId"].toString();
            QString userName = data["userName"].toString();

            QString roomId = m_roomManager->createRoom(name, userId, userName, password);
            Room *room = m_roomManager->getRoom(roomId);
            room->setMaxUsers(maxUsers);

            // Répondre avec les infos du salon créé
            QByteArray response = Protocol::createRoomInfoMessage(room->toJson());
            m_networkManager->sendMessage(response);
        }
        break;
    }

    case MessageType::JOIN_ROOM:
    {
        if (m_networkManager->isServerRunning())
        {
            QString roomId = data["roomId"].toString();
            QString password = data["password"].toString();
            QString userId = data["userId"].toString();
            QString userName = data["userName"].toString();

            if (m_roomManager->joinRoom(roomId, userId, userName, password))
            {
                Room *room = m_roomManager->getRoom(roomId);
                QByteArray response = Protocol::createRoomInfoMessage(room->toJson());
                m_networkManager->sendMessage(response);

                // Notifier les autres utilisateurs
                User user = room->getUser(userId);
                QByteArray userJoinedMsg = Protocol::createUserJoinedMessage(user);
                m_networkManager->broadcastMessage(userJoinedMsg);
            }
            else
            {
                QByteArray errorMsg = Protocol::createErrorMessage("Impossible de rejoindre le salon");
                m_networkManager->sendMessage(errorMsg);
            }
        }
        break;
    }

    case MessageType::LEAVE_ROOM:
    {
        if (m_networkManager->isServerRunning())
        {
            QString roomId = data["roomId"].toString();
            QString userId = data["userId"].toString();

            if (m_roomManager->leaveRoom(roomId, userId))
            {
                QByteArray userLeftMsg = Protocol::createUserLeftMessage(userId);
                m_networkManager->broadcastMessage(userLeftMsg);
            }
        }
        break;
    }

    case MessageType::ROOM_LIST_REQUEST:
    {
        if (m_networkManager->isServerRunning())
        {
            QJsonArray roomsArray;
            for (Room *room : m_roomManager->getPublicRooms())
            {
                roomsArray.append(room->toJson());
            }
            QByteArray response = Protocol::createRoomListResponseMessage(roomsArray);
            m_networkManager->sendMessage(response);
        }
        break;
    }

    case MessageType::ROOM_LIST_RESPONSE:
    {
        // Cette partie est maintenant gérée directement par le signal roomListReceived du DrumClient
        qDebug() << "[MAINWINDOW] ROOM_LIST_RESPONSE reçu via handleNetworkMessage (redondant)";
        break;
    }


    case MessageType::ROOM_INFO:
    {
        // Réponse après avoir rejoint un salon
        Room *room = Room::fromJson(data, this);
        if (room)
        {
            m_currentRoomId = room->getId();
            m_userListWidget->setCurrentRoom(room->getId(), room->getName());
            m_userListWidget->updateUserList(room->getUsers());
            switchToGameMode();
            room->deleteLater();
        }
        break;
    }

    case MessageType::USER_JOINED:
    {
        User user = User::fromJson(data);
        statusBar()->showMessage(QString("%1 a rejoint le salon").arg(user.name));
        updateRoomDisplay();
        break;
    }

    case MessageType::USER_LEFT:
    {
        QString userId = data["userId"].toString();
        statusBar()->showMessage("Un utilisateur a quitté le salon");
        updateRoomDisplay();
        break;
    }

    case MessageType::GRID_UPDATE:
    {
        GridCell cell = GridCell::fromJson(data);
        m_drumGrid->setCellActive(cell.row, cell.col, cell.active, cell.userId);
        break;
    }

    case MessageType::TEMPO_CHANGE:
    {
        int bpm = data["bpm"].toInt();
        m_tempoSpin->setValue(bpm);
        m_drumGrid->setTempo(bpm);
        break;
    }

    case MessageType::PLAY_STATE:
    {
        bool playing = data["playing"].toBool();
        m_isPlaying = playing;
        m_drumGrid->setPlaying(playing);
        updatePlayButton();
        break;
    }

    case MessageType::SYNC_REQUEST:
    {
        if (m_networkManager->isServer())
        {
            QJsonObject gridState = m_drumGrid->getGridState();
            QByteArray response = Protocol::createSyncResponseMessage(gridState);
            m_networkManager->sendMessage(response);
        }
        break;
    }

    case MessageType::SYNC_RESPONSE:
    {
        m_drumGrid->setGridState(data);
        m_tempoSpin->setValue(data["tempo"].toInt(120));
        m_isPlaying = data["playing"].toBool(false);
        updatePlayButton();
        break;
    }

    case MessageType::ERROR_MESSAGE:
    {
        QString error = data["message"].toString();
        QMessageBox::warning(this, "Erreur du serveur", error);
        break;
    }

    default:
        break;
    }
}

void MainWindow::onAddColumnClicked()
{
    m_drumGrid->addColumn();

    // Synchronisation réseau si connecté
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected())
    {
        // Envoyer l'état complet de la grille avec le nouveau nombre de colonnes
        QJsonObject gridState = m_drumGrid->getGridState();
        QByteArray message = Protocol::createSyncResponseMessage(gridState);

        if (m_networkManager->isServer())
        {
            m_networkManager->broadcastMessage(message);
        }
        else
        {
            m_networkManager->sendMessage(message);
        }
    }
}

void MainWindow::onRemoveColumnClicked()
{
    m_drumGrid->removeColumn();

    // Synchronisation réseau si connecté
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected())
    {
        // Envoyer l'état complet de la grille avec le nouveau nombre de colonnes
        QJsonObject gridState = m_drumGrid->getGridState();
        QByteArray message = Protocol::createSyncResponseMessage(gridState);

        if (m_networkManager->isServer())
        {
            m_networkManager->broadcastMessage(message);
        }
        else
        {
            m_networkManager->sendMessage(message);
        }
    }
}

void MainWindow::onStepCountChanged(int newCount)
{
    // Mettre à jour l'affichage du nombre de colonnes
    m_stepCountLabel->setText(QString::number(newCount));

    // Activer/désactiver les boutons selon les limites
    m_removeColumnBtn->setEnabled(newCount > 8);
    m_addColumnBtn->setEnabled(newCount < 64);

    // Mettre à jour le tooltip avec plus d'informations
    m_stepCountLabel->setToolTip(QString("Nombre de pas: %1 (min: 8, max: 64)").arg(newCount));
}

void MainWindow::createConnectionDialog()
{
    // TODO: Implémenter si nécessaire
}
