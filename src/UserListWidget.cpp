#include "UserListWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QIcon>

UserListWidget::UserListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void UserListWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // Info de la room avec style moderne
    m_roomNameLabel = new QLabel("Aucun salon", this);
    m_roomNameLabel->setStyleSheet(R"(
        font-weight: bold;
        font-size: 18px;
        color: #ffffff;
        padding: 10px;
        background: rgba(59, 130, 246, 0.2);
        border-radius: 8px;
        text-align: center;
    )");

    m_userCountLabel = new QLabel("0/4 utilisateurs", this);
    m_userCountLabel->setStyleSheet(R"(
        color: #94a3b8;
        font-size: 14px;
        padding: 5px 10px;
        background: rgba(255, 255, 255, 0.05);
        border-radius: 5px;
        text-align: center;
    )");

    // Liste des utilisateurs avec style moderne
    QLabel* usersLabel = new QLabel("Utilisateurs:", this);
    usersLabel->setStyleSheet(R"(
        font-weight: bold;
        color: #e2e8f0;
        font-size: 16px;
        margin-top: 15px;
    )");

    m_userList = new QListWidget(this);
    m_userList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_userList->setStyleSheet(R"(
        QListWidget {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            padding: 10px;
            color: #e2e8f0;
        }

        QListWidget::item {
            background: transparent;
            padding: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }

        QListWidget::item:hover {
            background: rgba(255, 255, 255, 0.08);
        }

        QListWidget::item:selected {
            background: rgba(59, 130, 246, 0.2);
        }
    )");

    connect(m_userList, &QListWidget::customContextMenuRequested,
            this, &UserListWidget::onUserContextMenu);

    // Bouton pour quitter avec style moderne
    m_leaveRoomBtn = new QPushButton("🚪 Quitter le Salon", this);
    m_leaveRoomBtn->setEnabled(false);
    m_leaveRoomBtn->setCursor(Qt::PointingHandCursor);
    m_leaveRoomBtn->setStyleSheet(R"(
        QPushButton {
            background: #ef4444;
            color: white;
            font-weight: bold;
            padding: 12px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
        }
        QPushButton:hover {
            background: #dc2626;
            transform: translateY(-2px);
        }
        QPushButton:pressed {
            transform: translateY(0px);
        }
        QPushButton:disabled {
            background: #4b5563;
            color: #9ca3af;
        }
    )");

    connect(m_leaveRoomBtn, &QPushButton::clicked, this, &UserListWidget::onLeaveRoomClicked);

    layout->addWidget(m_roomNameLabel);
    layout->addWidget(m_userCountLabel);
    layout->addWidget(usersLabel);
    layout->addWidget(m_userList, 1);
    layout->addWidget(m_leaveRoomBtn);
}

void UserListWidget::updateUserList(const QList<User>& users) {
    m_users = users;
    m_userList->clear();

    for (const User& user : users) {
        QListWidgetItem* item = new QListWidgetItem();

        QString displayText = user.name;
        if (user.isHost) {
            displayText += " 👑"; // Couronne pour l'hôte
        }
        if (!user.isOnline) {
            displayText += " (Hors ligne)";
        }

        item->setText(displayText);
        item->setData(Qt::UserRole, user.id);

        // Couleur de l'utilisateur
        QPixmap colorPixmap(16, 16);
        colorPixmap.fill(user.color);
        item->setIcon(QIcon(colorPixmap));

        // Style selon le statut
        if (!user.isOnline) {
            item->setForeground(QColor(150, 150, 150));
        } else if (user.id == m_currentUserId) {
            item->setForeground(QColor(100, 150, 255));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }

        m_userList->addItem(item);
    }

    updateRoomInfo();
}

void UserListWidget::setCurrentUser(const QString& userId) {
    m_currentUserId = userId;
}

void UserListWidget::setCurrentRoom(const QString& roomId, const QString& roomName) {
    m_currentRoomId = roomId;
    m_currentRoomName = roomName;
    m_leaveRoomBtn->setEnabled(!roomId.isEmpty());
    updateRoomInfo();
}

void UserListWidget::updateRoomInfo() {
    if (m_currentRoomId.isEmpty()) {
        m_roomNameLabel->setText("Aucun salon");
        m_userCountLabel->setText("0/4 utilisateurs");
    } else {
        m_roomNameLabel->setText(m_currentRoomName);

        int onlineUsers = 0;
        for (const User& user : m_users) {
            if (user.isOnline) onlineUsers++;
        }

        m_userCountLabel->setText(QString("%1/%2 utilisateurs (%3 en ligne)")
                                      .arg(m_users.size())
                                      .arg(4) // TODO: Récupérer le vrai maxUsers
                                      .arg(onlineUsers));
    }
}

void UserListWidget::onLeaveRoomClicked() {
    int ret = QMessageBox::question(this, "Quitter le Salon",
                                    "Êtes-vous sûr de vouloir quitter ce salon ?",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        emit leaveRoomRequested();
    }
}

void UserListWidget::onUserContextMenu(const QPoint& point) {
    QListWidgetItem* item = m_userList->itemAt(point);
    if (!item) return;

    QString userId = item->data(Qt::UserRole).toString();
    if (userId == m_currentUserId) return; // Pas de menu pour soi-même

    // Vérifier si l'utilisateur actuel est l'hôte
    bool isCurrentUserHost = false;
    for (const User& user : m_users) {
        if (user.id == m_currentUserId && user.isHost) {
            isCurrentUserHost = true;
            break;
        }
    }

    if (!isCurrentUserHost) return; // Seul l'hôte peut utiliser le menu contextuel

    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background: #2d2d44;
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 5px;
        }
        QMenu::item {
            background: transparent;
            color: #e2e8f0;
            padding: 8px 20px;
            border-radius: 4px;
        }
        QMenu::item:hover {
            background: rgba(59, 130, 246, 0.3);
        }
    )");

    QAction* transferHostAction = menu.addAction("👑 Transférer l'hôte");
    QAction* kickAction = menu.addAction("❌ Expulser");

    QAction* selectedAction = menu.exec(m_userList->mapToGlobal(point));

    if (selectedAction == transferHostAction) {
        int ret = QMessageBox::question(this, "Transférer l'hôte",
                                        QString("Transférer le rôle d'hôte à %1 ?").arg(item->text().split(" ")[0]),
                                        QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            emit transferHostRequested(userId);
        }
    } else if (selectedAction == kickAction) {
        int ret = QMessageBox::question(this, "Expulser l'utilisateur",
                                        QString("Expulser %1 du salon ?").arg(item->text().split(" ")[0]),
                                        QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            emit kickUserRequested(userId);
        }
    }
}
