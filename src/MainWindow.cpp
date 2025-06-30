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
#include <QInputDialog>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QScreen>
#include <QApplication>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_drumGrid(nullptr)
    , m_audioEngine(nullptr)
    , m_networkManager(nullptr)
    , m_roomManager(nullptr)
    , m_roomListWidget(nullptr)
    , m_userListWidget(nullptr)
    , m_stackedWidget(nullptr)
    , m_isPlaying(false)
    , m_inGameMode(false)
    , m_playPauseBtn(nullptr)  // Initialisation explicite
    , m_stopBtn(nullptr)       // Initialisation explicite
    , m_tempoSpin(nullptr)
    , m_volumeSlider(nullptr)
    , m_addColumnBtn(nullptr)
    , m_removeColumnBtn(nullptr)
    , m_stepCountLabel(nullptr)
    , m_instrumentCountLabel(nullptr)
{
    qDebug() << "=== DEBUT CONSTRUCTION MAINWINDOW ===";

    try {
        // Générer un ID utilisateur unique
        qDebug() << "Génération ID utilisateur...";
        m_currentUserId = QUuid::createUuid().toString();
        m_currentUserName = "User";
        qDebug() << "ID utilisateur généré:" << m_currentUserId;

        // Création des objets un par un avec vérification
        qDebug() << "Création DrumGrid...";
        m_drumGrid = new DrumGrid(this);
        if (!m_drumGrid) {
            qDebug() << "ERREUR: Échec création DrumGrid";
            return;
        }
        qDebug() << "DrumGrid créé avec succès";

        qDebug() << "Création AudioEngine...";
        m_audioEngine = new AudioEngine(this);
        if (!m_audioEngine) {
            qDebug() << "ERREUR: Échec création AudioEngine";
            return;
        }
        qDebug() << "AudioEngine créé avec succès";

        qDebug() << "Création NetworkManager...";
        m_networkManager = new NetworkManager(this);
        if (!m_networkManager) {
            qDebug() << "ERREUR: Échec création NetworkManager";
            return;
        }
        qDebug() << "NetworkManager créé avec succès";

        qDebug() << "Création RoomManager...";
        m_roomManager = new RoomManager(this);
        if (!m_roomManager) {
            qDebug() << "ERREUR: Échec création RoomManager";
            return;
        }
        qDebug() << "RoomManager créé avec succès";

        qDebug() << "Création RoomListWidget...";
        m_roomListWidget = new RoomListWidget(this);
        if (!m_roomListWidget) {
            qDebug() << "ERREUR: Échec création RoomListWidget";
            return;
        }
        qDebug() << "RoomListWidget créé avec succès";

        qDebug() << "Création UserListWidget...";
        m_userListWidget = new UserListWidget(this);
        if (!m_userListWidget) {
            qDebug() << "ERREUR: Échec création UserListWidget";
            return;
        }
        qDebug() << "UserListWidget créé avec succès";

        qDebug() << "Création StackedWidget...";
        m_stackedWidget = new QStackedWidget(this);
        if (!m_stackedWidget) {
            qDebug() << "ERREUR: Échec création StackedWidget";
            return;
        }
        qDebug() << "StackedWidget créé avec succès";

        qDebug() << "Début setupUI()...";
        setupUI();
        qDebug() << "setupUI() terminé avec succès";

        qDebug() << "Début setupMenus()...";
        setupMenus();
        qDebug() << "setupMenus() terminé avec succès";

        qDebug() << "Début setupStatusBar()...";
        setupStatusBar();
        qDebug() << "setupStatusBar() terminé avec succès";

        // Configuration AudioEngine
        qDebug() << "Configuration AudioEngine...";
        m_audioEngine->setMaxInstruments(0); // 0 = illimité
        qDebug() << "AudioEngine configuré";

        // Connexions audio - APRÈS création des boutons
        qDebug() << "Début connexions audio...";
        connect(m_drumGrid, &DrumGrid::stepTriggered, this, [this](int step, const QList<int> &instruments) {
            m_audioEngine->playMultipleInstruments(instruments);
        });
        connect(m_drumGrid, &DrumGrid::cellClicked, this, &MainWindow::onGridCellClicked);
        connect(m_drumGrid, &DrumGrid::stepTriggered, this, &MainWindow::onStepTriggered);
        qDebug() << "Connexions audio terminées";

        // Connexion pour la mise à jour dynamique des instruments
        qDebug() << "Connexion mise à jour instruments...";
        connect(m_audioEngine, &AudioEngine::instrumentCountChanged, this, [this](int newCount) {
            m_drumGrid->setInstrumentCount(newCount);
            QStringList instrumentNames = m_audioEngine->getInstrumentNames();
            m_drumGrid->setInstrumentNames(instrumentNames);
            qDebug() << "Nombre d'instruments mis à jour:" << newCount;
            statusBar()->showMessage(QString("Instruments chargés: %1").arg(newCount), 3000);
        });
        qDebug() << "Connexion instruments terminée";

        // Connexion pour gérer l'avertissement de limite atteinte
        qDebug() << "Connexion limite instruments...";
        connect(m_audioEngine, &AudioEngine::maxInstrumentsReached, this,
                [this](int maxCount, int totalFiles) {
                    QString message = QString("Limite d'instruments atteinte !\n\n"
                                              "Fichiers trouvés: %1\n"
                                              "Instruments chargés: %2\n"
                                              "Fichiers ignorés: %3\n\n"
                                              "Voulez-vous augmenter la limite ou charger tous les fichiers ?")
                                          .arg(totalFiles)
                                          .arg(maxCount)
                                          .arg(totalFiles - maxCount);

                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Limite d'instruments");
                    msgBox.setText(message);
                    msgBox.setIcon(QMessageBox::Question);

                    QPushButton* unlimitedBtn = msgBox.addButton("Charger tous", QMessageBox::ActionRole);
                    QPushButton* limitBtn = msgBox.addButton("Garder la limite", QMessageBox::RejectRole);
                    QPushButton* customBtn = msgBox.addButton("Limite personnalisée", QMessageBox::ActionRole);

                    msgBox.exec();

                    if (msgBox.clickedButton() == unlimitedBtn) {
                        m_audioEngine->setMaxInstruments(0);
                        reloadAudioSamples();
                        statusBar()->showMessage("Tous les instruments ont été chargés", 3000);
                    } else if (msgBox.clickedButton() == customBtn) {
                        bool ok;
                        int newLimit = QInputDialog::getInt(this, "Limite personnalisée",
                                                            "Nombre maximum d'instruments:",
                                                            maxCount, 1, 1000, 1, &ok);
                        if (ok && newLimit > maxCount) {
                            m_audioEngine->setMaxInstruments(newLimit);
                            reloadAudioSamples();
                            statusBar()->showMessage(QString("Limite augmentée à %1 instruments").arg(newLimit), 3000);
                        }
                    }
                });
        qDebug() << "Connexion limite terminée";

        // Connexions réseau
        qDebug() << "Début connexions réseau...";
        connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
        connect(m_networkManager, &NetworkManager::clientConnected, this, &MainWindow::onClientConnected);
        connect(m_networkManager, &NetworkManager::clientDisconnected, this, &MainWindow::onClientDisconnected);
        connect(m_networkManager, &NetworkManager::connectionEstablished, this, &MainWindow::onConnectionEstablished);
        connect(m_networkManager, &NetworkManager::connectionLost, this, &MainWindow::onConnectionLost);
        connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);
        qDebug() << "Connexions réseau terminées";

        // Configuration de la fenêtre principale
        qDebug() << "Configuration fenêtre principale...";
        setWindowTitle("Collaborative Drum Machine");
        setWindowIcon(QIcon(":/icons/logo.png"));
        setMinimumSize(1000, 700);
        resize(1200, 800);
        qDebug() << "Configuration fenêtre terminée";

        // Centrer la fenêtre
        qDebug() << "Centrage fenêtre...";
        centerWindow();
        qDebug() << "Centrage terminé";

        // Configuration de l'interface
        qDebug() << "Début connectSignals()...";
        connectSignals();
        qDebug() << "connectSignals() terminé";

        qDebug() << "Début applyModernStyle()...";
        applyModernStyle();
        qDebug() << "applyModernStyle() terminé";

        // Configuration audio
        qDebug() << "Chargement samples audio...";
        m_audioEngine->loadSamples();
        qDebug() << "Samples audio chargés";

        // Configuration client
        qDebug() << "Configuration client réseau...";
        if (m_networkManager->getClient()) {
            connect(m_networkManager->getClient(), &DrumClient::gridCellUpdated,
                    m_drumGrid, &DrumGrid::applyGridUpdate, Qt::UniqueConnection);
            qDebug() << "Client réseau configuré";
        } else {
            qDebug() << "Pas de client réseau disponible";
        }

        // État initial
        qDebug() << "Basculement vers lobby...";
        switchToLobbyMode();
        qDebug() << "Lobby activé";

        qDebug() << "Mise à jour status réseau...";
        updateNetworkStatus();
        qDebug() << "Status réseau mis à jour";

        qDebug() << "=== CONSTRUCTION MAINWINDOW TERMINEE AVEC SUCCES ===";

    } catch (const std::exception& e) {
        qDebug() << "EXCEPTION dans MainWindow:" << e.what();
        throw;
    } catch (...) {
        qDebug() << "EXCEPTION INCONNUE dans MainWindow";
        throw;
    }
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
        if (m_networkManager->getClient()) {
            connect(m_networkManager->getClient(), &DrumClient::columnCountReceived,
                    m_drumGrid, &DrumGrid::setStepCount, Qt::UniqueConnection);
        }
        QTimer::singleShot(200, this, [this]() {
            if (m_networkManager->getClient()) {
                m_networkManager->getClient()->requestRoomList();
            }
        });
    }
}

void MainWindow::centerWindow() {
    const QRect screenGeometry = QApplication::primaryScreen()->geometry();
    const QRect windowGeometry = geometry();

    const int x = (screenGeometry.width() - windowGeometry.width()) / 2;
    const int y = (screenGeometry.height() - windowGeometry.height()) / 2;

    move(x, y);
}

void MainWindow::onColumnCountChanged(int newCount)
{
    QByteArray message = Protocol::createColumnUpdateMessage(newCount);

    if (m_networkManager->isServer())
        m_networkManager->broadcastMessage(message);
    else if (m_networkManager->isClientConnected())
        m_networkManager->sendMessage(message);
}

void MainWindow::setupUI() {
    qDebug() << "setupUI() - début";

    try {
        // Widget central
        qDebug() << "Création widget central...";
        QWidget* centralWidget = new QWidget(this);
        centralWidget->setObjectName("centralWidget");
        setCentralWidget(centralWidget);
        qDebug() << "Widget central créé";

        // Layout principal
        qDebug() << "Création layout principal...";
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        qDebug() << "Layout principal créé";

        // Header
        qDebug() << "Début createHeaderWidget()...";
        QWidget* headerWidget = createHeaderWidget();
        qDebug() << "createHeaderWidget() terminé";

        qDebug() << "Ajout header au layout...";
        mainLayout->addWidget(headerWidget);
        qDebug() << "Header ajouté";

        // Stacked widget pour les pages
        qDebug() << "Configuration stackedWidget...";
        m_stackedWidget->setObjectName("stackedWidget");
        qDebug() << "StackedWidget configuré";

        // Créer les pages
        qDebug() << "Début createLobbyPage()...";
        QWidget* lobbyPage = createLobbyPage();
        qDebug() << "createLobbyPage() terminé";

        qDebug() << "Début createGamePage()...";
        QWidget* gamePage = createGamePage();
        qDebug() << "createGamePage() terminé";

        qDebug() << "Ajout pages au stackedWidget...";
        m_stackedWidget->addWidget(lobbyPage);
        m_stackedWidget->addWidget(gamePage);
        qDebug() << "Pages ajoutées";

        qDebug() << "Ajout stackedWidget au layout principal...";
        mainLayout->addWidget(m_stackedWidget);
        qDebug() << "StackedWidget ajouté";

        // Configuration initiale
        qDebug() << "Configuration initiale widgets...";
        if (m_roomListWidget) {
            qDebug() << "Configuration RoomListWidget...";
            m_roomListWidget->setCurrentUser(m_currentUserId, m_currentUserName);
            qDebug() << "RoomListWidget configuré";
        }
        if (m_userListWidget) {
            qDebug() << "Configuration UserListWidget...";
            m_userListWidget->setCurrentUser(m_currentUserId);
            qDebug() << "UserListWidget configuré";
        }

        qDebug() << "setupUI() - fin avec succès";

    } catch (const std::exception& e) {
        qDebug() << "EXCEPTION dans setupUI():" << e.what();
        throw;
    } catch (...) {
        qDebug() << "EXCEPTION INCONNUE dans setupUI()";
        throw;
    }
}



QWidget* MainWindow::createHeaderWidget() {
    qDebug() << "createHeaderWidget() - début";

    try {
        qDebug() << "Création widget header...";
        QWidget* header = new QWidget(this);
        if (!header) {
            qDebug() << "ERREUR: Échec création header widget";
            return nullptr;
        }
        qDebug() << "Header widget créé";

        qDebug() << "Configuration ObjectName...";
        header->setObjectName("headerWidget");
        qDebug() << "ObjectName configuré";

        qDebug() << "Configuration taille fixe...";
        header->setFixedHeight(80);
        qDebug() << "Taille fixe configurée";

        qDebug() << "Création layout header...";
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
        if (!headerLayout) {
            qDebug() << "ERREUR: Échec création headerLayout";
            return nullptr;
        }
        qDebug() << "HeaderLayout créé";

        qDebug() << "Configuration marges layout...";
        headerLayout->setContentsMargins(20, 10, 20, 10);
        qDebug() << "Marges configurées";

        // Logo
        qDebug() << "Création logoLabel...";
        QLabel* logoLabel = new QLabel(this);
        if (!logoLabel) {
            qDebug() << "ERREUR: Échec création logoLabel";
            return nullptr;
        }
        qDebug() << "LogoLabel créé";

        qDebug() << "Configuration logoLabel...";
        logoLabel->setObjectName("logoLabel");
        logoLabel->setFixedSize(60, 60);
        logoLabel->setScaledContents(true);
        qDebug() << "LogoLabel configuré";

        // Titre
        qDebug() << "Création titleLabel...";
        QLabel* titleLabel = new QLabel("BeeBee", this);
        if (!titleLabel) {
            qDebug() << "ERREUR: Échec création titleLabel";
            return nullptr;
        }
        qDebug() << "TitleLabel créé";

        qDebug() << "Configuration titleLabel...";
        titleLabel->setObjectName("titleLabel");
        qDebug() << "TitleLabel configuré";

        // Status réseau
        qDebug() << "Création networkStatusLabel...";
        m_networkStatusLabel = new QLabel("Déconnecté", this);
        if (!m_networkStatusLabel) {
            qDebug() << "ERREUR: Échec création networkStatusLabel";
            return nullptr;
        }
        qDebug() << "NetworkStatusLabel créé";

        qDebug() << "Configuration networkStatusLabel...";
        m_networkStatusLabel->setObjectName("networkStatusLabel");
        m_networkStatusLabel->setAlignment(Qt::AlignRight);
        qDebug() << "NetworkStatusLabel configuré";

        qDebug() << "Ajout widgets au layout...";
        headerLayout->addWidget(logoLabel);
        qDebug() << "LogoLabel ajouté";

        headerLayout->addWidget(titleLabel);
        qDebug() << "TitleLabel ajouté";

        headerLayout->addStretch();
        qDebug() << "Stretch ajouté";

        headerLayout->addWidget(m_networkStatusLabel);
        qDebug() << "NetworkStatusLabel ajouté";

        qDebug() << "createHeaderWidget() - fin avec succès";
        return header;

    } catch (const std::exception& e) {
        qDebug() << "EXCEPTION dans createHeaderWidget():" << e.what();
        return nullptr;
    } catch (...) {
        qDebug() << "EXCEPTION INCONNUE dans createHeaderWidget()";
        return nullptr;
    }
}


QWidget* MainWindow::createLobbyPage() {
    QWidget* lobbyPage = new QWidget(this);
    lobbyPage->setObjectName("lobbyPage");

    QVBoxLayout* lobbyLayout = new QVBoxLayout(lobbyPage);
    lobbyLayout->setContentsMargins(20, 20, 20, 20);
    lobbyLayout->setSpacing(20);

    // Carte de connexion
    QWidget* connectionCard = createConnectionCard();
    lobbyLayout->addWidget(connectionCard);

    // Contenu des salons
    QHBoxLayout* roomsLayout = new QHBoxLayout();
    roomsLayout->setSpacing(20);

    // Panneau des salons
    QWidget* roomsPanel = createRoomsPanel();
    roomsLayout->addWidget(roomsPanel, 2);

    // Panneau des utilisateurs
    QWidget* usersPanel = createUsersPanel();
    roomsLayout->addWidget(usersPanel, 1);

    lobbyLayout->addLayout(roomsLayout);

    return lobbyPage;
}

QWidget* MainWindow::createConnectionCard() {
    QWidget* card = new QWidget(this);
    card->setObjectName("connectionCard");
    card->setMaximumHeight(200);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 20, 30, 20);

    QLabel* titleLabel = new QLabel("Connexion Réseau", this);
    titleLabel->setObjectName("cardTitle");

    QGridLayout* formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(15);

    // Champs de formulaire
    m_userNameEdit = createStyledLineEdit("Votre pseudo", "Joueur");
    m_serverAddressEdit = createStyledLineEdit("Adresse du serveur", "localhost");
    m_portSpin = createStyledSpinBox(1024, 65535, 8888);

    // Boutons
    m_startServerBtn = createStyledButton("Héberger une partie", "primary");
    m_connectBtn = createStyledButton("Rejoindre", "secondary");
    m_disconnectBtn = createStyledButton("Déconnecter", "danger");
    m_disconnectBtn->setEnabled(false);

    // Layout du formulaire
    formLayout->addWidget(new QLabel("Pseudo:", this), 0, 0);
    formLayout->addWidget(m_userNameEdit, 0, 1, 1, 3);

    formLayout->addWidget(new QLabel("Serveur:", this), 1, 0);
    formLayout->addWidget(m_serverAddressEdit, 1, 1, 1, 2);
    formLayout->addWidget(m_portSpin, 1, 3);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addWidget(m_startServerBtn);
    buttonLayout->addWidget(m_connectBtn);
    buttonLayout->addWidget(m_disconnectBtn);

    cardLayout->addWidget(titleLabel);
    cardLayout->addLayout(formLayout);
    cardLayout->addLayout(buttonLayout);

    return card;
}

QWidget* MainWindow::createRoomsPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("roomsPanel");

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    m_roomListWidget->setObjectName("roomListWidget");
    layout->addWidget(m_roomListWidget);

    return panel;
}

QWidget* MainWindow::createUsersPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("usersPanel");

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    m_userListWidget->setObjectName("userListWidget");
    layout->addWidget(m_userListWidget);

    return panel;
}

// Ajoutez ces méthodes à la fin de votre MainWindow.cpp

void MainWindow::setupMenus() {
    // Menu Fichier
    QMenu* fileMenu = menuBar()->addMenu("&Fichier");
    fileMenu->addAction("&Nouveau", QKeySequence::New, [this]() { /* TODO */ });
    fileMenu->addAction("&Ouvrir", QKeySequence::Open, [this]() { /* TODO */ });
    fileMenu->addAction("&Sauvegarder", QKeySequence::Save, [this]() { /* TODO */ });
    fileMenu->addSeparator();
    fileMenu->addAction("&Quitter", QKeySequence::Quit, this, &QWidget::close);

    // Menu Audio
    QMenu* audioMenu = menuBar()->addMenu("&Audio");
    audioMenu->addAction("&Recharger les samples", this, &MainWindow::reloadAudioSamples);
}

void MainWindow::setupStatusBar() {
    statusBar()->setObjectName("statusBar");
    statusBar()->showMessage("Bienvenue dans DrumBox Multiplayer !");
}

void MainWindow::onRoomListReceived(const QJsonArray& roomsArray) {
    qDebug() << "[MAINWINDOW] Room list received:" << roomsArray.size();
    if (m_roomListWidget) {
        m_roomListWidget->updateRoomList(roomsArray);
    }
}

void MainWindow::onRoomStateReceived(const QJsonObject& roomInfo) {
    m_currentRoomId = roomInfo["id"].toString();
    if (m_userListWidget) {
        m_userListWidget->setCurrentRoom(m_currentRoomId, roomInfo["name"].toString());
        m_userListWidget->updateUserList(User::listFromJson(roomInfo["users"].toArray()));
    }
    switchToGameMode();
    if (roomInfo.contains("grid") && m_drumGrid) {
        m_drumGrid->setGridState(roomInfo["grid"].toObject());
    }
    statusBar()->showMessage(QString("Rejoint le salon '%1'").arg(roomInfo["name"].toString()));
}

QWidget* MainWindow::createGamePage() {
    QWidget* gamePage = new QWidget(this);
    gamePage->setObjectName("gamePage");

    QHBoxLayout* gameLayout = new QHBoxLayout(gamePage);
    gameLayout->setContentsMargins(20, 20, 20, 20);
    gameLayout->setSpacing(20);

    // Panneau de contrôle
    QWidget* controlPanel = createControlPanel();
    gameLayout->addWidget(controlPanel);

    // Grille de batterie
    QWidget* gridContainer = new QWidget(this);
    gridContainer->setObjectName("gridContainer");
    QVBoxLayout* gridLayout = new QVBoxLayout(gridContainer);
    gridLayout->setContentsMargins(20, 20, 20, 20);

    m_drumGrid->setObjectName("drumGrid");
    gridLayout->addWidget(m_drumGrid);

    gameLayout->addWidget(gridContainer, 1);

    return gamePage;
}

QWidget* MainWindow::createControlPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("controlPanel");
    panel->setMaximumWidth(350);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // Contrôles de lecture
    QWidget* playbackCard = createPlaybackCard();
    layout->addWidget(playbackCard);

    // Contrôles pour la grille
    QGroupBox* gridControlGroup = new QGroupBox("Grille", this);
    gridControlGroup->setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            color: #e2e8f0;
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
    )");

    QVBoxLayout* gridControlLayout = new QVBoxLayout(gridControlGroup);
    gridControlLayout->setContentsMargins(15, 15, 15, 15);
    gridControlLayout->setSpacing(15);

    // Contrôles de colonnes
    QHBoxLayout *columnLayout = new QHBoxLayout();
    QLabel *columnLabel = new QLabel("Colonnes:", this);
    columnLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");

    m_removeColumnBtn = new QPushButton("-", this);
    m_removeColumnBtn->setObjectName("columnControlBtn");
    m_removeColumnBtn->setMaximumWidth(40);
    m_removeColumnBtn->setMaximumHeight(30);
    m_removeColumnBtn->setToolTip("Enlever une colonne");

    m_stepCountLabel = new QLabel("16", this);
    m_stepCountLabel->setAlignment(Qt::AlignCenter);
    m_stepCountLabel->setMinimumWidth(40);
    m_stepCountLabel->setStyleSheet(R"(
        font-weight: bold;
        color: #3b82f6;
        background: rgba(59, 130, 246, 0.1);
        border-radius: 5px;
        padding: 5px;
    )");

    m_addColumnBtn = new QPushButton("+", this);
    m_addColumnBtn->setObjectName("columnControlBtn");
    m_addColumnBtn->setMaximumWidth(40);
    m_addColumnBtn->setMaximumHeight(30);
    m_addColumnBtn->setToolTip("Ajouter une colonne");

    columnLayout->addWidget(columnLabel);
    columnLayout->addWidget(m_removeColumnBtn);
    columnLayout->addWidget(m_stepCountLabel);
    columnLayout->addWidget(m_addColumnBtn);
    columnLayout->addStretch();

    gridControlLayout->addLayout(columnLayout);

    // Affichage du nombre d'instruments
    QHBoxLayout* instrumentLayout = new QHBoxLayout();
    QLabel* instrumentLabel = new QLabel("Instruments:", this);
    instrumentLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");

    m_instrumentCountLabel = new QLabel("8", this);
    m_instrumentCountLabel->setAlignment(Qt::AlignCenter);
    m_instrumentCountLabel->setMinimumWidth(40);
    m_instrumentCountLabel->setStyleSheet(R"(
        font-weight: bold;
        color: #10b981;
        background: rgba(16, 185, 129, 0.1);
        border-radius: 5px;
        padding: 5px;
    )");

    instrumentLayout->addWidget(instrumentLabel);
    instrumentLayout->addWidget(m_instrumentCountLabel);
    instrumentLayout->addStretch();

    gridControlLayout->addLayout(instrumentLayout);

    layout->addWidget(gridControlGroup);

    // Liste des utilisateurs en jeu
    layout->addWidget(m_userListWidget);

    layout->addStretch();

    return panel;
}

QWidget* MainWindow::createPlaybackCard() {
    QWidget* card = new QWidget(this);
    card->setObjectName("playbackCard");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel* titleLabel = new QLabel("Contrôles", this);
    titleLabel->setObjectName("cardTitle");
    titleLabel->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(titleLabel);

    // Boutons de lecture
    QHBoxLayout* playbackLayout = new QHBoxLayout();
    m_playPauseBtn = createStyledButton("▶ Play", "primary");
    m_stopBtn = createStyledButton("⏹ Stop", "secondary");
    playbackLayout->addWidget(m_playPauseBtn);
    playbackLayout->addWidget(m_stopBtn);
    layout->addLayout(playbackLayout);

    // Tempo
    QHBoxLayout* tempoLayout = new QHBoxLayout();
    QLabel* tempoLabel = new QLabel("Tempo:", this);
    tempoLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");
    m_tempoSpin = createStyledSpinBox(60, 200, 120);
    m_tempoSpin->setSuffix(" BPM");
    tempoLayout->addWidget(tempoLabel);
    tempoLayout->addWidget(m_tempoSpin);
    layout->addLayout(tempoLayout);

    // Volume
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    QLabel* volumeLabel = new QLabel("Volume:", this);
    volumeLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setObjectName("volumeSlider");
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(70);
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(m_volumeSlider);
    layout->addLayout(volumeLayout);

    return card;
}

QLineEdit* MainWindow::createStyledLineEdit(const QString& placeholder, const QString& text) {
    QLineEdit* edit = new QLineEdit(text, this);
    edit->setPlaceholderText(placeholder);
    edit->setObjectName("styledLineEdit");
    return edit;
}

QSpinBox* MainWindow::createStyledSpinBox(int min, int max, int value) {
    QSpinBox* spin = new QSpinBox(this);
    spin->setObjectName("styledSpinBox");
    spin->setRange(min, max);
    spin->setValue(value);
    return spin;
}

QPushButton* MainWindow::createStyledButton(const QString& text, const QString& style) {
    QPushButton* button = new QPushButton(text, this);
    button->setObjectName("styledButton");
    button->setProperty("buttonStyle", style);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void MainWindow::connectSignals() {
    // Connexions des boutons
    connect(m_startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectToServerClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_userNameEdit, &QLineEdit::textChanged, [this](const QString& text) {
        m_currentUserName = text.isEmpty() ? "Joueur" : text;
    });

    // Connexions pour les boutons de colonnes - VÉRIFIER EXISTENCE
    if (m_addColumnBtn && m_removeColumnBtn) {
        connect(m_addColumnBtn, &QPushButton::clicked, this, &MainWindow::onAddColumnClicked);
        connect(m_removeColumnBtn, &QPushButton::clicked, this, &MainWindow::onRemoveColumnClicked);
    }

    if (m_drumGrid) {
        connect(m_drumGrid, &DrumGrid::stepCountChanged, this, &MainWindow::onStepCountChanged);
        connect(m_drumGrid, &DrumGrid::columnCountChanged, this, &MainWindow::onColumnCountChanged);
    }

    // Connexion pour mettre à jour l'affichage du nombre d'instruments avec limite
    if (m_drumGrid && m_instrumentCountLabel) {
        connect(m_drumGrid, &DrumGrid::instrumentCountChanged, this, [this](int newCount) {
            m_instrumentCountLabel->setText(QString::number(newCount));

            int maxInstruments = m_audioEngine->getMaxInstruments();
            QString limitInfo = (maxInstruments == 0) ? "illimitée" : QString("max: %1").arg(maxInstruments);
            m_instrumentCountLabel->setToolTip(QString("Instruments: %1 (%2)").arg(newCount).arg(limitInfo));
        });
    }

    // Connexions audio - VÉRIFIER EXISTENCE DES BOUTONS
    if (m_playPauseBtn && m_stopBtn && m_tempoSpin && m_volumeSlider) {
        connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
        connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
        connect(m_tempoSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    }

    // Connexions room list
    if (m_roomListWidget) {
        connect(m_roomListWidget, &RoomListWidget::createRoomRequested, this, &MainWindow::onCreateRoomRequested);
        connect(m_roomListWidget, &RoomListWidget::joinRoomRequested, this, &MainWindow::onJoinRoomRequested);
        connect(m_roomListWidget, &RoomListWidget::refreshRequested, this, &MainWindow::onRefreshRoomsRequested);
    }

    // Connexions user list
    if (m_userListWidget) {
        connect(m_userListWidget, &UserListWidget::leaveRoomRequested, this, &MainWindow::onLeaveRoomRequested);
        connect(m_userListWidget, &UserListWidget::kickUserRequested, this, &MainWindow::onKickUserRequested);
        connect(m_userListWidget, &UserListWidget::transferHostRequested, this, &MainWindow::onTransferHostRequested);
    }

    if (m_networkManager && m_networkManager->getClient()) {
        connect(m_networkManager->getClient(), &DrumClient::roomStateReceived,
                this, &MainWindow::onRoomStateReceived);
    }

    // Titre de la fenêtre
    setWindowTitle("DrumBox Multiplayer - Lobby");
    resize(1200, 700);
}

void MainWindow::applyModernStyle() {
    // Style moderne pour l'application
    QString styleSheet = R"(
        /* Couleurs principales */
        * {
            font-family: 'Segoe UI', Arial, sans-serif;
        }

        /* Arrière-plan principal */
        #centralWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #1a1a2e, stop:1 #16213e);
        }

        /* Header */
        #headerWidget {
            background: rgba(30, 30, 46, 0.9);
            border-bottom: 2px solid #0f3460;
        }

        #titleLabel {
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
            margin-left: 15px;
        }

        #networkStatusLabel {
            color: #94a3b8;
            font-size: 14px;
            padding: 8px 16px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 20px;
        }

        /* Cartes et panneaux */
        #connectionCard, #playbackCard, #roomsPanel, #usersPanel, #gridContainer {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 15px;
        }

        #connectionCard {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(255, 255, 255, 0.08), stop:1 rgba(255, 255, 255, 0.03));
        }

        .cardTitle {
            color: #ffffff;
            font-size: 18px;
            font-weight: bold;
            margin-bottom: 10px;
        }

        /* Champs de formulaire */
        #styledLineEdit, #styledSpinBox {
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            padding: 10px 15px;
            color: #ffffff;
            font-size: 14px;
        }

        #styledLineEdit:focus, #styledSpinBox:focus {
            border-color: #3b82f6;
            background: rgba(255, 255, 255, 0.15);
        }

        /* Boutons */
        #styledButton {
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
            transition: all 0.3s ease;
        }

        #styledButton[buttonStyle="primary"] {
            background: #3b82f6;
            color: white;
        }

        #styledButton[buttonStyle="primary"]:hover {
            background: #2563eb;
            transform: translateY(-2px);
        }

        #styledButton[buttonStyle="secondary"] {
            background: rgba(255, 255, 255, 0.1);
            color: #e2e8f0;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        #styledButton[buttonStyle="secondary"]:hover {
            background: rgba(255, 255, 255, 0.2);
        }

        #styledButton[buttonStyle="danger"] {
            background: #ef4444;
            color: white;
        }

        #styledButton[buttonStyle="danger"]:hover {
            background: #dc2626;
        }

        #styledButton:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        /* Boutons de contrôle des colonnes */
        #columnControlBtn {
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 6px;
            color: #e2e8f0;
            font-weight: bold;
            font-size: 16px;
        }

        #columnControlBtn:hover {
            background: rgba(255, 255, 255, 0.2);
            border-color: #3b82f6;
        }

        #columnControlBtn:pressed {
            background: rgba(59, 130, 246, 0.3);
        }

        /* Slider */
        #volumeSlider::groove:horizontal {
            background: rgba(255, 255, 255, 0.1);
            height: 6px;
            border-radius: 3px;
        }

        #volumeSlider::handle:horizontal {
            background: #3b82f6;
            width: 16px;
            height: 16px;
            margin: -5px 0;
            border-radius: 8px;
        }

        #volumeSlider::handle:horizontal:hover {
            background: #2563eb;
        }

        /* Labels */
        QLabel {
            color: #e2e8f0;
        }

        /* Room List Widget */
        #roomListWidget {
            background: transparent;
            color: #e2e8f0;
        }

        /* User List Widget */
        #userListWidget {
            background: transparent;
            color: #e2e8f0;
        }

        /* Status Bar */
        QStatusBar {
            background: rgba(30, 30, 46, 0.9);
            color: #94a3b8;
            border-top: 1px solid rgba(255, 255, 255, 0.1);
        }

        /* Drum Grid */
        #drumGrid {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
        }
    )";

    setStyleSheet(styleSheet);
}
void MainWindow::cleanupConnections() {
    if (m_networkManager->isServerRunning()) {
        m_networkManager->stopServer();
    } else if (m_networkManager->isClientConnected()) {
        m_networkManager->disconnectFromServer();
    }
}

void MainWindow::switchToLobbyMode() {
    m_inGameMode = false;
    m_stackedWidget->setCurrentIndex(0);

    // Arrêter la lecture si en cours
    if (m_isPlaying) {
        onStopClicked();
    }

    // Mettre à jour la liste des salons
    QTimer::singleShot(100, this, [this]() {
        onRefreshRoomsRequested();
    });

    if (m_networkManager->isServerRunning()) {
        // En tant que serveur, afficher nos propres salons
        QJsonArray roomsArray;
        for (Room* room : m_roomManager->getAllRooms()) {
            roomsArray.append(room->toJson());
        }
        m_roomListWidget->updateRoomList(roomsArray);

    }

    // Animation de transition
    QPropertyAnimation* animation = new QPropertyAnimation(m_stackedWidget, "geometry");
    animation->setDuration(300);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}


void MainWindow::switchToGameMode()
{
    m_inGameMode = true;
    m_stackedWidget->setCurrentIndex(1);
    setWindowTitle(QString("DrumBox Multiplayer - %1").arg(m_userListWidget->findChild<QLabel *>()->text()));
    updateRoomDisplay();

    // Animation de transition
    QPropertyAnimation* animation = new QPropertyAnimation(m_stackedWidget, "geometry");
    animation->setDuration(300);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

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

void MainWindow::reloadAudioSamples() {
    int previousCount = m_audioEngine->getInstrumentCount();

    if (m_audioEngine->loadSamples()) {
        int newCount = m_audioEngine->getInstrumentCount();
        statusBar()->showMessage(QString("Samples rechargés: %1 instruments (était %2)").arg(newCount).arg(previousCount), 3000);

        // Log détaillé
        qDebug() << "Rechargement terminé:";
        qDebug() << "  - Instruments précédents:" << previousCount;
        qDebug() << "  - Nouveaux instruments:" << newCount;
        qDebug() << "  - Limite actuelle:" << (m_audioEngine->getMaxInstruments() == 0 ? "Illimitée" : QString::number(m_audioEngine->getMaxInstruments()));
    } else {
        statusBar()->showMessage("Aucun sample trouvé - Mode silencieux", 3000);
    }
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
        // Partager le RoomManager ET le MainWindow avec le serveur
        if (m_networkManager->getServer()) {
            m_networkManager->getServer()->setRoomManager(m_roomManager);
            m_networkManager->getServer()->setHostWindow(this);
            qDebug() << "[MAINWINDOW] RoomManager et host window partagés avec le serveur";
        }

        m_startServerBtn->setText("Arrêter Serveur");
        statusBar()->showMessage(QString("Serveur démarré sur le port %1").arg(port));
        onRefreshRoomsRequested();
    }
    else
    {
        QMessageBox::warning(this, "Erreur", "Impossible de démarrer le serveur");
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
        // Partage du RoomManager et du MainWindow avec le serveur
        if (m_networkManager->getServer()) {
            m_networkManager->getServer()->setRoomManager(m_roomManager);
            m_networkManager->getServer()->setHostWindow(this);
            qDebug() << "[MAINWINDOW] RoomManager et host window partagés avec le serveur (auto)";
        }
    }

    // Création de la salle via RoomManager (retourne un QString, pas un Room*)
    QString roomId = m_roomManager->createRoom(name, password, QString::number(maxUsers));
    if (roomId.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Impossible de créer la salle.");
        return;
    }

    // Actualiser la liste des salles
    onRefreshRoomsRequested();

    // Rejoindre automatiquement la salle créée
    onJoinRoomRequested(roomId, password);
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

void MainWindow::updatePlayButton() {
    m_playPauseBtn->setText(m_isPlaying ? "⏸ Pause" : "▶ Play");
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

void MainWindow::syncGridWithNetwork() {
    if (m_networkManager->isServerRunning()) {
        // Le serveur diffuse son état complet
        QJsonObject gridState = m_drumGrid->getGridState();
        gridState["instrumentNames"] = QJsonArray::fromStringList(m_audioEngine->getInstrumentNames());
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

    case MessageType::COLUMN_UPDATE:
    {
        int columnCount = data["columnCount"].toInt();
        m_drumGrid->setStepCount(columnCount);
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
            gridState["instrumentNames"] = QJsonArray::fromStringList(m_audioEngine->getInstrumentNames());
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

        // Mettre à jour les instruments si présents
        if (data.contains("instrumentNames")) {
            QJsonArray namesArray = data["instrumentNames"].toArray();
            QStringList instrumentNames;
            for (const auto& nameValue : namesArray) {
                instrumentNames.append(nameValue.toString());
            }
            m_drumGrid->setInstrumentNames(instrumentNames);
        }

        if (data.contains("instrumentCount")) {
            int instrumentCount = data["instrumentCount"].toInt();
            m_drumGrid->setInstrumentCount(instrumentCount);
        }
        break;
    }

    case MessageType::INSTRUMENT_SYNC: {
        QJsonArray namesArray = data["instrumentNames"].toArray();
        QStringList instrumentNames;
        for (const auto& nameValue : namesArray) {
            instrumentNames.append(nameValue.toString());
        }
        m_drumGrid->setInstrumentNames(instrumentNames);

        int instrumentCount = data["instrumentCount"].toInt();
        m_drumGrid->setInstrumentCount(instrumentCount);

        statusBar()->showMessage(QString("Instruments synchronisés: %1").arg(instrumentCount), 2000);
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
}

void MainWindow::onRemoveColumnClicked()
{
    m_drumGrid->removeColumn();

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
