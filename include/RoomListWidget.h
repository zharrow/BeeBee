#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QDialog>
#include "Room.h"

class RoomListWidget : public QWidget {
    Q_OBJECT

public:
    explicit RoomListWidget(QWidget* parent = nullptr);

    void updateRoomList(const QList<Room*>& rooms);
    void setCurrentUser(const QString& userId, const QString& userName);

signals:
    void createRoomRequested(const QString& name, const QString& password, int maxUsers);
    void joinRoomRequested(const QString& roomId, const QString& password);
    void refreshRequested();

private slots:
    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onRefreshClicked();
    void onRoomDoubleClicked();
    void updateJoinButton();

private:
    void setupUI();
    QPushButton* createModernButton(const QString& text, const QString& color, const QString& hoverColor);

    QListWidget* m_roomList;
    QPushButton* m_createRoomBtn;
    QPushButton* m_joinRoomBtn;
    QPushButton* m_refreshBtn;

    QString m_currentUserId;
    QString m_currentUserName;
};

class CreateRoomDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateRoomDialog(QWidget* parent = nullptr);

    QString getRoomName() const;
    QString getPassword() const;
    int getMaxUsers() const;

private:
    void setupUI();

    QLineEdit* m_nameEdit;
    QLineEdit* m_passwordEdit;
    QSpinBox* m_maxUsersSpin;
    QCheckBox* m_passwordCheckBox;
};

class JoinRoomDialog : public QDialog {
    Q_OBJECT

public:
    explicit JoinRoomDialog(const QString& roomName, bool hasPassword, QWidget* parent = nullptr);

    QString getPassword() const;

private:
    void setupUI();

    QLineEdit* m_passwordEdit;
    QString m_roomName;
    bool m_hasPassword;
};
