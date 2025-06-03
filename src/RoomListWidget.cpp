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

void RoomListWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // En-tête
    QLabel* titleLabel = new QLabel("Salons Disponibles", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px 0;");

    // Liste des rooms
    m_roomList = new QListWidget(this);
    m_roomList->setAlternatingRowColors(true);
    connect(m_roomList, &QListWidget::itemDoubleClicked, this, &RoomListWidget::onRoomDoubleClicked);
    connect(m_roomList, &QListWidget::itemSelectionChanged, this, &RoomListWidget::updateJoinButton);

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_createRoomBtn = new QPushButton("Créer un Salon", this);
    m_joinRoomBtn = new QPushButton("Rejoindre", this);
    m_refreshBtn = new QPushButton("Actualiser", this);

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
            item->setForeground(QColor(0, 0, 0));
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
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Nom du salon
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Nom du salon:", this));
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Mon Super Beat");
    nameLayout->addWidget(m_nameEdit);

    // Nombre maximum d'utilisateurs
    QHBoxLayout* maxUsersLayout = new QHBoxLayout();
    maxUsersLayout->addWidget(new QLabel("Nombre max d'utilisateurs:", this));
    m_maxUsersSpin = new QSpinBox(this);
    m_maxUsersSpin->setRange(2, 8);
    m_maxUsersSpin->setValue(4);
    maxUsersLayout->addWidget(m_maxUsersSpin);
    maxUsersLayout->addStretch();

    // Mot de passe optionnel
    m_passwordCheckBox = new QCheckBox("Protéger par mot de passe", this);

    QHBoxLayout* passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(new QLabel("Mot de passe:", this));
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setEnabled(false);
    passwordLayout->addWidget(m_passwordEdit);

    connect(m_passwordCheckBox, &QCheckBox::toggled, m_passwordEdit, &QLineEdit::setEnabled);

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* createBtn = new QPushButton("Créer", this);
    QPushButton* cancelBtn = new QPushButton("Annuler", this);

    createBtn->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(createBtn);
    buttonLayout->addWidget(cancelBtn);

    // Layout principal
    mainLayout->addLayout(nameLayout);
    mainLayout->addLayout(maxUsersLayout);
    mainLayout->addWidget(m_passwordCheckBox);
    mainLayout->addLayout(passwordLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(buttonLayout);

    // Connexions
    connect(createBtn, &QPushButton::clicked, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Erreur", "Veuillez entrer un nom pour le salon.");
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
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Information du salon
    QLabel* infoLabel = new QLabel(QString("Rejoindre: %1").arg(m_roomName), this);
    infoLabel->setStyleSheet("font-weight: bold; margin-bottom: 10px;");

    // Mot de passe si nécessaire
    if (m_hasPassword) {
        QHBoxLayout* passwordLayout = new QHBoxLayout();
        passwordLayout->addWidget(new QLabel("Mot de passe:", this));
        m_passwordEdit = new QLineEdit(this);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        passwordLayout->addWidget(m_passwordEdit);

        mainLayout->addWidget(infoLabel);
        mainLayout->addLayout(passwordLayout);
    } else {
        mainLayout->addWidget(infoLabel);
        mainLayout->addWidget(new QLabel("Ce salon n'est pas protégé par mot de passe.", this));
    }

    // Boutons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* joinBtn = new QPushButton("Rejoindre", this);
    QPushButton* cancelBtn = new QPushButton("Annuler", this);

    joinBtn->setDefault(true);

    buttonLayout->addStretch();
    buttonLayout->addWidget(joinBtn);
    buttonLayout->addWidget(cancelBtn);

    mainLayout->addSpacing(20);
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
