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
#include <DrumGrid.h>
#include <QHeaderView>

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

    // connect(m_leaveRoomBtn, &QPushButton::clicked, this, &UserListWidget::onLeaveRoomClicked);

    layout->addWidget(m_roomNameLabel);
    layout->addWidget(m_userCountLabel);
    layout->addWidget(usersLabel);
    layout->addWidget(m_userList, 1);
    layout->addWidget(m_leaveRoomBtn);
}

