#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QTimer>

class DrumServer : public QObject {
    Q_OBJECT

public:
    explicit DrumServer(QObject* parent = nullptr);
    ~DrumServer();

    bool startListening(quint16 port);
    void stopListening();
    bool isListening() const;

    void broadcastMessage(const QByteArray& message);
    void sendMessageToClient(const QString& clientId, const QByteArray& message);

    QStringList getConnectedClients() const;
    int getClientCount() const;
    bool hasClient(const QString& clientId) const;
    void kickClient(const QString& clientId, const QString& reason = QString());

    QHostAddress getServerAddress() const;
    quint16 getServerPort() const;
    void setMaxPendingConnections(int maxConnections);

signals:
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);
    void messageReceived(const QByteArray& message, const QString& fromClientId);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientDataReceived();
    void onPingTimer();

private:
    void processClientMessage(QTcpSocket* client, const QByteArray& data);
    QString getClientId(QTcpSocket* socket) const;

    QTcpServer* m_server;
    QMap<QString, QTcpSocket*> m_clients;
    QMap<QTcpSocket*, QByteArray> m_clientBuffers;
    QTimer* m_pingTimer;
};
