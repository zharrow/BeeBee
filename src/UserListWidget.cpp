// UserListWidget.cpp - Corrections pour l'affichage des utilisateurs

void UserListWidget::updateUserList(const QList<User>& users) {
    qDebug() << "[USERLIST] Mise à jour avec" << users.size() << "utilisateurs";

    m_users = users;
    m_userList->clear();

    for (const User& user : users) {
        QListWidgetItem* item = new QListWidgetItem();

        QString displayText = user.name;

        // Ajouter les indicateurs visuels
        if (user.isHost) {
            displayText += " 👑"; // Couronne pour l'hôte
        }
        if (!user.isOnline) {
            displayText += " (Hors ligne)";
        }

        item->setText(displayText);
        item->setData(Qt::UserRole, user.id);

        // Créer une icône de couleur pour l'utilisateur
        QPixmap colorPixmap(20, 20);
        QColor userColor = user.color.isValid() ? user.color : QColor(100, 150, 255);
        colorPixmap.fill(userColor);

        // Créer un cercle de couleur plus joli
        QPainter painter(&colorPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QBrush(userColor));
        painter.drawEllipse(2, 2, 16, 16);

        item->setIcon(QIcon(colorPixmap));

        // Style selon le statut
        if (!user.isOnline) {
            item->setForeground(QColor(150, 150, 150));
            item->setToolTip(QString("%1 - Hors ligne").arg(user.name));
        } else if (user.id == m_currentUserId) {
            // Utilisateur actuel en surbrillance
            item->setForeground(QColor(100, 200, 255));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setToolTip("C'est vous !");
        } else {
            item->setForeground(QColor(226, 232, 240));
            item->setToolTip(QString("%1 - En ligne").arg(user.name));
        }

        // Style spécial pour l'hôte
        if (user.isHost) {
            QString existingTooltip = item->toolTip();
            item->setToolTip(existingTooltip + " (Hôte du salon)");

            // Effet doré pour l'hôte
            QColor hostColor = item->foreground().color();
            hostColor.setRgb(255, 215, 0); // Or
            item->setForeground(hostColor);
        }

        m_userList->addItem(item);

        qDebug() << "[USERLIST] Ajouté:" << user.name
                 << "Host:" << user.isHost
                 << "Online:" << user.isOnline
                 << "Color:" << userColor.name();
    }

    // Forcer la mise à jour de l'affichage
    m_userList->update();
    updateRoomInfo();

    qDebug() << "[USERLIST] Liste mise à jour - total items:" << m_userList->count();
}

void UserListWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Info de la room avec style moderne
    m_roomNameLabel = new QLabel("Aucun salon", this);
    m_roomNameLabel->setAlignment(Qt::AlignCenter);
    m_roomNameLabel->setStyleSheet(R"(
        QLabel {
            font-weight: bold;
            font-size: 18px;
            color: #ffffff;
            padding: 12px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(59, 130, 246, 0.3), stop:1 rgba(139, 92, 246, 0.3));
            border-radius: 10px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }
    )");

    m_userCountLabel = new QLabel("0/4 utilisateurs", this);
    m_userCountLabel->setAlignment(Qt::AlignCenter);
    m_userCountLabel->setStyleSheet(R"(
        QLabel {
            color: #94a3b8;
            font-size: 14px;
            font-weight: bold;
            padding: 8px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 6px;
            border: 1px solid rgba(255, 255, 255, 0.05);
        }
    )");

    // Section titre pour les utilisateurs
    QLabel* usersLabel = new QLabel("👥 Utilisateurs connectés", this);
    usersLabel->setStyleSheet(R"(
        QLabel {
            font-weight: bold;
            color: #e2e8f0;
            font-size: 16px;
            margin-top: 15px;
            margin-bottom: 5px;
        }
    )");

    // Liste des utilisateurs avec style moderne amélioré
    m_userList = new QListWidget(this);
    m_userList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_userList->setStyleSheet(R"(
        QListWidget {
            background: rgba(255, 255, 255, 0.03);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            padding: 8px;
            color: #e2e8f0;
            outline: none;
        }

        QListWidget::item {
            background: rgba(255, 255, 255, 0.02);
            padding: 12px 8px;
            margin: 3px 0px;
            border-radius: 8px;
            border: 1px solid transparent;
            font-size: 14px;
            font-weight: 500;
        }

        QListWidget::item:hover {
            background: rgba(255, 255, 255, 0.08);
            border: 1px solid rgba(59, 130, 246, 0.3);
            transform: translateX(2px);
        }

        QListWidget::item:selected {
            background: rgba(59, 130, 246, 0.2);
            border: 1px solid rgba(59, 130, 246, 0.5);
        }
    )");

    connect(m_userList, &QListWidget::customContextMenuRequested,
            this, &UserListWidget::onUserContextMenu);

    // Bouton pour quitter avec style moderne amélioré
    m_leaveRoomBtn = new QPushButton("🚪 Quitter le Salon", this);
    m_leaveRoomBtn->setEnabled(false);
    m_leaveRoomBtn->setCursor(Qt::PointingHandCursor);
    m_leaveRoomBtn->setMinimumHeight(45);
    m_leaveRoomBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #ef4444, stop:1 #dc2626);
            color: white;
            font-weight: bold;
            padding: 12px;
            border: none;
            border-radius: 10px;
            font-size: 14px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #dc2626, stop:1 #b91c1c);
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(239, 68, 68, 0.4);
        }
        QPushButton:pressed {
            transform: translateY(0px);
        }
        QPushButton:disabled {
            background: #4b5563;
            color: #9ca3af;
            transform: none;
        }
    )");

    connect(m_leaveRoomBtn, &QPushButton::clicked, this, &UserListWidget::onLeaveRoomClicked);

    // Assemblage du layout
    layout->addWidget(m_roomNameLabel);
    layout->addWidget(m_userCountLabel);
    layout->addWidget(usersLabel);
    layout->addWidget(m_userList, 1); // Prend tout l'espace disponible
    layout->addWidget(m_leaveRoomBtn);
}

void UserListWidget::updateRoomInfo() {
    if (m_currentRoomId.isEmpty()) {
        m_roomNameLabel->setText("Aucun salon");
        m_userCountLabel->setText("0/4 utilisateurs");
        m_roomNameLabel->setStyleSheet(m_roomNameLabel->styleSheet().replace(
            "rgba(59, 130, 246, 0.3)", "rgba(107, 114, 128, 0.3)"));
    } else {
        m_roomNameLabel->setText(QString("🎵 %1").arg(m_currentRoomName));

        int onlineUsers = 0;
        for (const User& user : m_users) {
            if (user.isOnline) onlineUsers++;
        }

        QString userInfo;
        if (onlineUsers != m_users.size()) {
            userInfo = QString("%1/%2 utilisateurs (%3 en ligne)")
            .arg(m_users.size())
                .arg(4) // TODO: Récupérer le vrai maxUsers depuis le serveur
                .arg(onlineUsers);
        } else {
            userInfo = QString("%1/%2 utilisateurs connectés")
                           .arg(m_users.size())
                           .arg(4);
        }

        m_userCountLabel->setText(userInfo);

        // Couleur verte quand on est dans un salon
        m_roomNameLabel->setStyleSheet(m_roomNameLabel->styleSheet().replace(
            "rgba(107, 114, 128, 0.3)", "rgba(59, 130, 246, 0.3)"));
    }
}

void UserListWidget::setCurrentRoom(const QString& roomId, const QString& roomName) {
    qDebug() << "[USERLIST] setCurrentRoom:" << roomId << roomName;

    m_currentRoomId = roomId;
    m_currentRoomName = roomName;
    m_leaveRoomBtn->setEnabled(!roomId.isEmpty());

    updateRoomInfo();

    // Si on quitte une room, vider la liste des utilisateurs
    if (roomId.isEmpty()) {
        m_users.clear();
        m_userList->clear();
    }
}
