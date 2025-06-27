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

void DrumClient::processMessage(const QByteArray &data) {
    if (data.size() < 4) {
        qWarning() << "[CLIENT] Message trop court reçu";
        return;
    }

    MessageType type;
    QJsonObject content;

    if (!Protocol::parseMessage(data, type, content)) {
        qWarning() << "[CLIENT] Impossible de parser le message";
        return;
    }

    qDebug() << "[CLIENT] Message reçu type:" << Protocol::messageTypeToString(type);

    switch (type) {
    case MessageType::ROOM_LIST_RESPONSE: {
        qDebug() << "[CLIENT] === DIAGNOSTIC ROOM_LIST_RESPONSE ===";
        qDebug() << "[CLIENT] Contenu JSON complet reçu:" << QJsonDocument(content).toJson(QJsonDocument::Compact);
        qDebug() << "[CLIENT] Clés disponibles dans content:" << content.keys();
        qDebug() << "[CLIENT] Contient 'rooms':" << content.contains("rooms");

        if (content.contains("rooms")) {
            QJsonValue roomsValue = content["rooms"];
            qDebug() << "[CLIENT] Type de 'rooms':" << (roomsValue.isArray() ? "Array" : "Autre");

            if (roomsValue.isArray()) {
                QJsonArray roomsArray = roomsValue.toArray();
                qDebug() << "[CLIENT] Nombre de salles dans le JSON:" << roomsArray.size();

                // Afficher chaque salle
                for (int i = 0; i < roomsArray.size(); ++i) {
                    QJsonObject room = roomsArray[i].toObject();
                    qDebug() << "[CLIENT] Salle" << i << ":" << room["name"].toString()
                             << "ID:" << room["id"].toString();
                }

                emit roomListReceived(roomsArray);
            } else {
                qWarning() << "[CLIENT] 'rooms' n'est pas un array";
            }
        } else {
            qWarning() << "[CLIENT] Pas de clé 'rooms' dans le JSON";
        }

        qDebug() << "[CLIENT] === FIN DIAGNOSTIC ===";
        break;
    }

    case MessageType::GRID_UPDATE: {
        GridCell cell = GridCell::fromJson(content);
        emit gridCellUpdated(cell);
        break;
    }

    case MessageType::ROOM_INFO: {
        emit roomStateReceived(content);
        break;
    }
    default:
        qWarning() << "[CLIENT] Type de message non géré:" << static_cast<int>(type);
    }

    emit messageReceived(data);
}

void DrumClient::joinRoom(const QString& roomId, const QString& userId, const QString& userName, const QString& password) {
    if (isConnected()) {
        QJsonObject content;
        content["roomId"] = roomId;
        content["userId"] = userId;
        content["userName"] = userName;
        content["password"] = password;
        QByteArray message = Protocol::createJoinRoomMessage(roomId, userId, userName, password);
        sendMessage(message);
    } else {
        qWarning() << "[CLIENT] Pas de connexion pour rejoindre la salle";
    }
}


void DrumClient::requestRoomList() {
    if (isConnected()) {
        qDebug() << "[CLIENT] Demande de liste des salles";
        QByteArray message = Protocol::createRoomListRequestMessage();
        sendMessage(message);
    } else {
        qWarning() << "[CLIENT] Pas de connexion pour demander la liste des salles";
    }
}

void DrumClient::requestRoomState(const QString& roomId) {
    if (isConnected()) {
        qDebug() << "[CLIENT] Demande d'état de salle:" << roomId;
        QJsonObject data;
        data["roomId"] = roomId;
        QByteArray message = Protocol::createRoomInfoRequestMessage(data);
        sendMessage(message);
    }
}

