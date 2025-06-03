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


int main(int argc, char *argv[]) {


    // Dans main(), avant de créer MainWindow :
    qDebug() << "=== Test Audio ===";
  //  qDebug() << "Supported audio codecs:" << QMediaFormat().supportedAudioCodecs(QMediaFormat::Decode);

    auto devices = QMediaDevices::audioOutputs();
    qDebug() << "Audio output devices:" << devices.size();
    for (const QAudioDevice &device : devices) {
        qDebug() << " -" << device.description();
    }


   // connect(player, &QMediaPlayer::positionChanged, this, &MediaExample::positionChanged);


    // Test direct
    QSoundEffect testSound;
    testSound.setSource(QUrl::fromLocalFile("samples/kick.wav"));
    if (testSound.status() == QSoundEffect::Error) {
        qWarning() << "Erreur de chargement du fichier de test";
    }
    QApplication app(argc, argv);

    // Configuration de l'application
    app.setApplicationName("DrumBox Multiplayer");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("Student Project");
    app.setOrganizationDomain("drumbox.local");

// Style moderne pour Windows
#ifdef Q_OS_WIN
    app.setStyle(QStyleFactory::create("Fusion"));
#endif

    // Configuration des ressources
    QDir::setCurrent(QApplication::applicationDirPath());

    // Créer et afficher la fenêtre principale
    MainWindow window;
    window.show();

    return app.exec();
}
