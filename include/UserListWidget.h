#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "Room.h"

class UserListWidget : public QWidget {
    Q_OBJECT

public:
    explicit UserListWidget(QWidget* parent = nullptr);

    void updateUserList(const QList<User>& users);
    void setCurrentUser(const QString& userId);
    void setCurrentRoom(const QString& roomId, const QString& roomName);

signals:
    void leaveRoomRequested();
    void kickUserRequested(const QString& userId);
    void transferHostRequested(const QString& userId);

private slots:
    void onLeaveRoomClicked();  // Décommenté
    void onUserContextMenu(const QPoint& point);

private:
    void setupUI();
    void updateRoomInfo();

    QLabel* m_roomNameLabel;
    QLabel* m_userCountLabel;
    QListWidget* m_userList;
    QPushButton* m_leaveRoomBtn;

    QString m_currentUserId;
    QString m_currentRoomId;
    QString m_currentRoomName;
    QList<User> m_users;
};
