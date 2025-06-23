#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>
#include "MainWindow.h"

#include <QSoundEffect>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Métadonnées de l'application
    app.setApplicationName("BeeBee");
    // app.setApplicationDisplayName("BeeBee - Collab' Drum Machine");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("BeTeam");

    // Icône par défaut pour toutes les fenêtres
    app.setWindowIcon(QIcon(":../resources/icons/logo.png"));

// Style moderne pour Windows
#ifdef Q_OS_WIN
    app.setStyle(QStyleFactory::create("Fusion"));
#endif

    // Configuration des ressources
    QDir::setCurrent(QApplication::applicationDirPath());

    // Tests audio (optionnels, peuvent être commentés en production)
    qDebug() << "=== Test Audio ===";

    auto devices = QMediaDevices::audioOutputs();
    qDebug() << "Audio output devices:" << devices.size();
    for (const QAudioDevice &device : devices) {
        qDebug() << " -" << device.description();
    }

    // Test de chargement d'un fichier audio
    QSoundEffect testSound;
    testSound.setSource(QUrl::fromLocalFile("samples/kick.wav"));
    if (testSound.status() == QSoundEffect::Error) {
        qWarning() << "Erreur de chargement du fichier de test";
    } else {
        qDebug() << "Test audio: fichier chargé avec succès";
    }

    // Créer et afficher la fenêtre principale
    MainWindow window;
    window.show();

    // Lancer la boucle d'événements
    return app.exec();
}
