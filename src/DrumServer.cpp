#include "DrumServer.h"
#include "Protocol.h"
#include <QDataStream>
#include <QHostAddress>
#include <QDebug>
#include <QRandomGenerator>

DrumServer::DrumServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_pingTimer(new QTimer(this)),
    m_roomManager(nullptr)
{
    connect(m_server, &QTcpServer::newConnection, this, &DrumServer::onNewConnection);

    // Ping périodique pour détecter les déconnexions
    m_pingTimer->setInterval(30000); // 30 secondes
    connect(m_pingTimer, &QTimer::timeout, this, &DrumServer::onPingTimer);

}

DrumServer::~DrumServer()
{
    stopListening();
}

bool DrumServer::startListening(quint16 port)
{
    if (m_server->listen(QHostAddress::Any, port))
    {
        m_pingTimer->start();
        qDebug() << "Serveur démarré sur le port" << port;
        return true;
    }
    else
    {
        qWarning() << "Impossible de démarrer le serveur:" << m_server->errorString();
        return false;
    }
}

void DrumServer::stopListening()
{
    m_pingTimer->stop();

    // Fermer toutes les connexions clients
    for (auto *socket : m_clients)
    {
        if (socket->state() != QTcpSocket::UnconnectedState)
        {
            socket->disconnectFromHost();
            if (!socket->waitForDisconnected(3000))
            {
                socket->abort();
            }
        }
    }

    m_clients.clear();
    m_clientBuffers.clear();

    if (m_server->isListening())
    {
        m_server->close();
        qDebug() << "Serveur arrêté";
    }
}

bool DrumServer::isListening() const
{
    return m_server->isListening();
}

void DrumServer::broadcastMessage(const QByteArray &message)
{
    int sentCount = 0;
    for (auto *socket : m_clients)
    {
        if (socket->state() == QTcpSocket::ConnectedState)
        {
            qint64 written = socket->write(message);
            if (written == message.size())
            {
                sentCount++;
            }
            else
            {
                qWarning() << "Erreur d'envoi de message au client" << getClientId(socket);
            }
        }
    }
    qDebug() << "Message diffusé à" << sentCount << "clients";
}

void DrumServer::sendMessageToClient(const QString& clientId, const QByteArray& message) {
    qDebug() << "[DEBUG] === DIAGNOSTIC sendMessageToClient ===";
    qDebug() << "[DEBUG] Tentative d'envoi à clientId =" << clientId;
    qDebug() << "[DEBUG] Clients dans m_clients:" << m_clients.keys();
    qDebug() << "[DEBUG] Client existe dans m_clients:" << m_clients.contains(clientId);

    if (!m_clients.contains(clientId)) {
        qWarning() << "[SERVER] Client non trouvé dans m_clients:" << clientId;
        return;
    }

    QTcpSocket* socket = m_clients[clientId];
    qDebug() << "[DEBUG] Socket récupéré:" << (socket != nullptr);

    if (!socket) {
        qWarning() << "[SERVER] Socket null pour client:" << clientId;
        return;
    }

    qDebug() << "[DEBUG] État du socket:" << socket->state();
    qDebug() << "[DEBUG] ConnectedState =" << QAbstractSocket::ConnectedState;

    if (socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[SERVER] Client non connecté:" << clientId << "État:" << socket->state();
        return;
    }

    qint64 written = socket->write(message);
    qDebug() << "[DEBUG] Bytes écrits:" << written << "/" << message.size();

    if (written != message.size()) {
        qWarning() << "[SERVER] Erreur d'envoi partiel:" << written << "/" << message.size();
    } else {
        qDebug() << "[SERVER] Message envoyé avec succès à" << clientId;
    }
    qDebug() << "[DEBUG] === FIN DIAGNOSTIC ===";
}





QStringList DrumServer::getConnectedClients() const
{
    QStringList connectedClients;
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it.value()->state() == QTcpSocket::ConnectedState)
        {
            connectedClients.append(it.key());
        }
    }
    return connectedClients;
}

void DrumServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        QString clientId = generateClientId();

        qDebug() << "[SERVER] === ENREGISTREMENT CLIENT ===";
        qDebug() << "[SERVER] Nouveau client ID:" << clientId;
        qDebug() << "[SERVER] Socket valide:" << (socket != nullptr);
        qDebug() << "[SERVER] État initial du socket:" << socket->state();

        m_clients[clientId] = socket;
        m_socketToId[socket] = clientId;

        qDebug() << "[SERVER] Nouveau client connecté:" << clientId;

        // Connexion simple pour la réception de données
        connect(socket, &QTcpSocket::readyRead, this, &DrumServer::onClientDataReceived);

        connect(socket, &QTcpSocket::disconnected, this, [this, socket, clientId]() {
            qDebug() << "[SERVER] Client déconnecté:" << clientId;
            m_clients.remove(clientId);
            m_socketToId.remove(socket);
            m_clientBuffers.remove(socket);
            socket->deleteLater();
            emit clientDisconnected(clientId);
        });

        // Envoi initial de la liste des salles avec un délai
        QTimer::singleShot(100, this, [this, clientId]() {
            if (m_clients.contains(clientId)) {
                sendInitialRoomList(clientId);
            }
        });

        emit clientConnected(clientId);
    }
}

void DrumServer::sendInitialRoomList(const QString& clientId) {
    if (!m_roomManager) return;

    qDebug() << "[SERVER] Envoi initial de la liste des salles à" << clientId;

    QList<Room*> publicRooms = m_roomManager->getPublicRooms();
    QJsonArray roomArray;
    for (Room* room : publicRooms) {
        if (room) {
            roomArray.append(room->toJson());
        }
    }

    QByteArray response = Protocol::createRoomListResponseMessage(roomArray);
    sendMessageToClient(clientId, response);
}




QString DrumServer::generateClientId() const
{
    return QString::number(QRandomGenerator::global()->generate());
}

void DrumServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    QString clientId = m_socketToId.value(socket, "");
    if (clientId.isEmpty())
    {
        qWarning() << "[SERVER] Socket inconnu pour message reçu";
        return;
    }
    if (!clientId.isEmpty())
    {
        qDebug() << "Client déconnecté:" << clientId;

        m_clients.remove(clientId);
        m_clientBuffers.remove(socket);

        emit clientDisconnected(clientId);
    }

    // Programmer la suppression du socket
    socket->deleteLater();
}

void DrumServer::onClientDataReceived() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QString clientId = m_socketToId.value(socket);
    if (clientId.isEmpty()) {
        qWarning() << "[SERVER] Socket sans ID client";
        return;
    }

    QByteArray& buffer = m_clientBuffers[socket];
    buffer.append(socket->readAll());

    while (buffer.size() >= 4) {
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 messageSize;
        stream >> messageSize;

        if (messageSize > 1024 * 1024) {
            qWarning() << "[SERVER] Message trop volumineux";
            socket->disconnectFromHost();
            return;
        }

        if (buffer.size() >= 4 + messageSize) {
            QByteArray message = buffer.mid(0, 4 + messageSize);
            buffer.remove(0, 4 + messageSize);
            processClientMessage(socket, message);
        } else {
            break;
        }
    }
}


void DrumServer::onPingTimer()
{
    // Vérification de l'état des connexions
    QStringList disconnectedClients;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        QTcpSocket *socket = it.value();
        if (socket->state() != QTcpSocket::ConnectedState)
        {
            disconnectedClients.append(it.key());
        }
    }

    // Nettoyage des clients déconnectés
    for (const QString &clientId : disconnectedClients)
    {
        qDebug() << "Nettoyage du client déconnecté:" << clientId;
        QTcpSocket *socket = m_clients.take(clientId);
        m_clientBuffers.remove(socket);
        emit clientDisconnected(clientId);
        socket->deleteLater();
    }

    if (!disconnectedClients.isEmpty())
    {
        qDebug() << "Clients connectés actuels:" << getConnectedClients().size();
    }
}

void DrumServer::setRoomManager(RoomManager* roomManager) {
    Q_ASSERT(roomManager != nullptr);
    m_roomManager = roomManager;
    qDebug() << "[SERVER] RoomManager partagé configuré";
}




void DrumServer::processClientMessage(QTcpSocket* socket, const QByteArray& message) {
    QString clientId = m_clients.key(socket);

    if (clientId.isEmpty()) {
        qWarning() << "[SERVER] Socket non enregistré";
        return;
    }

    MessageType type;
    QJsonObject content;
    if (!Protocol::parseMessage(message, type, content)) {
        qWarning() << "[SERVER] Message invalide reçu de" << clientId;
        return;
    }

    switch (type) {
    case MessageType::ROOM_LIST_REQUEST: {
        qDebug() << "[SERVER] Traitement ROOM_LIST_REQUEST pour" << clientId;

        QList<Room*> publicRooms = m_roomManager->getPublicRooms();
        QJsonArray roomArray;
        for (Room* room : publicRooms) {
            if (room) {
                roomArray.append(room->toJson());
            }
        }

        QByteArray response = Protocol::createRoomListResponseMessage(roomArray);

        // UTILISER sendMessageToClient au lieu de l'autre méthode
        sendMessageToClient(clientId, response);
        break;
    }

    case MessageType::CREATE_ROOM: {
        QString name = content["name"].toString();
        QString password = content["password"].toString();
        int maxUsers = content["maxUsers"].toInt(4);
        QString hostName = "Host";

        QString roomId = m_roomManager->createRoom(name, clientId, hostName, password);

        // Diffusion à tous les clients
        QList<Room*> publicRooms = m_roomManager->getPublicRooms();
        QJsonArray roomArray;
        for (Room* room : publicRooms) {
            roomArray.append(room->toJson());
        }
        QByteArray broadcastMsg = Protocol::createRoomListResponseMessage(roomArray);

        // UTILISER broadcastMessage au lieu de l'autre méthode
        broadcastMessage(broadcastMsg);
        break;
    }
    case MessageType::JOIN_ROOM: {
        QString roomId = content["roomId"].toString();
        QString userId = content["userId"].toString();
        QString userName = content["userName"].toString();
        QString password = content["password"].toString();

        bool ok = m_roomManager->joinRoom(roomId, userId, userName, password);
        if (ok) {
            Room* room = m_roomManager->getRoom(roomId);
            QByteArray response = Protocol::createRoomInfoMessage(room->toJson());
            sendMessageToClient(clientId, response);
        } else {
            QByteArray errorMsg = Protocol::createErrorMessage("Impossible de rejoindre la salle");
            sendMessageToClient(clientId, errorMsg);
        }
        break;
    }

    default:
        qWarning() << "[SERVER] Type de message non géré:" << static_cast<int>(type);
    }
}


QString DrumServer::getClientId(QTcpSocket *socket) const
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it.value() == socket)
        {
            return it.key();
        }
    }
    return QString();
}



// Méthodes utilitaires supplémentaires

int DrumServer::getClientCount() const
{
    return getConnectedClients().size();
}

bool DrumServer::hasClient(const QString &clientId) const
{
    return m_clients.contains(clientId) &&
           m_clients[clientId]->state() == QTcpSocket::ConnectedState;
}

void DrumServer::kickClient(const QString &clientId, const QString &reason)
{
    if (hasClient(clientId))
    {
        qDebug() << "Expulsion du client" << clientId << "raison:" << reason;

        // Optionnel : envoyer un message d'expulsion avant de déconnecter
        if (!reason.isEmpty())
        {
            QByteArray kickMessage = Protocol::createErrorMessage(
                QString("Vous avez été expulsé: %1").arg(reason));
            sendMessageToClient(clientId, kickMessage);
        }

        // Forcer la déconnexion
        QTcpSocket *socket = m_clients[clientId];
        socket->disconnectFromHost();
        if (!socket->waitForDisconnected(3000))
        {
            socket->abort();
        }
    }
}

QHostAddress DrumServer::getServerAddress() const
{
    if (m_server->isListening())
    {
        return m_server->serverAddress();
    }
    return QHostAddress();
}

quint16 DrumServer::getServerPort() const
{
    if (m_server->isListening())
    {
        return m_server->serverPort();
    }
    return 0;
}

void DrumServer::setMaxPendingConnections(int maxConnections)
{
    m_server->setMaxPendingConnections(maxConnections);
}
