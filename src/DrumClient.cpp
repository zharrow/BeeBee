#include "DrumClient.h"
#include <QDataStream>
#include <QDebug>
#include "Protocol.h"


DrumClient::DrumClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_pingTimer(new QTimer(this)), m_serverPort(0)
{
    // Connexions des signaux du socket
    connect(m_socket, &QTcpSocket::connected, this, &DrumClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DrumClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &DrumClient::onDataReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &DrumClient::onSocketError);

    // Timer de ping pour maintenir la connexion
    m_pingTimer->setInterval(30000); // 30 secondes
    connect(m_pingTimer, &QTimer::timeout, this, &DrumClient::onPingTimer);
}

DrumClient::~DrumClient()
{
    disconnectFromServer();
}

bool DrumClient::connectToServer(const QString &host, quint16 port)
{
    if (isConnected())
    {
        qWarning() << "Déjà connecté au serveur";
        return false;
    }

    m_serverHost = host;
    m_serverPort = port;

    qDebug() << "Tentative de connexion à" << host << ":" << port;

    m_socket->connectToHost(host, port);
    
    // Attendre la connexion (non-bloquant grâce aux signaux)
    return true;
}

void DrumClient::disconnectFromServer()
{
    if (!isConnected())
    {
        return;
    }

    m_pingTimer->stop();

    if (m_socket->state() != QTcpSocket::UnconnectedState)
    {
        qDebug() << "Déconnexion du serveur";
        m_socket->disconnectFromHost();

        if (!m_socket->waitForDisconnected(3000))
        {
            m_socket->abort();
        }
    }

    m_buffer.clear();
}

bool DrumClient::isConnected() const
{
    return m_socket->state() == QTcpSocket::ConnectedState;
}

void DrumClient::sendMessage(const QByteArray &message)
{
    if (!isConnected())
    {
        qWarning() << "Tentative d'envoi de message sans connexion";
        
        return;
    }

    qint64 written = m_socket->write(message);
    if (written != message.size())
    {
        qWarning() << "Erreur d'envoi de message:" << written << "/" << message.size() << "bytes envoyés";
        emit errorOccurred("Erreur d'envoi de message");
    }
    else
    {
        qDebug() << "Message envoyé au serveur:" << message.size() << "bytes";
    }
}

void DrumClient::onConnected()
{
    qDebug() << "Connecté au serveur" << m_serverHost << ":" << m_serverPort;
    m_pingTimer->start();

    QByteArray request = Protocol::createRoomListRequestMessage();
    sendMessage(request);

    emit connected();
}

void DrumClient::onDisconnected()
{
    qDebug() << "Déconnecté du serveur";
    m_pingTimer->stop();
    m_buffer.clear();
    emit disconnected();
}

void DrumClient::onDataReceived()
{
    QByteArray newData = m_socket->readAll();
    m_buffer.append(newData);

    qDebug() << "Données reçues du serveur:" << newData.size() << "bytes";

    // Traitement des messages complets
    while (m_buffer.size() >= 4)
    {
        QDataStream stream(m_buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 messageSize;
        stream >> messageSize;

        // Validation de la taille
        if (messageSize > 1024 * 1024)
        { // 1MB max
            qWarning() << "Message trop volumineux reçu:" << messageSize << "bytes";
            m_socket->disconnectFromHost();
            return;
        }

        if (m_buffer.size() >= 4 + messageSize)
        {
            QByteArray message = m_buffer.mid(0, 4 + messageSize);
            m_buffer.remove(0, 4 + messageSize);

            qDebug() << "Message complet reçu:" << messageSize << "bytes";
            processMessage(message);
        }
        else
        {
            // Message incomplet, attendre plus de données
            qDebug() << "Message incomplet - attendu:" << (4 + messageSize)
                     << "reçu:" << m_buffer.size();
            break;
        }
    }
}

void DrumClient::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString;

    switch (error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "Connexion refusée par le serveur";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "Le serveur a fermé la connexion";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "Serveur non trouvé";
        break;
    case QAbstractSocket::SocketTimeoutError:
        errorString = "Timeout de connexion";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "Erreur réseau";
        break;
    default:
        errorString = QString("Erreur socket: %1").arg(m_socket->errorString());
        break;
    }

    qWarning() << "Erreur socket:" << errorString;
    emit errorOccurred(errorString);
}

void DrumClient::onPingTimer()
{
    if (isConnected())
    {
        // Vérifier que la connexion est toujours active
        if (m_socket->state() != QTcpSocket::ConnectedState)
        {
            qWarning() << "Connexion perdue détectée par le ping timer";
            onDisconnected();
        }
    }
}

void DrumClient::processMessage(const QByteArray &data)
{
    if (data.size() < 4)
    {
        qWarning() << "Message trop court reçu";
        return;
    }

    emit messageReceived(data);
}
