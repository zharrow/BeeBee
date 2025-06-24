// Protocol.h - Version corrigée
#pragma once
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QString>

// Forward declarations
struct User;

enum class MessageType {
    // Gestion des rooms
    CREATE_ROOM,
    JOIN_ROOM,
    LEAVE_ROOM,
    ROOM_LIST_REQUEST,
    ROOM_LIST_RESPONSE,
    ROOM_INFO_REQUEST,
    ROOM_INFO,
    USER_JOINED,
    USER_LEFT,
    HOST_CHANGED,

    // Session de jeu
    JOIN_SESSION,
    GRID_UPDATE,
    TEMPO_CHANGE,
    PLAY_STATE,
    SYNC_REQUEST,
    SYNC_RESPONSE,

    // Communication
    CHAT_MESSAGE,
    USER_INFO,

    // Erreurs
    ERROR_MESSAGE
};

struct GridCell {
    int row;
    int col;
    bool active;
    QString userId;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["row"] = row;
        obj["col"] = col;
        obj["active"] = active;
        obj["userId"] = userId;
        return obj;
    }

    static GridCell fromJson(const QJsonObject& obj) {
        GridCell cell;
        cell.row = obj["row"].toInt();
        cell.col = obj["col"].toInt();
        cell.active = obj["active"].toBool();
        cell.userId = obj["userId"].toString();
        return cell;
    }
};

class Protocol {
public:
    static QByteArray createMessage(MessageType type, const QJsonObject& data);
    static bool parseMessage(const QByteArray& data, MessageType& type, QJsonObject& content);
    static QByteArray createRoomInfoRequestMessage(const QJsonObject& data);

    // Messages de rooms
    static QByteArray createCreateRoomMessage(const QString& name, const QString& password, int maxUsers);
    static QByteArray createJoinRoomMessage(const QString& roomId, const QString& password);
    static QByteArray createLeaveRoomMessage(const QString& roomId);
    static QByteArray createRoomListRequestMessage();
    static QByteArray createRoomListResponseMessage(const QJsonArray& rooms);
    static QByteArray createRoomInfoMessage(const QJsonObject& roomInfo);
    static QByteArray createUserJoinedMessage(const User& user);
    static QByteArray createUserLeftMessage(const QString& userId);
    static QByteArray createHostChangedMessage(const QString& oldHostId, const QString& newHostId);
    static QByteArray createErrorMessage(const QString& error);
    static QByteArray createJoinRoomMessage(const QString& roomId, const QString& userId, const QString& userName, const QString& password);

    // Messages existants
    static QByteArray createJoinMessage(const QString& userName);
    static QByteArray createGridUpdateMessage(const GridCell& cell);
    static QByteArray createTempoMessage(int bpm);
    static QByteArray createPlayStateMessage(bool playing);
    static QByteArray createSyncRequestMessage();
    static QByteArray createSyncResponseMessage(const QJsonObject& gridState);

    // Fonctions utilitaires
    static QString messageTypeToString(MessageType type);
    static MessageType stringToMessageType(const QString& str);
};
