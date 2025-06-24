#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>

class DrumClient : public QObject {
    Q_OBJECT

public:
    explicit DrumClient(QObject* parent = nullptr);
    ~DrumClient();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    void sendMessage(const QByteArray& message);
    void joinRoom(const QString& roomId, const QString& userId, const QString& userName, const QString& password = QString());
    // Méthodes publiques pour les requêtes
    void requestRoomList();
    void requestRoomState(const QString& roomId);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& message);
    void errorOccurred(const QString& error);
    void roomListReceived(const QJsonArray& rooms);        // Signal pour la liste des salles
    void roomStateReceived(const QJsonObject& state);      // Signal pour l'état d'une salle

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
