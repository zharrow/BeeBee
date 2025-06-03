#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include "Protocol.h"

// Forward declarations
class DrumServer;
class DrumClient;

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    // Mode serveur
    bool startServer(quint16 port = 8888);
    void stopServer();
    bool isServerRunning() const;

    // Mode client
    bool connectToServer(const QString& host, quint16 port = 8888);
    void disconnectFromServer();
    bool isClientConnected() const;

    // Communication
    void sendMessage(const QByteArray& message);
    void broadcastMessage(const QByteArray& message); // Serveur uniquement

    // État - inline functions pour éviter les redéfinitions
    bool isServer() const { return m_isServer; }
    QString getUserId() const { return m_userId; }
    void setUserId(const QString& userId) { m_userId = userId; }

signals:
    // Événements réseau
    void messageReceived(const QByteArray& message);
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);
    void connectionEstablished();
    void connectionLost();
    void errorOccurred(const QString& error);

private:
    DrumServer* m_server;
    DrumClient* m_client;
    bool m_isServer;
    QString m_userId;
};
