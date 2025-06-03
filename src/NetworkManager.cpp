#include "NetworkManager.h"
#include "DrumServer.h"
#include "DrumClient.h"
#include <QDebug>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_client(nullptr)
    , m_isServer(false)
{
    qDebug() << "NetworkManager initialisé";
}

NetworkManager::~NetworkManager() {
    stopServer();
    disconnectFromServer();
}

bool NetworkManager::startServer(quint16 port) {
    if (m_server) {
        qDebug() << "Arrêt du serveur existant";
        stopServer();
    }

    // Déconnecter du serveur si on était client
    if (m_client) {
        disconnectFromServer();
    }

    m_server = new DrumServer(this);

    // Connexions des signaux du serveur
    connect(m_server, &DrumServer::clientConnected, this, &NetworkManager::clientConnected);
    connect(m_server, &DrumServer::clientDisconnected, this, &NetworkManager::clientDisconnected);
    connect(m_server, &DrumServer::messageReceived, this,
            [this](const QByteArray& msg, const QString& clientId) {
                Q_UNUSED(clientId)
                emit messageReceived(msg);
            });
    connect(m_server, &DrumServer::errorOccurred, this, &NetworkManager::errorOccurred);

    if (m_server->startListening(port)) {
        m_isServer = true;
        qDebug() << "Serveur démarré avec succès sur le port" << port;
        emit connectionEstablished();
        return true;
    } else {
        qWarning() << "Échec du démarrage du serveur";
        delete m_server;
        m_server = nullptr;
        return false;
    }
}

void NetworkManager::stopServer() {
    if (m_server) {
        qDebug() << "Arrêt du serveur";
        m_server->stopListening();
        delete m_server;
        m_server = nullptr;
        m_isServer = false;
        emit connectionLost();
    }
}

bool NetworkManager::isServerRunning() const {
    return m_server && m_server->isListening();
}

bool NetworkManager::connectToServer(const QString& host, quint16 port) {
    if (m_client) {
        qDebug() << "Déconnexion du serveur existant";
        disconnectFromServer();
    }

    // Arrêter le serveur si on en hébergeait un
    if (m_server) {
        stopServer();
    }

    m_client = new DrumClient(this);

    // Connexions des signaux du client
    connect(m_client, &DrumClient::connected, this, &NetworkManager::connectionEstablished);
    connect(m_client, &DrumClient::disconnected, this, &NetworkManager::connectionLost);
    connect(m_client, &DrumClient::messageReceived, this, &NetworkManager::messageReceived);
    connect(m_client, &DrumClient::errorOccurred, this, &NetworkManager::errorOccurred);

    return m_client->connectToServer(host, port);
}

void NetworkManager::disconnectFromServer() {
    if (m_client) {
        qDebug() << "Déconnexion du serveur";
        m_client->disconnectFromServer();
        delete m_client;
        m_client = nullptr;
    }
}

bool NetworkManager::isClientConnected() const {
    return m_client && m_client->isConnected();
}

void NetworkManager::sendMessage(const QByteArray& message) {
    if (m_client && m_client->isConnected()) {
        m_client->sendMessage(message);
    } else {
        qWarning() << "Tentative d'envoi de message sans connexion client";
    }
}

void NetworkManager::broadcastMessage(const QByteArray& message) {
    if (m_server && m_server->isListening()) {
        m_server->broadcastMessage(message);
    } else {
        qWarning() << "Tentative de diffusion sans serveur actif";
    }
}
