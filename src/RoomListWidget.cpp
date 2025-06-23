#include "RoomListWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QListWidget>
#include <QInputDialog>

RoomListWidget::RoomListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

QPushButton* RoomListWidget::createModernButton(const QString& text, const QString& color, const QString& hoverColor) {
    QPushButton* button = new QPushButton(text, this);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: %2;
            transform: translateY(-2px);
        }
        QPushButton:pressed {
            transform: translateY(0px);
        }
        QPushButton:disabled {
            background: #4b5563;
            color: #9ca3af;
        }
    )").arg(color, hoverColor));

    return button;
}

void RoomListWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // En-tête avec style moderne
    QLabel* titleLabel = new QLabel("Salons Disponibles", this);
    titleLabel->setObjectName("sectionTitle");
    titleLabel->setStyleSheet(R"(
        #sectionTitle {
            color: #ffffff;
            font-size: 20px;
            font-weight: bold;
            margin-bottom: 10px;
        }
    )");

    // Liste des rooms avec style moderne
    m_roomList = new QListWidget(this);
    m_roomList->setObjectName("modernListWidget");
    m_roomList->setStyleSheet(R"(
        #modernListWidget {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            padding: 10px;
            color: #e2e8f0;
        }

        #modernListWidget::item {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 8px;
            padding: 12px;
            margin: 4px 0;
            color: #e2e8f0;
        }

        #modernListWidget::item:hover {
            background: rgba(255, 255, 255, 0.1);
            transform: translateX(5px);
        }

        #modernListWidget::item:selected {
            background: rgba(59, 130, 246, 0.3);
            border: 1px solid rgba(59, 130, 246, 0.5);
        }
    )");

    connect(m_roomList, &QListWidget::itemDoubleClicked, this, &RoomListWidget::onRoomDoubleClicked);
    connect(m_roomList, &QListWidget::itemSelectionChanged, this, &RoomListWidget::updateJoinButton);

    // Boutons avec style moderne
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_createRoomBtn = createModernButton("➕ Créer un Salon", "#3b82f6", "#2563eb");
    m_joinRoomBtn = createModernButton("🚪 Rejoindre", "#10b981", "#059669");
    m_refreshBtn = createModernButton("🔄 Actualiser", "#6366f1", "#4f46e5");

    m_joinRoomBtn->setEnabled(false);

    buttonLayout->addWidget(m_createRoomBtn);
    buttonLayout->addWidget(m_joinRoomBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_refreshBtn);

    // Layout principal
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(m_roomList, 1);
    mainLayout->addLayout(buttonLayout);

    // Connexions
    connect(m_createRoomBtn, &QPushButton::clicked, this, &RoomListWidget::onCreateRoomClicked);
    connect(m_joinRoomBtn, &QPushButton::clicked, this, &RoomListWidget::onJoinRoomClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &RoomListWidget::onRefreshClicked);
}

void RoomListWidget::updateRoomList(const QList<Room*>& rooms) {
    m_roomList->clear();

    for (Room* room : rooms) {
        QListWidgetItem* item = new QListWidgetItem();

        QString displayText = QString("%1 (%2/%3)")
                                  .arg(room->getName())
                                  .arg(room->getUserCount())
                                  .arg(room->getMaxUsers());

        if (room->hasPassword()) {
            displayText += " 🔒";
        }

        item->setText(displayText);
        item->setData(Qt::UserRole, room->getId());

        // Couleur selon l'état
        if (room->isFull()) {
            item->setForeground(QColor(150, 150, 150));
            item->setToolTip("Salon complet");
        } else {
            item->setForeground(QColor(226, 232, 240)); // Couleur claire pour le thème sombre
            item->setToolTip(QString("Hôte: %1\nCréé: %2")
                                 .arg(room->getUser(room->getHostId()).name)
                                 .arg(room->getCreatedTime().toString("hh:mm:ss")));
        }

        m_roomList->addItem(item);
    }

    updateJoinButton();
}

void RoomListWidget::setCurrentUser(const QString& userId, const QString& userName) {
    m_currentUserId = userId;
    m_currentUserName = userName;
}

void RoomListWidget::onCreateRoomClicked() {
    CreateRoomDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        emit createRoomRequested(dialog.getRoomName(), dialog.getPassword(), dialog.getMaxUsers());
    }
}

void RoomListWidget::onJoinRoomClicked() {
    QListWidgetItem* currentItem = m_roomList->currentItem();
    if (!currentItem) return;

    QString roomId = currentItem->data(Qt::UserRole).toString();

    // Vérifier si le salon nécessite un mot de passe
    QString roomText = currentItem->text();
    bool hasPassword = roomText.contains("🔒");

    if (hasPassword) {
        JoinRoomDialog dialog(roomText, true, this);
        if (dialog.exec() == QDialog::Accepted) {
            emit joinRoomRequested(roomId, dialog.getPassword());
        }
    } else {
        emit joinRoomRequested(roomId, QString());
    }
}

void RoomListWidget::onRefreshClicked() {
    emit refreshRequested();
}

void RoomListWidget::onRoomDoubleClicked() {
    onJoinRoomClicked();
}

void RoomListWidget::updateJoinButton() {
    QListWidgetItem* currentItem = m_roomList->currentItem();
    m_joinRoomBtn->setEnabled(currentItem != nullptr);
}

// CreateRoomDialog implementation
CreateRoomDialog::CreateRoomDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Créer un Salon");
    setModal(true);
    setupUI();
}

void CreateRoomDialog::setupUI() {
    setStyleSheet(R"(
        QDialog {
            background: #1a1a2e;
            color: #e2e8f0;
        }

        QLabel {
            color: #e2e8f0;
            font-size: 14px;
        }

        QLineEdit, QSpinBox {
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            padding: 8px 12px;
            color: #ffffff;
            font-size: 14px;
        }

        QLineEdit:focus, QSpinBox:focus {
            border-color: #3b82f6;
            background: rgba(255, 255, 255, 0.15);
        }

        QCheckBox {
            color: #e2e8f0;
            spacing: 8px;
        }

        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 4px;
        }

        QCheckBox::indicator:checked {
            background: #3b82f6;
            border-color: #3b82f6;
        }

        QPushButton {
            background: #3b82f6;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            min-width: 100px;
        }

        QPushButton:hover {
            background: #2563eb;
        }

        QPushButton[role="cancel"] {
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        QPushButton[role="cancel"]:hover {
            background: rgba(255, 255, 255, 0.2);
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // Titre de la dialog
    QLabel* titleLabel = new QLabel("🎵 Créer un nouveau salon", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px;");

    // Nom du salon
    QWidget* nameWidget = new QWidget(this);
    QVBoxLayout* nameLayout = new QVBoxLayout(nameWidget);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(5);

    QLabel* nameLabel = new QLabel("Nom du salon:", this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Ex: Jam Session Cool");

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);

    // Nombre maximum d'utilisateurs
    QWidget* usersWidget = new QWidget(this);
    QHBoxLayout* usersLayout = new QHBoxLayout(usersWidget);
    usersLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* usersLabel = new QLabel("Nombre max de joueurs:", this);
    m_maxUsersSpin = new QSpinBox(this);
    m_maxUsersSpin->setRange(2, 8);
    m_maxUsersSpin->setValue(4);
    m_maxUsersSpin->setFixedWidth(80);

    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(m_maxUsersSpin);
    usersLayout->addStretch();

    // Section mot de passe
    QWidget* passwordSection = new QWidget(this);
    QVBoxLayout* passwordLayout = new QVBoxLayout(passwordSection);
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(10);

    m_passwordCheckBox = new QCheckBox("🔒 Protéger par mot de passe", this);

    QWidget* passwordWidget = new QWidget(this);
    QHBoxLayout* pwdLayout = new QHBoxLayout(passwordWidget);
    pwdLayout->setContentsMargins(20, 0, 0, 0);

    QLabel* pwdLabel = new QLabel("Mot de passe:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setEnabled(false);
    m_passwordEdit->setPlaceholderText("Entrez un mot de passe sécurisé");

    pwdLayout->addWidget(pwdLabel);
    pwdLayout->addWidget(m_passwordEdit);

    passwordLayout->addWidget(m_passwordCheckBox);
    passwordLayout->addWidget(passwordWidget);

    connect(m_passwordCheckBox, &QCheckBox::toggled, m_passwordEdit, &QLineEdit::setEnabled);

    // Séparateur
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background: rgba(255, 255, 255, 0.1); max-height: 1px;");

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* createBtn = new QPushButton("✨ Créer le salon", this);
    QPushButton* cancelBtn = new QPushButton("Annuler", this);
    cancelBtn->setProperty("role", "cancel");

    createBtn->setDefault(true);
    createBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setCursor(Qt::PointingHandCursor);

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(createBtn);

    // Layout principal
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(nameWidget);
    mainLayout->addWidget(usersWidget);
    mainLayout->addWidget(passwordSection);
    mainLayout->addWidget(separator);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);

    // Connexions
    connect(createBtn, &QPushButton::clicked, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            m_nameEdit->setStyleSheet(m_nameEdit->styleSheet() + "border-color: #ef4444;");
            m_nameEdit->setPlaceholderText("⚠️ Le nom est requis");
            return;
        }
        accept();
    });

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Focus initial
    m_nameEdit->setFocus();
}

QString CreateRoomDialog::getRoomName() const {
    return m_nameEdit->text().trimmed();
}

QString CreateRoomDialog::getPassword() const {
    return m_passwordCheckBox->isChecked() ? m_passwordEdit->text() : QString();
}

int CreateRoomDialog::getMaxUsers() const {
    return m_maxUsersSpin->value();
}

// JoinRoomDialog implementation
JoinRoomDialog::JoinRoomDialog(const QString& roomName, bool hasPassword, QWidget* parent)
    : QDialog(parent)
    , m_roomName(roomName)
    , m_hasPassword(hasPassword)
    , m_passwordEdit(nullptr)
{
    setWindowTitle("Rejoindre le Salon");
    setModal(true);
    setupUI();
}

void JoinRoomDialog::setupUI() {
    setStyleSheet(R"(
        QDialog {
            background: #1a1a2e;
            color: #e2e8f0;
        }

        QLabel {
            color: #e2e8f0;
            font-size: 14px;
        }

        QLineEdit {
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            padding: 10px 15px;
            color: #ffffff;
            font-size: 14px;
        }

        QLineEdit:focus {
            border-color: #3b82f6;
            background: rgba(255, 255, 255, 0.15);
        }

        QPushButton {
            background: #10b981;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            min-width: 100px;
        }

        QPushButton:hover {
            background: #059669;
        }

        QPushButton[role="cancel"] {
            background: rgba(255, 255, 255, 0.1);
            border: 1px solid rgba(255, 255, 255, 0.2);
        }

        QPushButton[role="cancel"]:hover {
            background: rgba(255, 255, 255, 0.2);
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // Icône et titre
    QLabel* titleLabel = new QLabel("🚪 Rejoindre le salon", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    // Information du salon
    QWidget* infoWidget = new QWidget(this);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(10);
    infoWidget->setStyleSheet(R"(
        background: rgba(255, 255, 255, 0.05);
        border-radius: 10px;
    )");

    QLabel* roomNameLabel = new QLabel(m_roomName, this);
    roomNameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #3b82f6;");

    infoLayout->addWidget(roomNameLabel);

    // Mot de passe si nécessaire
    if (m_hasPassword) {
        QLabel* lockIcon = new QLabel("🔒 Ce salon est protégé par mot de passe", this);
        lockIcon->setStyleSheet("color: #fbbf24; font-size: 14px;");
        infoLayout->addWidget(lockIcon);

        QWidget* passwordWidget = new QWidget(this);
        QVBoxLayout* pwdLayout = new QVBoxLayout(passwordWidget);
        pwdLayout->setContentsMargins(0, 10, 0, 0);

        QLabel* pwdLabel = new QLabel("Mot de passe:", this);
        m_passwordEdit = new QLineEdit(this);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_passwordEdit->setPlaceholderText("Entrez le mot de passe du salon");

        pwdLayout->addWidget(pwdLabel);
        pwdLayout->addWidget(m_passwordEdit);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(infoWidget);
        mainLayout->addWidget(passwordWidget);
    } else {
        QLabel* openIcon = new QLabel("🌟 Ce salon est ouvert à tous", this);
        openIcon->setStyleSheet("color: #10b981; font-size: 14px;");
        infoLayout->addWidget(openIcon);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(infoWidget);
    }

    // Séparateur
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background: rgba(255, 255, 255, 0.1); max-height: 1px;");
    mainLayout->addWidget(separator);

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* joinBtn = new QPushButton("✨ Rejoindre", this);
    QPushButton* cancelBtn = new QPushButton("Annuler", this);
    cancelBtn->setProperty("role", "cancel");

    joinBtn->setDefault(true);
    joinBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setCursor(Qt::PointingHandCursor);

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(joinBtn);

    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);

    // Connexions
    connect(joinBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    if (m_passwordEdit) {
        m_passwordEdit->setFocus();
    }
}

QString JoinRoomDialog::getPassword() const {
    return m_passwordEdit ? m_passwordEdit->text() : QString();
}
