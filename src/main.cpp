#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
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
