#include "MainWindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QAction>
#include <QInputDialog>
#include <QTimer>
#include <QMenu>
#include "RoomManager.h"
#include "RoomListWidget.h"
#include "UserListWidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_drumGrid(new DrumGrid(this))
    , m_audioEngine(new AudioEngine(this))
    , m_networkManager(new NetworkManager(this))
    , m_roomManager(new RoomManager(this))
    , m_roomListWidget(new RoomListWidget(this))
    , m_userListWidget(new UserListWidget(this))
    , m_stackedWidget(new QStackedWidget(this))
    , m_isPlaying(false)
    , m_currentUserId(QString("USER_%1").arg(QRandomGenerator::global()->generate() % 10000))
    , m_currentUserName("Joueur")
    , m_inGameMode(false)
    , m_playPauseBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_tempoSpin(nullptr)
    , m_volumeSlider(nullptr)
    , m_startServerBtn(nullptr)
    , m_connectBtn(nullptr)
    , m_disconnectBtn(nullptr)
    , m_serverAddressEdit(nullptr)
    , m_portSpin(nullptr)
    , m_userNameEdit(nullptr)
    , m_networkStatusLabel(nullptr)
    , m_addColumnBtn(nullptr)
    , m_removeColumnBtn(nullptr)
    , m_stepCountLabel(nullptr)
    , m_instrumentCountLabel(nullptr)
{
    qDebug() << "MainWindow - Initialisation";

    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    connectSignals();
    applyModernStyle();

    // Charger les échantillons audio
    if (!m_audioEngine->loadSamples()) {
        statusBar()->showMessage("Mode silencieux - Aucun fichier audio trouvé", 5000);
    }

    // Configuration initiale de la grille avec les instruments chargés
    m_drumGrid->setInstrumentNames(m_audioEngine->getInstrumentNames());

    centerWindow();
}

MainWindow::~MainWindow() {
    cleanupConnections();
}

void MainWindow::centerWindow() {
    if (screen()) {
        QRect screenGeometry = screen()->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

void MainWindow::cleanupConnections() {
    if (m_networkManager) {
        if (m_networkManager->isServerRunning()) {
            m_networkManager->stopServer();
        }
        if (m_networkManager->isClientConnected()) {
            m_networkManager->disconnectFromServer();
        }
    }
}

void MainWindow::setupUI() {
    qDebug() << "setupUI() - début";

    try {
        // Widget central
        QWidget* centralWidget = new QWidget(this);
        centralWidget->setObjectName("centralWidget");
        setCentralWidget(centralWidget);

        // Layout principal
        QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Header
        QWidget* headerWidget = createHeaderWidget();
        mainLayout->addWidget(headerWidget);

        // Stacked widget pour les pages
        m_stackedWidget->setObjectName("stackedWidget");

        // Créer les pages dans le bon ordre
        QWidget* lobbyPage = createLobbyPage();
        QWidget* gamePage = createGamePage();

        m_stackedWidget->addWidget(lobbyPage);    // Index 0
        m_stackedWidget->addWidget(gamePage);     // Index 1

        mainLayout->addWidget(m_stackedWidget);

        // Configuration initiale des widgets
        if (m_roomListWidget) {
            m_roomListWidget->setCurrentUser(m_currentUserId, m_currentUserName);
        }
        if (m_userListWidget) {
            m_userListWidget->setCurrentUser(m_currentUserId);
        }

        qDebug() << "setupUI() - terminé avec succès";

    } catch (const std::exception& e) {
        qDebug() << "EXCEPTION dans setupUI():" << e.what();
        throw;
    }
}

void MainWindow::setupMenus() {
    // Menu Fichier
    QMenu* fileMenu = menuBar()->addMenu("&Fichier");

    QAction* reloadSamplesAction = fileMenu->addAction("🔄 Recharger les samples");
    connect(reloadSamplesAction, &QAction::triggered, this, &MainWindow::reloadAudioSamples);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction("&Quitter");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    // Menu Aide
    QMenu* helpMenu = menuBar()->addMenu("&Aide");

    QAction* aboutAction = helpMenu->addAction("À propos...");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "À propos de DrumBox Multiplayer",
                           "<h3>DrumBox Multiplayer v2.0</h3>"
                           "<p>Boîte à rythmes collaborative en réseau</p>"
                           "<p>Développé avec Qt 6 et C++</p>");
    });
}

void MainWindow::setupMenus() {
    // Menu Fichier
    QMenu* fileMenu = menuBar()->addMenu("&Fichier");

    QAction* syncAction = fileMenu->addAction("🔄 Synchroniser");
    connect(syncAction, &QAction::triggered, this, &MainWindow::syncGridWithNetwork);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction("&Quitter");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    // Menu Aide
    QMenu* helpMenu = menuBar()->addMenu("&Aide");

    QAction* aboutAction = helpMenu->addAction("À propos...");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "À propos de DrumBox Multiplayer",
                           "<h3>DrumBox Multiplayer v2.0</h3>"
                           "<p>Boîte à rythmes collaborative en réseau</p>"
                           "<p>Développé avec Qt 6 et C++</p>");
    });
}

void MainWindow::setupToolbar() {
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);

    toolbar->addAction("🏠 Lobby", this, &MainWindow::switchToLobbyMode);
    toolbar->addSeparator();
    toolbar->addAction("🔊 Recharger Audio", this, &MainWindow::reloadAudioSamples);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Prêt - Mode Lobby");
    statusBar()->setStyleSheet(R"(
        QStatusBar {
            background: rgba(0, 0, 0, 0.5);
            color: #e2e8f0;
            border-top: 1px solid rgba(255, 255, 255, 0.1);
            font-size: 13px;
            padding: 5px;
        }
    )");
}

void MainWindow::connectSignals() {
    // Connexions drum grid
    connect(m_drumGrid, &DrumGrid::cellClicked, this, &MainWindow::onGridCellClicked);
    connect(m_drumGrid, &DrumGrid::stepTriggered, this, &MainWindow::onStepTriggered);
    connect(m_drumGrid, &DrumGrid::stepCountChanged, this, &MainWindow::onStepCountChanged);

    // Connexion pour le nombre d'instruments
    connect(m_drumGrid, &DrumGrid::instrumentCountChanged, this, [this](int newCount) {
        if (m_instrumentCountLabel) {
            m_instrumentCountLabel->setText(QString::number(newCount));
        }
    });

    // Connexions réseau
    connect(m_networkManager, &NetworkManager::connectionEstablished, this, &MainWindow::onConnectionEstablished);
    connect(m_networkManager, &NetworkManager::connectionLost, this, &MainWindow::onConnectionLost);
    connect(m_networkManager, &NetworkManager::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_networkManager, &NetworkManager::errorOccurred, this, &MainWindow::onNetworkError);
    connect(m_networkManager, &NetworkManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(m_networkManager, &NetworkManager::clientDisconnected, this, &MainWindow::onClientDisconnected);

    // Configuration initiale
    setWindowTitle("DrumBox Multiplayer - Lobby");
    resize(1200, 700);
}

void MainWindow::applyModernStyle() {
    setStyleSheet(R"(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0f172a, stop:1 #1e293b);
        }

        QWidget#centralWidget {
            background: transparent;
        }

        QWidget#headerWidget {
            background: rgba(0, 0, 0, 0.3);
            border-bottom: 2px solid rgba(255, 255, 255, 0.1);
        }

        QGroupBox {
            font-weight: bold;
            color: #e2e8f0;
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 10px;
            background: rgba(255, 255, 255, 0.02);
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 10px 0 10px;
        }

        QLabel {
            color: #e2e8f0;
        }

        QPushButton {
            background: #3b82f6;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 14px;
        }

        QPushButton:hover {
            background: #2563eb;
        }

        QPushButton:pressed {
            background: #1d4ed8;
        }

        QPushButton:disabled {
            background: #4b5563;
            color: #9ca3af;
        }

        QLineEdit, QSpinBox {
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            padding: 8px;
            color: white;
            font-size: 14px;
        }

        QLineEdit:focus, QSpinBox:focus {
            border-color: #3b82f6;
            background: rgba(255, 255, 255, 0.15);
        }

        QSlider {
            height: 30px;
        }

        QSlider::groove:horizontal {
            background: rgba(255, 255, 255, 0.1);
            height: 8px;
            border-radius: 4px;
        }

        QSlider::handle:horizontal {
            background: #3b82f6;
            width: 20px;
            height: 20px;
            margin: -6px 0;
            border-radius: 10px;
        }

        QSlider::handle:horizontal:hover {
            background: #2563eb;
        }

        QMenuBar {
            background: rgba(0, 0, 0, 0.5);
            color: #e2e8f0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }

        QMenuBar::item:selected {
            background: rgba(255, 255, 255, 0.1);
        }

        QMenu {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        QMenu::item:selected {
            background: #3b82f6;
        }

        QToolBar {
            background: rgba(0, 0, 0, 0.3);
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            spacing: 10px;
            padding: 5px;
        }
    )");
}

// Création des widgets UI
QWidget* MainWindow::createHeaderWidget() {
    QWidget* header = new QWidget(this);
    header->setObjectName("headerWidget");
    header->setMaximumHeight(80);

    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(20, 10, 20, 10);

    // Logo et titre
    QLabel* logoLabel = new QLabel("🥁", this);
    logoLabel->setStyleSheet("font-size: 36px;");

    QLabel* titleLabel = new QLabel("DrumBox Multiplayer", this);
    titleLabel->setStyleSheet(R"(
        font-size: 28px;
        font-weight: bold;
        color: #ffffff;
        margin-left: 10px;
    )");

    // Info utilisateur
    QWidget* userInfo = new QWidget(this);
    QHBoxLayout* userLayout = new QHBoxLayout(userInfo);
    userLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* userIcon = new QLabel("👤", this);
    userIcon->setStyleSheet("font-size: 20px;");

    QLabel* userLabel = new QLabel(m_currentUserName, this);
    userLabel->setStyleSheet(R"(
        font-size: 16px;
        color: #94a3b8;
        margin-left: 5px;
    )");

    userLayout->addWidget(userIcon);
    userLayout->addWidget(userLabel);

    layout->addWidget(logoLabel);
    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(userInfo);

    return header;
}

QWidget* MainWindow::createLobbyPage() {
    QWidget* lobbyPage = new QWidget(this);
    lobbyPage->setObjectName("lobbyPage");

    QVBoxLayout* mainLayout = new QVBoxLayout(lobbyPage);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Carte de connexion
    QWidget* connectionCard = createConnectionCard();
    mainLayout->addWidget(connectionCard);

    // Conteneur pour rooms et chat
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // Liste des rooms
    QWidget* roomsPanel = createRoomsPanel();
    contentLayout->addWidget(roomsPanel, 2);

    mainLayout->addLayout(contentLayout);

    return lobbyPage;
}

QWidget* MainWindow::createGamePage() {
    QWidget* gamePage = new QWidget(this);
    gamePage->setObjectName("gamePage");

    QHBoxLayout* gameLayout = new QHBoxLayout(gamePage);
    gameLayout->setContentsMargins(20, 20, 20, 20);
    gameLayout->setSpacing(20);

    // Panneau de contrôle (créé en premier pour initialiser les boutons)
    QWidget* controlPanel = createControlPanel();
    gameLayout->addWidget(controlPanel);

    // Grille de batterie avec contrôles
    QWidget* gridContainer = new QWidget(this);
    gridContainer->setObjectName("gridContainer");
    QVBoxLayout* gridLayout = new QVBoxLayout(gridContainer);
    gridLayout->setContentsMargins(20, 20, 20, 20);
    gridLayout->setSpacing(15);

    // Ajouter la grille
    m_drumGrid->setObjectName("drumGrid");
    gridLayout->addWidget(m_drumGrid);

    gameLayout->addWidget(gridContainer, 1);

    return gamePage;
}

QWidget* MainWindow::createConnectionCard() {
    QWidget* card = new QWidget(this);
    card->setObjectName("connectionCard");
    card->setMaximumHeight(180);

    // Style moderne type carte
    card->setStyleSheet(R"(
        #connectionCard {
            background: rgba(255, 255, 255, 0.05);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 15px;
        }
    )");

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 20, 30, 20);
    cardLayout->setSpacing(15);

    // Titre de la carte
    QLabel* titleLabel = new QLabel("🌐 Connexion Réseau", this);
    titleLabel->setStyleSheet(R"(
        font-size: 20px;
        font-weight: bold;
        color: #ffffff;
        margin-bottom: 10px;
    )");

    // Conteneur principal horizontal
    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->setSpacing(30);

    // Section nom d'utilisateur
    QWidget* userSection = new QWidget(this);
    QVBoxLayout* userLayout = new QVBoxLayout(userSection);
    userLayout->setContentsMargins(0, 0, 0, 0);
    userLayout->setSpacing(5);

    QLabel* userLabel = new QLabel("Nom d'utilisateur:", this);
    userLabel->setStyleSheet("color: #94a3b8; font-weight: bold;");

    m_userNameEdit = createStyledLineEdit("Entrez votre nom", m_currentUserName);
    m_userNameEdit->setMaximumWidth(200);

    userLayout->addWidget(userLabel);
    userLayout->addWidget(m_userNameEdit);

    // Section serveur
    QWidget* serverSection = new QWidget(this);
    QVBoxLayout* serverLayout = new QVBoxLayout(serverSection);
    serverLayout->setContentsMargins(0, 0, 0, 0);
    serverLayout->setSpacing(10);

    m_startServerBtn = createStyledButton("🏠 Héberger", "primary");
    m_startServerBtn->setMinimumWidth(150);

    serverLayout->addWidget(m_startServerBtn);

    // Section client
    QWidget* clientSection = new QWidget(this);
    QVBoxLayout* clientLayout = new QVBoxLayout(clientSection);
    clientLayout->setContentsMargins(0, 0, 0, 0);
    clientLayout->setSpacing(10);

    QHBoxLayout* addressLayout = new QHBoxLayout();
    m_serverAddressEdit = createStyledLineEdit("Adresse IP", "localhost");
    m_serverAddressEdit->setMaximumWidth(150);

    m_portSpin = createStyledSpinBox(1024, 65535, 8888);
    m_portSpin->setMaximumWidth(100);

    addressLayout->addWidget(m_serverAddressEdit);
    addressLayout->addWidget(m_portSpin);

    m_connectBtn = createStyledButton("🔗 Rejoindre", "success");
    m_connectBtn->setMinimumWidth(150);

    clientLayout->addLayout(addressLayout);
    clientLayout->addWidget(m_connectBtn);

    // Section déconnexion et statut
    QWidget* statusSection = new QWidget(this);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusSection);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(10);

    m_disconnectBtn = createStyledButton("❌ Déconnecter", "danger");
    m_disconnectBtn->setEnabled(false);
    m_disconnectBtn->setMinimumWidth(150);

    m_networkStatusLabel = new QLabel("🔴 Hors ligne", this);
    m_networkStatusLabel->setAlignment(Qt::AlignCenter);
    m_networkStatusLabel->setStyleSheet(R"(
        background: rgba(239, 68, 68, 0.2);
        color: #fca5a5;
        padding: 8px 16px;
        border-radius: 20px;
        font-weight: bold;
    )");

    statusLayout->addWidget(m_disconnectBtn);
    statusLayout->addWidget(m_networkStatusLabel);

    // Assemblage
    mainLayout->addWidget(userSection);
    mainLayout->addWidget(new QFrame()); // Séparateur
    mainLayout->addWidget(serverSection);
    mainLayout->addWidget(clientSection);
    mainLayout->addWidget(new QFrame()); // Séparateur
    mainLayout->addWidget(statusSection);

    cardLayout->addWidget(titleLabel);
    cardLayout->addLayout(mainLayout);

    // Connexions des boutons après leur création
    connect(m_startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectToServerClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_userNameEdit, &QLineEdit::textChanged, [this](const QString& text) {
        m_currentUserName = text.isEmpty() ? "Joueur" : text;
        if (m_roomListWidget) {
            m_roomListWidget->setCurrentUser(m_currentUserId, m_currentUserName);
        }
    });

    return card;
}

QWidget* MainWindow::createRoomsPanel() {
    m_roomListWidget->setObjectName("roomsPanel");
    m_roomListWidget->setMinimumWidth(600);
    return m_roomListWidget;
}

QWidget* MainWindow::createUsersPanel() {
    m_userListWidget->setObjectName("usersPanel");
    m_userListWidget->setMinimumWidth(250);
    return m_userListWidget;
}

QWidget* MainWindow::createControlPanel() {
    QWidget* panel = new QWidget(this);
    panel->setObjectName("controlPanel");
    panel->setMaximumWidth(350);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // Contrôles de lecture (créer d'abord)
    QWidget* playbackCard = createPlaybackCard();
    layout->addWidget(playbackCard);

    // Contrôles pour la grille
    QGroupBox* gridControlGroup = new QGroupBox("Grille", this);
    gridControlGroup->setStyleSheet(R"(
        QGroupBox {
            font-weight: bold;
            color: #e2e8f0;
            font-size: 16px;
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 10px 0 10px;
        }
    )");

    QVBoxLayout* gridControlLayout = new QVBoxLayout(gridControlGroup);
    gridControlLayout->setSpacing(15);

    // Contrôles de colonnes
    QHBoxLayout* columnLayout = new QHBoxLayout();
    QLabel* columnLabel = new QLabel("Colonnes:", this);
    columnLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");

    m_removeColumnBtn = new QPushButton("-", this);
    m_removeColumnBtn->setMaximumWidth(30);
    m_removeColumnBtn->setToolTip("Enlever une colonne");
    m_removeColumnBtn->setStyleSheet(R"(
        QPushButton {
            background: #ef4444;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: bold;
            font-size: 16px;
        }
        QPushButton:hover { background: #dc2626; }
        QPushButton:disabled { background: #6b7280; }
    )");

    m_stepCountLabel = new QLabel("16", this);
    m_stepCountLabel->setAlignment(Qt::AlignCenter);
    m_stepCountLabel->setMinimumWidth(40);
    m_stepCountLabel->setStyleSheet(R"(
        QLabel {
            font-weight: bold;
            color: #3b82f6;
            font-size: 18px;
            background: rgba(59, 130, 246, 0.1);
            border-radius: 6px;
            padding: 5px;
        }
    )");

    m_addColumnBtn = new QPushButton("+", this);
    m_addColumnBtn->setMaximumWidth(30);
    m_addColumnBtn->setToolTip("Ajouter une colonne");
    m_addColumnBtn->setStyleSheet(R"(
        QPushButton {
            background: #10b981;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: bold;
            font-size: 16px;
        }
        QPushButton:hover { background: #059669; }
        QPushButton:disabled { background: #6b7280; }
    )");

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
        QLabel {
            font-weight: bold;
            color: #10b981;
            font-size: 18px;
            background: rgba(16, 185, 129, 0.1);
            border-radius: 6px;
            padding: 5px;
        }
    )");

    instrumentLayout->addWidget(instrumentLabel);
    instrumentLayout->addWidget(m_instrumentCountLabel);
    instrumentLayout->addStretch();

    gridControlLayout->addLayout(instrumentLayout);
    layout->addWidget(gridControlGroup);

    // Liste des utilisateurs en jeu
    m_userListWidget->setObjectName("userListWidget");
    layout->addWidget(m_userListWidget);

    layout->addStretch();

    // Connexions des boutons de colonnes
    connect(m_addColumnBtn, &QPushButton::clicked, this, &MainWindow::onAddColumnClicked);
    connect(m_removeColumnBtn, &QPushButton::clicked, this, &MainWindow::onRemoveColumnClicked);

    return panel;
}

QWidget* MainWindow::createPlaybackCard() {
    QWidget* card = new QWidget(this);
    card->setObjectName("playbackCard");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QLabel* titleLabel = new QLabel("🎵 Contrôles", this);
    titleLabel->setObjectName("cardTitle");
    titleLabel->setStyleSheet(R"(
        font-size: 18px;
        font-weight: bold;
        color: #ffffff;
        margin-bottom: 10px;
    )");
    layout->addWidget(titleLabel);

    // Boutons de lecture
    QHBoxLayout* playbackLayout = new QHBoxLayout();
    playbackLayout->setSpacing(10);

    m_playPauseBtn = createStyledButton("▶ Play", "primary");
    m_playPauseBtn->setMinimumHeight(45);
    m_playPauseBtn->setStyleSheet(m_playPauseBtn->styleSheet() + "font-size: 16px;");

    m_stopBtn = createStyledButton("⏹ Stop", "secondary");
    m_stopBtn->setMinimumHeight(45);
    m_stopBtn->setStyleSheet(m_stopBtn->styleSheet() + "font-size: 16px;");

    playbackLayout->addWidget(m_playPauseBtn);
    playbackLayout->addWidget(m_stopBtn);
    layout->addLayout(playbackLayout);

    // Tempo
    QHBoxLayout* tempoLayout = new QHBoxLayout();
    QLabel* tempoLabel = new QLabel("🥁 Tempo:", this);
    tempoLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");

    m_tempoSpin = createStyledSpinBox(60, 200, 120);
    m_tempoSpin->setSuffix(" BPM");
    m_tempoSpin->setMinimumHeight(35);

    tempoLayout->addWidget(tempoLabel);
    tempoLayout->addWidget(m_tempoSpin);
    layout->addLayout(tempoLayout);

    // Volume
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    QLabel* volumeLabel = new QLabel("🔊 Volume:", this);
    volumeLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setObjectName("volumeSlider");
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(70);
    m_volumeSlider->setMinimumHeight(25);

    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(m_volumeSlider);
    layout->addLayout(volumeLayout);

    // Connexions des contrôles audio
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_tempoSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

    return card;
}

// Helpers pour créer des widgets stylisés
QLineEdit* MainWindow::createStyledLineEdit(const QString& placeholder, const QString& text) {
    QLineEdit* edit = new QLineEdit(this);
    edit->setPlaceholderText(placeholder);
    edit->setText(text);
    return edit;
}

QSpinBox* MainWindow::createStyledSpinBox(int min, int max, int value) {
    QSpinBox* spin = new QSpinBox(this);
    spin->setRange(min, max);
    spin->setValue(value);
    return spin;
}

QPushButton* MainWindow::createStyledButton(const QString& text, const QString& style) {
    QPushButton* button = new QPushButton(text, this);

    if (style == "primary") {
        button->setStyleSheet(R"(
            QPushButton {
                background: #3b82f6;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover { background: #2563eb; }
            QPushButton:disabled { background: #6b7280; }
        )");
    } else if (style == "success") {
        button->setStyleSheet(R"(
            QPushButton {
                background: #10b981;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover { background: #059669; }
            QPushButton:disabled { background: #6b7280; }
        )");
    } else if (style == "danger") {
        button->setStyleSheet(R"(
            QPushButton {
                background: #ef4444;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover { background: #dc2626; }
            QPushButton:disabled { background: #6b7280; }
        )");
    } else if (style == "secondary") {
        button->setStyleSheet(R"(
            QPushButton {
                background: #6366f1;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover { background: #4f46e5; }
            QPushButton:disabled { background: #6b7280; }
        )");
    }

    button->setCursor(Qt::PointingHandCursor);
    return button;
}

// Slots pour les contrôles audio
void MainWindow::onPlayPauseClicked() {
    m_isPlaying = !m_isPlaying;
    m_drumGrid->setPlaying(m_isPlaying);
    updatePlayButton();

    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected()) {
        QByteArray message = Protocol::createPlayStateMessage(m_isPlaying);
        m_networkManager->sendMessage(message);
    }

    statusBar()->showMessage(m_isPlaying ? "Lecture en cours" : "Lecture arrêtée", 2000);
}

void MainWindow::onStopClicked() {
    m_isPlaying = false;
    m_drumGrid->setPlaying(false);
    m_drumGrid->setCurrentStep(0);
    updatePlayButton();

    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected()) {
        QByteArray message = Protocol::createPlayStateMessage(false);
        m_networkManager->sendMessage(message);
    }

    statusBar()->showMessage("Arrêt", 2000);
}

void MainWindow::onTempoChanged(int bpm) {
    m_drumGrid->setTempo(bpm);

    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected()) {
        QByteArray message = Protocol::createTempoMessage(bpm);
        m_networkManager->sendMessage(message);
    }
}

void MainWindow::onVolumeChanged(int volume) {
    m_audioEngine->setVolume(volume / 100.0f);
}

void MainWindow::onAddColumnClicked() {
    m_drumGrid->addColumn();
}

void MainWindow::onRemoveColumnClicked() {
    m_drumGrid->removeColumn();
}

void MainWindow::onStepCountChanged(int newCount) {
    if (m_stepCountLabel) {
        m_stepCountLabel->setText(QString::number(newCount));
    }

    // Mettre à jour l'état des boutons
    if (m_addColumnBtn && m_removeColumnBtn) {
        m_addColumnBtn->setEnabled(newCount < 64);
        m_removeColumnBtn->setEnabled(newCount > 8);
    }
}

void MainWindow::reloadAudioSamples() {
    statusBar()->showMessage("Rechargement des samples audio...", 1000);

    bool success = m_audioEngine->loadSamples();

    if (success) {
        // Mettre à jour la grille avec les nouveaux instruments
        m_drumGrid->setInstrumentNames(m_audioEngine->getInstrumentNames());

        // Synchroniser avec le réseau si connecté
        if (m_networkManager->isServerRunning()) {
            QByteArray message = Protocol::createInstrumentSyncMessage(m_audioEngine->getInstrumentNames());
            m_networkManager->broadcastMessage(message);
        }

        statusBar()->showMessage("Samples rechargés avec succès", 3000);
    } else {
        statusBar()->showMessage("Aucun sample trouvé - Mode silencieux", 3000);
    }
}

// Slots pour la grille
void MainWindow::onGridCellClicked(int row, int col, bool active) {
    // Ajouter l'ID utilisateur à la mise à jour
    GridCell cell;
    cell.row = row;
    cell.col = col;
    cell.active = active;
    cell.userId = m_currentUserId;

    // Définir la couleur de l'utilisateur dans la grille
    m_drumGrid->setCellActive(row, col, active, m_currentUserId);

    // Envoyer la mise à jour au réseau
    if (m_networkManager->isServerRunning() || m_networkManager->isClientConnected()) {
        QByteArray message = Protocol::createGridUpdateMessage(cell);
        m_networkManager->sendMessage(message);
    }
}

void MainWindow::onStepTriggered(int step, const QList<int>& activeInstruments) {
    m_audioEngine->playMultipleInstruments(activeInstruments);
}

// Slots pour les salons
void MainWindow::onCreateRoomRequested(const QString& name, const QString& password, int maxUsers) {
    if (m_networkManager->isServerRunning()) {
        QString roomId = m_roomManager->createRoom(name, m_currentUserId, m_currentUserName, password);

        // Rejoindre automatiquement la room créée
        m_currentRoomId = roomId;
        m_userListWidget->setCurrentRoom(roomId, name);

        // Basculer vers le mode jeu
        switchToGameMode();

        statusBar()->showMessage(QString("Salon '%1' créé").arg(name), 3000);
    } else {
        // Si client, envoyer la requête au serveur
        QByteArray message = Protocol::createCreateRoomMessage(name, password, maxUsers);
        m_networkManager->sendMessage(message);
    }
}

void MainWindow::onJoinRoomRequested(const QString& roomId, const QString& password) {
    if (m_networkManager->isServerRunning()) {
        bool success = m_roomManager->joinRoom(roomId, m_currentUserId, m_currentUserName, password);
        if (success) {
            m_currentRoomId = roomId;
            Room* room = m_roomManager->getRoom(roomId);
            if (room) {
                m_userListWidget->setCurrentRoom(roomId, room->getName());
                switchToGameMode();
                statusBar()->showMessage(QString("Rejoint le salon '%1'").arg(room->getName()), 3000);
            }
        } else {
            statusBar()->showMessage("Impossible de rejoindre le salon", 3000);
        }
    } else if (m_networkManager->isClientConnected()) {
        // Si client, envoyer la requête au serveur
        m_networkManager->getClient()->joinRoom(roomId, m_currentUserId, m_currentUserName, password);
    }
}

void MainWindow::onLeaveRoomRequested() {
    if (!m_currentRoomId.isEmpty()) {
        if (m_networkManager->isServerRunning()) {
            m_roomManager->leaveRoom(m_currentRoomId, m_currentUserId);
        } else if (m_networkManager->isClientConnected()) {
            QByteArray message = Protocol::createLeaveRoomMessage(m_currentRoomId);
            m_networkManager->sendMessage(message);
        }

        // Réinitialiser l'état local
        m_currentRoomId.clear();
        m_userListWidget->setCurrentRoom(QString(), QString());

        // Retourner au lobby
        switchToLobbyMode();

        statusBar()->showMessage("Vous avez quitté le salon", 3000);
    }
}

void MainWindow::onRefreshRoomsRequested() {
    if (m_networkManager->isClientConnected() && m_networkManager->getClient()) {
        m_networkManager->getClient()->requestRoomList();
        statusBar()->showMessage("Actualisation de la liste des salons...", 2000);
    } else if (m_networkManager->isServerRunning()) {
        // Si on est serveur, rafraîchir directement
        QList<Room*> publicRooms = m_roomManager->getPublicRooms();
        QJsonArray roomArray;
        for (Room* room : publicRooms) {
            if (room) {
                roomArray.append(room->toJson());
            }
        }
        m_roomListWidget->updateRoomList(roomArray);
    }
}

void MainWindow::onKickUserRequested(const QString& userId) {
    if (m_networkManager->isServerRunning() && !m_currentRoomId.isEmpty()) {
        Room* room = m_roomManager->getRoom(m_currentRoomId);
        if (room && room->getHostId() == m_currentUserId) {
            m_roomManager->leaveRoom(m_currentRoomId, userId);
            statusBar()->showMessage("Utilisateur expulsé", 3000);
        }
    }
}

void MainWindow::onTransferHostRequested(const QString& userId) {
    if (m_networkManager->isServerRunning() && !m_currentRoomId.isEmpty()) {
        Room* room = m_roomManager->getRoom(m_currentRoomId);
        if (room && room->getHostId() == m_currentUserId) {
            room->transferHost(userId);
            updateRoomDisplay();
            statusBar()->showMessage("Rôle d'hôte transféré", 3000);
        }
    }
}

// Slots réseau
void MainWindow::onStartServerClicked() {
    startServer();
}

void MainWindow::startServer() {
    if (m_networkManager->startServer(m_portSpin->value())) {
        // Partager le RoomManager avec le serveur
        m_networkManager->getServer()->setRoomManager(m_roomManager);
        m_networkManager->getServer()->setHostWindow(this);

        m_startServerBtn->setEnabled(false);
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);

        statusBar()->showMessage("Serveur démarré sur le port " + QString::number(m_portSpin->value()), 3000);
        updateNetworkStatus();
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de démarrer le serveur");
    }
}

void MainWindow::onConnectToServerClicked() {
    createConnectionDialog();
}

void MainWindow::createConnectionDialog() {
    if (m_networkManager->connectToServer(m_serverAddressEdit->text(), m_portSpin->value())) {
        m_startServerBtn->setEnabled(false);
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);

        statusBar()->showMessage("Connexion en cours...", 2000);
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de se connecter au serveur");
    }
}

void MainWindow::onDisconnectClicked() {
    if (m_networkManager->isServerRunning()) {
        m_networkManager->stopServer();
    } else {
        m_networkManager->disconnectFromServer();
    }

    // Réinitialiser l'état
    m_currentRoomId.clear();
    m_userListWidget->setCurrentRoom(QString(), QString());

    m_startServerBtn->setEnabled(true);
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);

    switchToLobbyMode();
    updateNetworkStatus();
}

void MainWindow::onClientConnected(const QString& clientId) {
    statusBar()->showMessage(QString("Client connecté: %1").arg(clientId), 3000);
}

void MainWindow::onClientDisconnected(const QString& clientId) {
    statusBar()->showMessage(QString("Client déconnecté: %1").arg(clientId), 3000);
}

void MainWindow::onConnectionEstablished() {
    qDebug() << "[MAINWINDOW] Connexion établie";
    updateNetworkStatus();

    if (m_networkManager->isClientConnected() && m_networkManager->getClient()) {
        // Connecter les signaux pour recevoir les données du serveur
        connect(m_networkManager->getClient(), &DrumClient::roomListReceived,
                this, &MainWindow::onRoomListReceived, Qt::UniqueConnection);

        connect(m_networkManager->getClient(), &DrumClient::roomStateReceived,
                this, &MainWindow::onRoomStateReceived, Qt::UniqueConnection);

        connect(m_networkManager->getClient(), &DrumClient::gridCellUpdated,
                m_drumGrid, &DrumGrid::applyGridUpdate, Qt::UniqueConnection);

        // Demander la liste des rooms après un court délai
        QTimer::singleShot(200, this, [this]() {
            if (m_networkManager->getClient()) {
                m_networkManager->getClient()->requestRoomList();
            }
        });
    }
}

void MainWindow::onConnectionLost() {
    updateNetworkStatus();

    // Réinitialiser l'interface
    m_startServerBtn->setEnabled(true);
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);

    if (!m_currentRoomId.isEmpty()) {
        m_currentRoomId.clear();
        m_userListWidget->setCurrentRoom(QString(), QString());
        switchToLobbyMode();
    }
}

void MainWindow::onNetworkError(const QString& error) {
    statusBar()->showMessage(QString("Erreur réseau: %1").arg(error), 5000);
}

void MainWindow::onMessageReceived(const QByteArray& message) {
    MessageType type;
    QJsonObject data;

    if (!Protocol::parseMessage(message, type, data)) {
        qWarning() << "Message invalide reçu";
        return;
    }

    handleNetworkMessage(type, data);
}

void MainWindow::onRoomListReceived(const QJsonArray& roomsArray) {
    qDebug() << "[MAINWINDOW] Liste des salons reçue:" << roomsArray.size() << "salons";
    m_roomListWidget->updateRoomList(roomsArray);
}

void MainWindow::onRoomStateReceived(const QJsonObject& roomInfo) {
    qDebug() << "[MAINWINDOW] Room state reçu:" << roomInfo;

    m_currentRoomId = roomInfo["id"].toString();
    QString roomName = roomInfo["name"].toString();

    // Mettre à jour le widget des utilisateurs avec les données reçues
    if (m_userListWidget) {
        m_userListWidget->setCurrentRoom(m_currentRoomId, roomName);

        // Convertir les utilisateurs depuis le JSON
        QJsonArray usersArray = roomInfo["users"].toArray();
        QList<User> users = User::listFromJson(usersArray);

        qDebug() << "[MAINWINDOW] Utilisateurs dans la room:" << users.size();
        for (const User& user : users) {
            qDebug() << "  - " << user.name << (user.isHost ? "(hôte)" : "");
        }

        m_userListWidget->updateUserList(users);
    }

    // Basculer vers le mode jeu
    switchToGameMode();

    // Charger l'état de la grille si présent
    if (roomInfo.contains("grid")) {
        m_drumGrid->setGridState(roomInfo["grid"].toObject());
    }

    statusBar()->showMessage(QString("Rejoint le salon '%1'").arg(roomName));
}

// Méthodes utilitaires
void MainWindow::updatePlayButton() {
    if (m_playPauseBtn) {
        if (m_isPlaying) {
            m_playPauseBtn->setText("⏸ Pause");
            m_playPauseBtn->setStyleSheet(m_playPauseBtn->styleSheet() +
                                          "background: #f59e0b; QPushButton:hover { background: #d97706; }");
        } else {
            m_playPauseBtn->setText("▶ Play");
            m_playPauseBtn->setStyleSheet(m_playPauseBtn->styleSheet() +
                                          "background: #10b981; QPushButton:hover { background: #059669; }");
        }
    }
}

void MainWindow::updateNetworkStatus() {
    if (m_networkManager->isServerRunning()) {
        m_networkStatusLabel->setText("🟢 Serveur actif");
        m_networkStatusLabel->setStyleSheet(R"(
            background: rgba(16, 185, 129, 0.2);
            color: #86efac;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
        )");
    } else if (m_networkManager->isClientConnected()) {
        m_networkStatusLabel->setText("🟢 Connecté");
        m_networkStatusLabel->setStyleSheet(R"(
            background: rgba(16, 185, 129, 0.2);
            color: #86efac;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
        )");
    } else {
        m_networkStatusLabel->setText("🔴 Hors ligne");
        m_networkStatusLabel->setStyleSheet(R"(
            background: rgba(239, 68, 68, 0.2);
            color: #fca5a5;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
        )");
    }
}

void MainWindow::updateRoomDisplay() {
    if (m_currentRoomId.isEmpty() || !m_userListWidget) {
        return;
    }

    if (m_networkManager->isServerRunning() && m_roomManager) {
        Room* room = m_roomManager->getRoom(m_currentRoomId);
        if (room) {
            QList<User> users = room->getUsers();

            qDebug() << "[MAINWINDOW] Mise à jour affichage room - utilisateurs:" << users.size();

            m_userListWidget->updateUserList(users);

            // Mettre à jour les couleurs des utilisateurs dans la grille
            if (m_drumGrid) {
                for (const User& user : users) {
                    m_drumGrid->setUserColor(user.id, user.color);
                }
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
        statusBar()->showMessage("État synchronisé avec tous les clients", 2000);
    }
    else if (m_networkManager->isClientConnected()) {
        // Le client demande une synchronisation
        QByteArray message = Protocol::createSyncRequestMessage();
        m_networkManager->sendMessage(message);
        statusBar()->showMessage("Demande de synchronisation envoyée", 2000);
    } else {
        statusBar()->showMessage("Aucune connexion réseau active", 3000);
    }
}

void MainWindow::handleNetworkMessage(MessageType type, const QJsonObject& data) {
    switch (type) {
    case MessageType::GRID_UPDATE: {
        GridCell cell = GridCell::fromJson(data);
        m_drumGrid->applyGridUpdate(cell);
        break;
    }

    case MessageType::TEMPO_CHANGE: {
        int bpm = data["bpm"].toInt();
        m_drumGrid->setTempo(bpm);
        if (m_tempoSpin) {
            m_tempoSpin->setValue(bpm);
        }
        break;
    }

    case MessageType::PLAY_STATE: {
        bool playing = data["playing"].toBool();
        m_isPlaying = playing;
        m_drumGrid->setPlaying(playing);
        updatePlayButton();
        break;
    }

    case MessageType::SYNC_REQUEST: {
        if (m_networkManager->isServerRunning()) {
            QJsonObject gridState = m_drumGrid->getGridState();
            gridState["instrumentNames"] = QJsonArray::fromStringList(m_audioEngine->getInstrumentNames());
            QByteArray response = Protocol::createSyncResponseMessage(gridState);
            m_networkManager->broadcastMessage(response);
        }
        break;
    }

    case MessageType::SYNC_RESPONSE: {
        m_drumGrid->setGridState(data);

        // Synchroniser les instruments si présents
        if (data.contains("instrumentNames")) {
            QJsonArray names = data["instrumentNames"].toArray();
            QStringList instrumentNames;
            for (const auto& name : names) {
                instrumentNames.append(name.toString());
            }
            m_drumGrid->setInstrumentNames(instrumentNames);
        }

        statusBar()->showMessage("Synchronisation réussie", 2000);
        break;
    }

    case MessageType::INSTRUMENT_SYNC: {
        QJsonArray names = data["instrumentNames"].toArray();
        QStringList instrumentNames;
        for (const auto& name : names) {
            instrumentNames.append(name.toString());
        }
        m_drumGrid->setInstrumentNames(instrumentNames);
        break;
    }

    case MessageType::USER_INFO: {
        QString userId = data["userId"].toString();
        QString userName = data["userName"].toString();
        QColor userColor = QColor(data["userColor"].toString());

        m_drumGrid->setUserColor(userId, userColor);
        break;
    }

    case MessageType::ERROR_MESSAGE: {
        QString error = data["message"].toString();
        QMessageBox::warning(this, "Erreur", error);
        break;
    }

    default:
        qDebug() << "Type de message non géré:" << Protocol::messageTypeToString(type);
        break;
    }
}

void MainWindow::switchToGameMode() {
    m_stackedWidget->setCurrentIndex(1);
    m_inGameMode = true;
    setWindowTitle(QString("DrumBox Multiplayer - %1").arg(m_currentRoomName));
    statusBar()->showMessage("Mode jeu activé", 2000);
}

void MainWindow::switchToLobbyMode() {
    m_stackedWidget->setCurrentIndex(0);
    m_inGameMode = false;
    setWindowTitle("DrumBox Multiplayer - Lobby");
    statusBar()->showMessage("Mode lobby", 2000);
}
