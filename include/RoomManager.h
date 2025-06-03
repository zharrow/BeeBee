#pragma once
#include <QObject>
#include <QMap>
#include <QTimer>
#include "Room.h"

class RoomManager : public QObject {
    Q_OBJECT

public:
    explicit RoomManager(QObject* parent = nullptr);
    ~RoomManager();

    // Gestion des rooms
    QString createRoom(const QString& name, const QString& hostId, const QString& hostName, const QString& password = QString());
    bool deleteRoom(const QString& roomId);
    Room* getRoom(const QString& roomId) const;
    QList<Room*> getAllRooms() const;
    QList<Room*> getPublicRooms() const; // Rooms sans mot de passe

    // Utilisateurs
    bool joinRoom(const QString& roomId, const QString& userId, const QString& userName, const QString& password = QString());
    bool leaveRoom(const QString& roomId, const QString& userId);
    QString findUserRoom(const QString& userId) const;

    // Statistiques
    int getRoomCount() const { return m_rooms.size(); }
    int getTotalUsers() const;

    // Maintenance
    void cleanupEmptyRooms();
    void setUserOffline(const QString& userId);

signals:
    void roomCreated(const QString& roomId);
    void roomDeleted(const QString& roomId);
    void userJoinedRoom(const QString& roomId, const User& user);
    void userLeftRoom(const QString& roomId, const QString& userId);
    void roomListChanged();

private slots:
    void onRoomEmpty();
    void onCleanupTimer();

private:
    QMap<QString, Room*> m_rooms;
    QTimer* m_cleanupTimer;

    QString generateRoomId() const;
};
