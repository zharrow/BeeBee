#include "DrumServer.h"
#include "Protocol.h"
#include <QDataStream>
#include <QHostAddress>
#include <QDebug>

DrumServer::DrumServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_pingTimer(new QTimer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &DrumServer::onNewConnection);

    // Ping périodique pour détecter les déconnexions
    m_pingTimer->setInterval(30000); // 30 secondes
    connect(m_pingTimer, &QTimer::timeout, this, &DrumServer::onPingTimer);
}

DrumServer::~DrumServer() {
    stopListening();
}

bool DrumServer::startListening(quint16 port) {
    if (m_server->listen(QHostAddress::Any, port)) {
        m_pingTimer->start();
        qDebug() << "Serveur démarré sur le port" << port;
        return true;
    } else {
        qWarning() << "Impossible de démarrer le serveur:" << m_server->errorString();
        return false;
    }
}

void DrumServer::stopListening() {
    m_pingTimer->stop();

    // Fermer toutes les connexions clients
    for (auto* socket : m_clients) {
        if (socket->state() != QTcpSocket::UnconnectedState) {
            socket->disconnectFromHost();
            if (!socket->waitForDisconnected(3000)) {
                socket->abort();
            }
        }
    }

    m_clients.clear();
    m_clientBuffers.clear();

    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "Serveur arrêté";
    }
}

bool DrumServer::isListening() const {
    return m_server->isListening();
}

void DrumServer::broadcastMessage(const QByteArray& message) {
    int sentCount = 0;
    for (auto* socket : m_clients) {
        if (socket->state() == QTcpSocket::ConnectedState) {
            qint64 written = socket->write(message);
            if (written == message.size()) {
                sentCount++;
            } else {
                qWarning() << "Erreur d'envoi de message au client" << getClientId(socket);
            }
        }
    }
    qDebug() << "Message diffusé à" << sentCount << "clients";
}

void DrumServer::sendMessageToClient(const QString& clientId, const QByteArray& message) {
    if (m_clients.contains(clientId)) {
        QTcpSocket* socket = m_clients[clientId];
        if (socket->state() == QTcpSocket::ConnectedState) {
            qint64 written = socket->write(message);
            if (written != message.size()) {
                qWarning() << "Erreur d'envoi de message au client" << clientId;
            } else {
                qDebug() << "Message envoyé au client" << clientId;
            }
        } else {
            qWarning() << "Client" << clientId << "non connecté";
        }
    } else {
        qWarning() << "Client introuvable:" << clientId;
    }
}

QStringList DrumServer::getConnectedClients() const {
    QStringList connectedClients;
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value()->state() == QTcpSocket::ConnectedState) {
            connectedClients.append(it.key());
        }
    }
    return connectedClients;
}

void DrumServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();

        // Génération d'un ID client unique
        QString clientId = QString("Client_%1_%2")
                               .arg(socket->peerAddress().toString().replace(":", "_"))
                               .arg(socket->peerPort());

        // Éviter les doublons d'ID
        int counter = 1;
        QString originalId = clientId;
        while (m_clients.contains(clientId)) {
            clientId = QString("%1_%2").arg(originalId).arg(counter++);
        }

        m_clients[clientId] = socket;
        m_clientBuffers[socket] = QByteArray();

        // Connexion des signaux
        connect(socket, &QTcpSocket::readyRead, this, &DrumServer::onClientDataReceived);
        connect(socket, &QTcpSocket::disconnected, this, &DrumServer::onClientDisconnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                this, [this, socket](QAbstractSocket::SocketError error) {
                    QString clientId = getClientId(socket);
                    qWarning() << "Erreur socket pour client" << clientId << ":" << socket->errorString();
                    emit errorOccurred(QString("Erreur client %1: %2").arg(clientId, socket->errorString()));
                });

        qDebug() << "Nouveau client connecté:" << clientId
                 << "depuis" << socket->peerAddress().toString()
                 << ":" << socket->peerPort();

        emit clientConnected(clientId);
    }
}

void DrumServer::onClientDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QString clientId = getClientId(socket);
    if (!clientId.isEmpty()) {
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

    QString clientId = getClientId(socket);
    if (clientId.isEmpty()) {
        qWarning() << "Données reçues d'un client non identifié";
        return;
    }

    QByteArray& buffer = m_clientBuffers[socket];
    QByteArray newData = socket->readAll();
    buffer.append(newData);

    qDebug() << "Données reçues du client" << clientId << ":" << newData.size() << "bytes";

    // Traitement des messages complets
    while (buffer.size() >= 4) {
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 messageSize;
        stream >> messageSize;

        // Validation de la taille du message
        if (messageSize > 1024 * 1024) { // 1MB max
            qWarning() << "Message trop volumineux du client" << clientId << ":" << messageSize << "bytes";
            socket->disconnectFromHost();
            return;
        }

        if (buffer.size() >= 4 + messageSize) {
            QByteArray message = buffer.mid(0, 4 + messageSize);
            buffer.remove(0, 4 + messageSize);

            qDebug() << "Message complet reçu du client" << clientId << ":" << messageSize << "bytes";
            processClientMessage(socket, message);
        } else {
            // Message incomplet, attendre plus de données
            qDebug() << "Message incomplet du client" << clientId
                     << "- attendu:" << (4 + messageSize)
                     << "reçu:" << buffer.size();
            break;
        }
    }
}

void DrumServer::onPingTimer() {
    // Vérification de l'état des connexions
    QStringList disconnectedClients;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        QTcpSocket* socket = it.value();
        if (socket->state() != QTcpSocket::ConnectedState) {
            disconnectedClients.append(it.key());
        }
    }

    // Nettoyage des clients déconnectés
    for (const QString& clientId : disconnectedClients) {
        qDebug() << "Nettoyage du client déconnecté:" << clientId;
        QTcpSocket* socket = m_clients.take(clientId);
        m_clientBuffers.remove(socket);
        emit clientDisconnected(clientId);
        socket->deleteLater();
    }

    if (!disconnectedClients.isEmpty()) {
        qDebug() << "Clients connectés actuels:" << getConnectedClients().size();
    }
}

void DrumServer::processClientMessage(QTcpSocket* client, const QByteArray& data) {
    QString clientId = getClientId(client);
    if (clientId.isEmpty()) {
        qWarning() << "Impossible de traiter le message : client non identifié";
        return;
    }

    // Validation basique du message
    if (data.size() < 4) {
        qWarning() << "Message invalide du client" << clientId << ": trop court";
        return;
    }

    qDebug() << "Traitement du message du client" << clientId;
    emit messageReceived(data, clientId);
}

QString DrumServer::getClientId(QTcpSocket* socket) const {
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value() == socket) {
            return it.key();
        }
    }
    return QString();
}

// Méthodes utilitaires supplémentaires

int DrumServer::getClientCount() const {
    return getConnectedClients().size();
}

bool DrumServer::hasClient(const QString& clientId) const {
    return m_clients.contains(clientId) &&
           m_clients[clientId]->state() == QTcpSocket::ConnectedState;
}

void DrumServer::kickClient(const QString& clientId, const QString& reason) {
    if (hasClient(clientId)) {
        qDebug() << "Expulsion du client" << clientId << "raison:" << reason;

        // Optionnel : envoyer un message d'expulsion avant de déconnecter
        if (!reason.isEmpty()) {
            QByteArray kickMessage = Protocol::createErrorMessage(
                QString("Vous avez été expulsé: %1").arg(reason)
                );
            sendMessageToClient(clientId, kickMessage);
        }

        // Forcer la déconnexion
        QTcpSocket* socket = m_clients[clientId];
        socket->disconnectFromHost();
        if (!socket->waitForDisconnected(3000)) {
            socket->abort();
        }
    }
}

QHostAddress DrumServer::getServerAddress() const {
    if (m_server->isListening()) {
        return m_server->serverAddress();
    }
    return QHostAddress();
}

quint16 DrumServer::getServerPort() const {
    if (m_server->isListening()) {
        return m_server->serverPort();
    }
    return 0;
}

void DrumServer::setMaxPendingConnections(int maxConnections) {
    m_server->setMaxPendingConnections(maxConnections);
}
