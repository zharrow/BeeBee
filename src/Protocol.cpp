#include "Protocol.h"
#include "Room.h"
#include <QJsonDocument>
#include <QIODevice>
#include <QDataStream>

QByteArray Protocol::createMessage(MessageType type, const QJsonObject& data) {
    QJsonObject message;
    message["type"] = messageTypeToString(type);
    message["data"] = data;
    message["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // Préfixe avec la taille du message pour le parsing
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(jsonData.size());
    result.append(jsonData);

    return result;
}

bool Protocol::parseMessage(const QByteArray& data, MessageType& type, QJsonObject& content) {
    if (data.size() < 4) return false;

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    quint32 messageSize;
    stream >> messageSize;

    if (data.size() < 4 + messageSize) return false;

    QByteArray jsonData = data.mid(4, messageSize);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

    if (error.error != QJsonParseError::NoError) return false;

    QJsonObject message = doc.object();

    QString typeStr = message["type"].toString();
    type = stringToMessageType(typeStr);
    if (type == static_cast<MessageType>(-1)) return false;

    content = message["data"].toObject();
    return true;
}

QByteArray Protocol::createJoinMessage(const QString& userName) {
    QJsonObject data;
    data["userName"] = userName;
    return createMessage(MessageType::JOIN_SESSION, data);
}

QByteArray Protocol::createGridUpdateMessage(const GridCell& cell) {
    return createMessage(MessageType::GRID_UPDATE, cell.toJson());
}

QByteArray Protocol::createTempoMessage(int bpm) {
    QJsonObject data;
    data["bpm"] = bpm;
    return createMessage(MessageType::TEMPO_CHANGE, data);
}

QByteArray Protocol::createPlayStateMessage(bool playing) {
    QJsonObject data;
    data["playing"] = playing;
    return createMessage(MessageType::PLAY_STATE, data);
}

QByteArray Protocol::createRoomInfoRequestMessage(const QJsonObject& data) {
    return createMessage(MessageType::ROOM_INFO_REQUEST, data);
}


QByteArray Protocol::createSyncRequestMessage() {
    return createMessage(MessageType::SYNC_REQUEST, QJsonObject());
}

QByteArray Protocol::createSyncResponseMessage(const QJsonObject& gridState) {
    return createMessage(MessageType::SYNC_RESPONSE, gridState);
}

// Nouvelles méthodes pour les messages de rooms
QByteArray Protocol::createCreateRoomMessage(const QString& name, const QString& password, int maxUsers) {
    QJsonObject data;
    data["name"] = name;
    data["password"] = password;
    data["maxUsers"] = maxUsers;
    return createMessage(MessageType::CREATE_ROOM, data);
}

QByteArray Protocol::createJoinRoomMessage(const QString& roomId, const QString& password) {
    QJsonObject data;
    data["roomId"] = roomId;
    data["password"] = password;
    return createMessage(MessageType::JOIN_ROOM, data);
}

QByteArray Protocol::createLeaveRoomMessage(const QString& roomId) {
    QJsonObject data;
    data["roomId"] = roomId;
    return createMessage(MessageType::LEAVE_ROOM, data);
}

QByteArray Protocol::createRoomListRequestMessage() {
    return createMessage(MessageType::ROOM_LIST_REQUEST, QJsonObject());
}

QByteArray Protocol::createRoomListResponseMessage(const QJsonArray& rooms) {
    QJsonObject data;
    data["rooms"] = rooms;
    return createMessage(MessageType::ROOM_LIST_RESPONSE, data);
}

QByteArray Protocol::createRoomInfoMessage(const QJsonObject& roomInfo) {
    return createMessage(MessageType::ROOM_INFO, roomInfo);
}

QByteArray Protocol::createUserJoinedMessage(const User& user) {
    return createMessage(MessageType::USER_JOINED, user.toJson());
}

QByteArray Protocol::createUserLeftMessage(const QString& userId) {
    QJsonObject data;
    data["userId"] = userId;
    return createMessage(MessageType::USER_LEFT, data);
}

QByteArray Protocol::createHostChangedMessage(const QString& oldHostId, const QString& newHostId) {
    QJsonObject data;
    data["oldHostId"] = oldHostId;
    data["newHostId"] = newHostId;
    return createMessage(MessageType::HOST_CHANGED, data);
}

QByteArray Protocol::createErrorMessage(const QString& error) {
    QJsonObject data;
    data["message"] = error;
    return createMessage(MessageType::ERROR_MESSAGE, data);
}

QString Protocol::messageTypeToString(MessageType type) {
    switch (type) {
    case MessageType::CREATE_ROOM: return "CREATE_ROOM";
    case MessageType::JOIN_ROOM: return "JOIN_ROOM";
    case MessageType::LEAVE_ROOM: return "LEAVE_ROOM";
    case MessageType::ROOM_LIST_REQUEST: return "ROOM_LIST_REQUEST";
    case MessageType::ROOM_LIST_RESPONSE: return "ROOM_LIST_RESPONSE";
    case MessageType::ROOM_INFO: return "ROOM_INFO";
    case MessageType::USER_JOINED: return "USER_JOINED";
    case MessageType::USER_LEFT: return "USER_LEFT";
    case MessageType::HOST_CHANGED: return "HOST_CHANGED";
    case MessageType::JOIN_SESSION: return "JOIN_SESSION";
    case MessageType::GRID_UPDATE: return "GRID_UPDATE";
    case MessageType::TEMPO_CHANGE: return "TEMPO_CHANGE";
    case MessageType::PLAY_STATE: return "PLAY_STATE";
    case MessageType::SYNC_REQUEST: return "SYNC_REQUEST";
    case MessageType::SYNC_RESPONSE: return "SYNC_RESPONSE";
    case MessageType::CHAT_MESSAGE: return "CHAT_MESSAGE";
    case MessageType::USER_INFO: return "USER_INFO";
    case MessageType::ERROR_MESSAGE: return "ERROR_MESSAGE";
    default: return "UNKNOWN";
    }
}

MessageType Protocol::stringToMessageType(const QString& str) {
    if (str == "CREATE_ROOM") return MessageType::CREATE_ROOM;
    if (str == "JOIN_ROOM") return MessageType::JOIN_ROOM;
    if (str == "LEAVE_ROOM") return MessageType::LEAVE_ROOM;
    if (str == "ROOM_LIST_REQUEST") return MessageType::ROOM_LIST_REQUEST;
    if (str == "ROOM_LIST_RESPONSE") return MessageType::ROOM_LIST_RESPONSE;
    if (str == "ROOM_INFO") return MessageType::ROOM_INFO;
    if (str == "USER_JOINED") return MessageType::USER_JOINED;
    if (str == "USER_LEFT") return MessageType::USER_LEFT;
    if (str == "HOST_CHANGED") return MessageType::HOST_CHANGED;
    if (str == "JOIN_SESSION") return MessageType::JOIN_SESSION;
    if (str == "GRID_UPDATE") return MessageType::GRID_UPDATE;
    if (str == "TEMPO_CHANGE") return MessageType::TEMPO_CHANGE;
    if (str == "PLAY_STATE") return MessageType::PLAY_STATE;
    if (str == "SYNC_REQUEST") return MessageType::SYNC_REQUEST;
    if (str == "SYNC_RESPONSE") return MessageType::SYNC_RESPONSE;
    if (str == "CHAT_MESSAGE") return MessageType::CHAT_MESSAGE;
    if (str == "USER_INFO") return MessageType::USER_INFO;
    if (str == "ERROR_MESSAGE") return MessageType::ERROR_MESSAGE;
    return static_cast<MessageType>(-1);
}
