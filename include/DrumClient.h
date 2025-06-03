#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class DrumClient : public QObject {
    Q_OBJECT

public:
    explicit DrumClient(QObject* parent = nullptr);
    ~DrumClient();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendMessage(const QByteArray& message);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& message);
    void errorOccurred(const QString& error);

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
