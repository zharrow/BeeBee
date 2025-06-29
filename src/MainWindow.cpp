// Méthodes corrigées pour MainWindow.cpp

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

    // Contrôles de grille (colonnes/steps)
    QWidget* gridControlWidget = createGridControls();
    gridLayout->addWidget(gridControlWidget);

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

    // Boutons de lecture - CRÉER ICI
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

    return card;
}

QWidget* MainWindow::createGridControls() {
    QWidget* controls = new QWidget(this);
    controls->setMaximumHeight(60);

    QHBoxLayout* layout = new QHBoxLayout(controls);
    layout->setContentsMargins(10, 10, 10, 10);

    // Info sur les steps
    QLabel* infoLabel = new QLabel("Grille de batterie collaborative", this);
    infoLabel->setStyleSheet(R"(
        color: #94a3b8;
        font-style: italic;
        font-size: 14px;
    )");

    layout->addWidget(infoLabel);
    layout->addStretch();

    // Bouton de synchronisation
    QPushButton* syncBtn = createStyledButton("🔄 Sync", "secondary");
    syncBtn->setToolTip("Synchroniser avec les autres joueurs");
    connect(syncBtn, &QPushButton::clicked, this, &MainWindow::syncGridWithNetwork);

    layout->addWidget(syncBtn);

    return controls;
}

void MainWindow::connectSignals() {
    // Connexions des boutons de contrôle réseau
    connect(m_startServerBtn, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectToServerClicked);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_userNameEdit, &QLineEdit::textChanged, [this](const QString& text) {
        m_currentUserName = text.isEmpty() ? "Joueur" : text;
        // Mettre à jour les widgets avec le nouveau nom
        if (m_roomListWidget) {
            m_roomListWidget->setCurrentUser(m_currentUserId, m_currentUserName);
        }
    });

    // Connexions des boutons audio (maintenant que les boutons existent)
    if (m_playPauseBtn && m_stopBtn && m_tempoSpin && m_volumeSlider) {
        connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
        connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
        connect(m_tempoSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    }

    // Connexions pour les boutons de colonnes
    if (m_addColumnBtn && m_removeColumnBtn) {
        connect(m_addColumnBtn, &QPushButton::clicked, this, &MainWindow::onAddColumnClicked);
        connect(m_removeColumnBtn, &QPushButton::clicked, this, &MainWindow::onRemoveColumnClicked);
    }

    if (m_drumGrid) {
        connect(m_drumGrid, &DrumGrid::stepCountChanged, this, &MainWindow::onStepCountChanged);

        // Connexion pour mettre à jour l'affichage du nombre d'instruments
        connect(m_drumGrid, &DrumGrid::instrumentCountChanged, this, [this](int newCount) {
            if (m_instrumentCountLabel) {
                m_instrumentCountLabel->setText(QString::number(newCount));

                int maxInstruments = m_audioEngine->getMaxInstruments();
                QString limitInfo = (maxInstruments == 0) ? "illimitée" : QString("max: %1").arg(maxInstruments);
                m_instrumentCountLabel->setToolTip(QString("Instruments: %1 (%2)").arg(newCount).arg(limitInfo));
            }
        });
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

    // Connexions réseau pour les room states
    if (m_networkManager && m_networkManager->getClient()) {
        connect(m_networkManager->getClient(), &DrumClient::roomStateReceived,
                this, &MainWindow::onRoomStateReceived, Qt::UniqueConnection);
        connect(m_networkManager->getClient(), &DrumClient::roomListReceived,
                this, &MainWindow::onRoomListReceived, Qt::UniqueConnection);
    }

    // Titre de la fenêtre et taille
    setWindowTitle("DrumBox Multiplayer - Lobby");
    resize(1200, 700);
}

// Correction de la méthode onRoomStateReceived pour bien afficher les utilisateurs
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

// Correction pour s'assurer que les widgets sont bien configurés
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

// Méthodes supplémentaires nécessaires

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

// Correction de setupUI pour s'assurer que tout est initialisé correctement
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

// Correction pour les connexions réseau
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
