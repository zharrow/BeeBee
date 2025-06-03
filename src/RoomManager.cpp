#include "RoomManager.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QTimer>

RoomManager::RoomManager(QObject* parent)
    : QObject(parent)
    , m_cleanupTimer(new QTimer(this))
{
    // Nettoyage périodique des rooms vides
    m_cleanupTimer->setInterval(60000); // 1 minute
    connect(m_cleanupTimer, &QTimer::timeout, this, &RoomManager::onCleanupTimer);
    m_cleanupTimer->start();

    qDebug() << "RoomManager initialisé";
}

RoomManager::~RoomManager() {
    qDeleteAll(m_rooms);
}

QString RoomManager::createRoom(const QString& name, const QString& hostId, const QString& hostName, const QString& password) {
    QString roomId = generateRoomId();

    Room* room = new Room(roomId, name, hostId, this);
    if (!password.isEmpty()) {
        room->setPassword(password);
    }

    connect(room, &Room::roomEmpty, this, &RoomManager::onRoomEmpty);
    connect(room, &Room::userJoined, this, [this, roomId](const User& user) {
        emit userJoinedRoom(roomId, user);
    });
    connect(room, &Room::userLeft, this, [this, roomId](const QString& userId) {
        emit userLeftRoom(roomId, userId);
    });

    // Ajouter l'hôte à la room
    User host;
    host.id = hostId;
    host.name = hostName;
    host.isHost = true;
    room->addUser(host);

    m_rooms[roomId] = room;
    emit roomCreated(roomId);
    emit roomListChanged();

    qDebug() << "Room créée:" << roomId << "par" << hostName;
    return roomId;
}

bool RoomManager::deleteRoom(const QString& roomId) {
    if (!m_rooms.contains(roomId)) {
        return false;
    }

    Room* room = m_rooms.take(roomId);
    room->deleteLater();

    emit roomDeleted(roomId);
    emit roomListChanged();

    qDebug() << "Room supprimée:" << roomId;
    return true;
}

Room* RoomManager::getRoom(const QString& roomId) const {
    return m_rooms.value(roomId, nullptr);
}

QList<Room*> RoomManager::getAllRooms() const {
    return m_rooms.values();
}

QList<Room*> RoomManager::getPublicRooms() const {
    QList<Room*> publicRooms;
    for (Room* room : m_rooms) {
        if (!room->hasPassword()) {
            publicRooms.append(room);
        }
    }
    return publicRooms;
}

bool RoomManager::joinRoom(const QString& roomId, const QString& userId, const QString& userName, const QString& password) {
    Room* room = getRoom(roomId);
    if (!room) {
        qWarning() << "Room non trouvée:" << roomId;
        return false;
    }

    if (room->isFull()) {
        qDebug() << "Room pleine:" << roomId;
        return false;
    }

    if (room->hasPassword() && room->getPassword() != password) {
        qDebug() << "Mot de passe incorrect pour room:" << roomId;
        return false;
    }

    User user;
    user.id = userId;
    user.name = userName;
    user.isHost = false;

    bool success = room->addUser(user);
    if (success) {
        emit roomListChanged();
        qDebug() << "Utilisateur" << userName << "a rejoint la room" << roomId;
    }

    return success;
}

bool RoomManager::leaveRoom(const QString& roomId, const QString& userId) {
    Room* room = getRoom(roomId);
    if (!room) {
        return false;
    }

    bool success = room->removeUser(userId);
    if (success) {
        emit roomListChanged();
        qDebug() << "Utilisateur" << userId << "a quitté la room" << roomId;
    }

    return success;
}

QString RoomManager::findUserRoom(const QString& userId) const {
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
        if (it.value()->hasUser(userId)) {
            return it.key();
        }
    }
    return QString();
}

int RoomManager::getTotalUsers() const {
    int total = 0;
    for (Room* room : m_rooms) {
        total += room->getUserCount();
    }
    return total;
}

void RoomManager::cleanupEmptyRooms() {
    QStringList emptyRooms;
    for (auto it = m_rooms.begin(); it != m_rooms.end(); ++it) {
        if (it.value()->isEmpty()) {
            emptyRooms.append(it.key());
        }
    }

    for (const QString& roomId : emptyRooms) {
        deleteRoom(roomId);
    }

    if (!emptyRooms.isEmpty()) {
        qDebug() << "Nettoyage:" << emptyRooms.size() << "rooms vides supprimées";
    }
}

void RoomManager::setUserOffline(const QString& userId) {
    for (Room* room : m_rooms) {
        if (room->hasUser(userId)) {
            room->setUserOnlineStatus(userId, false);
            qDebug() << "Utilisateur" << userId << "marqué hors ligne dans room" << room->getId();
        }
    }
}

void RoomManager::onRoomEmpty() {
    Room* room = qobject_cast<Room*>(sender());
    if (room) {
        // Supprimer la room vide après un délai
        QTimer::singleShot(30000, this, [this, room]() {
            if (room->isEmpty()) {
                QString roomId = room->getId();
                deleteRoom(roomId);
            }
        });
    }
}

void RoomManager::onCleanupTimer() {
    cleanupEmptyRooms();
}

QString RoomManager::generateRoomId() const {
    QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString id;

    do {
        id.clear();
        for (int i = 0; i < 6; ++i) {
            id += chars[QRandomGenerator::global()->bounded(chars.length())];
        }
    } while (m_rooms.contains(id));

    return id;
}
