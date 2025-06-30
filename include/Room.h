// Room.h - Version nettoyée
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QDateTime>
#include <QJsonArray>
#include <QList>
#include <QColor>
#include <QMap>
#include <qjsonarray.h>
#include "GridState.h"

struct User {
    QString id;
    QString name;
    bool isHost;
    bool isOnline;
    QDateTime joinTime;
    QColor color;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["color"] = color.name();
        obj["isHost"] = isHost;
        obj["joinTime"] = joinTime.toString(Qt::ISODate);
        obj["isOnline"] = isOnline;
        return obj;
    }

    static User fromJson(const QJsonObject& obj) {
        User user;
        user.id = obj["id"].toString();
        user.name = obj["name"].toString();
        user.isHost = obj["isHost"].toBool();
        user.isOnline = obj["isOnline"].toBool();
        user.joinTime = QDateTime::fromString(obj["joinTime"].toString(), Qt::ISODate);
        user.color = QColor(obj["color"].toString());
        return user;
    }

    // Utilitaire statique pour convertir un QJsonArray en QList<User>
    static QList<User> listFromJson(const QJsonArray& array) {
        QList<User> users;
        for (const QJsonValue& value : array) {
            if (value.isObject())
                users.append(User::fromJson(value.toObject()));
        }
        return users;
    }
};

class Room : public QObject {
    Q_OBJECT

public:
    explicit Room(const QString& id, const QString& name, const QString& hostId, QObject* parent = nullptr);

    // Propriétés de base
    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString getPassword() const { return m_password; }
    void setPassword(const QString& password) { m_password = password; }
    bool hasPassword() const { return !m_password.isEmpty(); }

    int getMaxUsers() const { return m_maxUsers; }
    void setMaxUsers(int maxUsers) { m_maxUsers = qBound(2, maxUsers, 8); }

    QDateTime getCreatedTime() const { return m_createdTime; }
    QString getHostId() const { return m_hostId; }

    // Gestion des utilisateurs
    bool addUser(const User& user);
    bool removeUser(const QString& userId);
    bool hasUser(const QString& userId) const;
    User getUser(const QString& userId) const;
    QList<User> getUsers() const;
    QStringList getUserIds() const;
    int getUserCount() const { return m_users.size(); }
    bool isFull() const { return getUserCount() >= m_maxUsers; }
    bool isEmpty() const { return m_users.isEmpty(); }

    // État de la room
    bool isActive() const { return !isEmpty(); }
    void setUserOnlineStatus(const QString& userId, bool online);

    // Changement d'hôte
    bool transferHost(const QString& newHostId);
    void selectNewHost();

    // Sérialisation
    QJsonObject toJson() const;
    static Room* fromJson(const QJsonObject& obj, QObject* parent = nullptr);

    // Accéder à la grille de la room
    GridState* grid() const { return m_grid; }

signals:
    void userJoined(const User& user);
    void userLeft(const QString& userId);
    void hostChanged(const QString& oldHostId, const QString& newHostId);
    void roomEmpty();

private:
    QString m_id;
    QString m_name;
    QString m_password;
    QString m_hostId;
    int m_maxUsers;
    QDateTime m_createdTime;
    QMap<QString, User> m_users;
    GridState* m_grid;

    QColor generateUserColor() const;
};
