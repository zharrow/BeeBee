#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include "Protocol.h"

class DrumClient : public QObject {
    Q_OBJECT

public:
    explicit DrumClient(QObject* parent = nullptr);
    ~DrumClient();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendMessage(const QByteArray& message);

    // Méthodes pour les rooms
    void requestRoomList();
    void joinRoom(const QString& roomId, const QString& userId, const QString& userName, const QString& password = QString());

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& message);
    void errorOccurred(const QString& error);

    // Signaux pour les rooms
    void roomListReceived(const QJsonArray& rooms);
    void roomStateReceived(const QJsonObject& roomInfo);
    void gridCellUpdated(const GridCell& cell);

private slots:
    void onConnected();
    void onDisconnected();
    void onDataReceived();
    void onSocketError(QAbstractSocket::SocketError error);
    void onPingTimer();

private:
    void processMessage(const QByteArray& data);

    QTcpSocket* m_socket;
    QByteArray m_buffer;
    QTimer* m_pingTimer;
    QString m_serverHost;
    quint16 m_serverPort;
};
