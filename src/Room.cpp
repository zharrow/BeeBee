// Room.cpp - Version corrigée
#include "Room.h"
#include <QRandomGenerator>
#include <QJsonArray>
#include <QDebug>

Room::Room(const QString& id, const QString& name, const QString& hostId, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_name(name)
    , m_hostId(hostId)
    , m_maxUsers(4)
    , m_createdTime(QDateTime::currentDateTime())
{
}

bool Room::addUser(const User& user) {
    if (isFull() || hasUser(user.id)) {
        return false;
    }

    User newUser = user;
    if (newUser.color == QColor()) {
        newUser.color = generateUserColor();
    }

    newUser.joinTime = QDateTime::currentDateTime();
    newUser.isOnline = true;

    m_users[user.id] = newUser;
    emit userJoined(newUser);
    return true;
}

bool Room::removeUser(const QString& userId) {
    if (!hasUser(userId)) {
        return false;
    }

    bool wasHost = (userId == m_hostId);
    m_users.remove(userId);
    emit userLeft(userId);

    if (isEmpty()) {
        emit roomEmpty();
    } else if (wasHost) {
        selectNewHost();
    }

    return true;
}

bool Room::hasUser(const QString& userId) const {
    return m_users.contains(userId);
}

User Room::getUser(const QString& userId) const {
    return m_users.value(userId);
}

QList<User> Room::getUsers() const {
    return m_users.values();
}

QStringList Room::getUserIds() const {
    return m_users.keys();
}

void Room::setUserOnlineStatus(const QString& userId, bool online) {
    if (hasUser(userId)) {
        m_users[userId].isOnline = online;
        if (!online && userId == m_hostId) {
            selectNewHost();
        }
    }
}

bool Room::transferHost(const QString& newHostId) {
    if (!hasUser(newHostId) || newHostId == m_hostId) {
        return false;
    }

    QString oldHostId = m_hostId;

    // Retirer le statut d'hôte à l'ancien hôte
    if (hasUser(oldHostId)) {
        m_users[oldHostId].isHost = false;
    }

    // Donner le statut d'hôte au nouveau
    m_hostId = newHostId;
    m_users[newHostId].isHost = true;

    emit hostChanged(oldHostId, newHostId);
    return true;
}

void Room::selectNewHost() {
    // Chercher un utilisateur en ligne pour être le nouvel hôte
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        if (it.value().isOnline && it.key() != m_hostId) {
            transferHost(it.key());
            return;
        }
    }
}

QColor Room::generateUserColor() const {
    // Couleurs prédéfinies pour les utilisateurs
    static const QList<QColor> colors = {
        QColor(255, 100, 100), // Rouge
        QColor(100, 255, 100), // Vert
        QColor(100, 100, 255), // Bleu
        QColor(255, 255, 100), // Jaune
        QColor(255, 100, 255), // Magenta
        QColor(100, 255, 255), // Cyan
        QColor(255, 150, 100), // Orange
        QColor(150, 100, 255)  // Violet
    };

    // Utiliser des couleurs non prises (sans QSet)
    QList<QColor> usedColors;
    for (const User& user : m_users) {
        usedColors.append(user.color);
    }

    for (const QColor& color : colors) {
        bool isUsed = false;
        for (const QColor& usedColor : usedColors) {
            if (color == usedColor) {
                isUsed = true;
                break;
            }
        }
        if (!isUsed) {
            return color;
        }
    }

    // Si toutes les couleurs sont prises, générer une couleur aléatoire
    return QColor::fromHsv(
        QRandomGenerator::global()->bounded(360),
        200 + QRandomGenerator::global()->bounded(56),
        200 + QRandomGenerator::global()->bounded(56)
        );
}

QJsonObject Room::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    obj["hasPassword"] = hasPassword();
    obj["hostId"] = m_hostId;
    obj["maxUsers"] = m_maxUsers;
    obj["currentUsers"] = getUserCount();
    obj["createdTime"] = m_createdTime.toString(Qt::ISODate);

    QJsonArray usersArray;
    for (const User& user : m_users) {
        usersArray.append(user.toJson());
    }
    obj["users"] = usersArray;

    return obj;
}

Room* Room::fromJson(const QJsonObject& obj, QObject* parent) {
    QString id = obj["id"].toString();
    QString name = obj["name"].toString();
    QString hostId = obj["hostId"].toString();

    Room* room = new Room(id, name, hostId, parent);
    room->m_maxUsers = obj["maxUsers"].toInt(4);
    room->m_createdTime = QDateTime::fromString(obj["createdTime"].toString(), Qt::ISODate);

    QJsonArray usersArray = obj["users"].toArray();
    for (const auto& userValue : usersArray) {
        User user = User::fromJson(userValue.toObject());
        room->m_users[user.id] = user;
    }

    return room;
}
