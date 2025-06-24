#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QStatusBar>
#include <QGroupBox>
#include <QSlider>
#include <QStackedWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QGridLayout>
#include "DrumGrid.h"
#include "AudioEngine.h"
#include "NetworkManager.h"
#include "Protocol.h"

// Forward declarations pour éviter les includes circulaires
class RoomManager;
class RoomListWidget;
class UserListWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
public slots:
    void startServer();

private slots:
    // Contrôles audio
    void onPlayPauseClicked();
    void onStopClicked();
    void onTempoChanged(int bpm);
    void onVolumeChanged(int volume);
    void onAddColumnClicked();
    void onRemoveColumnClicked();
    void onStepCountChanged(int newCount);

    // Grille
    void onGridCellClicked(int row, int col, bool active);
    void onStepTriggered(int step, const QList<int>& activeInstruments);

    // Gestion des salons
    void onCreateRoomRequested(const QString& name, const QString& password, int maxUsers);
    void onJoinRoomRequested(const QString& roomId, const QString& password);
    void onLeaveRoomRequested();
    void onRefreshRoomsRequested();
    void onKickUserRequested(const QString& userId);
    void onTransferHostRequested(const QString& userId);

    // Réseau
    void onStartServerClicked();
    void onConnectToServerClicked();
    void onDisconnectClicked();
    void onMessageReceived(const QByteArray& message);
    void onClientConnected(const QString& clientId);
    void onClientDisconnected(const QString& clientId);
    void onConnectionEstablished();
    void onConnectionLost();
    void onNetworkError(const QString& error);

private:
    void setupUI();
    void setupMenus();
    void onRoomStateReceived(const QJsonObject& roomInfo);
    void setupToolbar();
    void setupStatusBar();
    void createConnectionDialog();

    void onRoomListReceived(const QJsonArray& roomsArray);

    void updatePlayButton();
    void updateNetworkStatus();
    void updateRoomDisplay();
    void syncGridWithNetwork();
    void handleNetworkMessage(MessageType type, const QJsonObject& data);
    void switchToGameMode();
    void switchToLobbyMode();

    // Widgets principaux
    DrumGrid* m_drumGrid;
    AudioEngine* m_audioEngine;
    NetworkManager* m_networkManager;
    RoomManager* m_roomManager;

    // Interfaces
    RoomListWidget* m_roomListWidget;
    UserListWidget* m_userListWidget;
    QStackedWidget* m_stackedWidget;

    // Contrôles audio
    QPushButton* m_playPauseBtn;
    QPushButton* m_stopBtn;
    QSpinBox* m_tempoSpin;
    QSlider* m_volumeSlider;
    QLabel* m_tempoLabel;
    QLabel* m_volumeLabel;

    // Contrôles réseau
    QGroupBox* m_networkGroup;
    QPushButton* m_startServerBtn;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLineEdit* m_serverAddressEdit;
    QSpinBox* m_portSpin;
    QLineEdit* m_userNameEdit;
    QLabel* m_networkStatusLabel;

    // État
    bool m_isPlaying;
    QString m_currentUserId;
    QString m_currentUserName;
    QString m_currentRoomId;
    bool m_inGameMode;

    // Nouveaux widgets pour les contrôles de colonnes
    QPushButton* m_addColumnBtn;
    QPushButton* m_removeColumnBtn;
    QLabel* m_stepCountLabel;
};
