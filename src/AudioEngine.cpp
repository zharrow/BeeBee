#include "AudioEngine.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>

// Définition des constantes statiques
const QStringList AudioEngine::DEFAULT_SAMPLES = {
    "samples/kick.wav",
    "samples/snare.wav",
    "samples/hihat.wav",
    "samples/openhat.wav",
    "samples/crash.wav",
    "samples/ride.wav",
    "samples/tom1.wav",
    "samples/tom2.wav"
};

const QStringList AudioEngine::DEFAULT_NAMES = {
    "Kick",
    "Snare",
    "Hi-Hat",
    "Open Hat",
    "Crash",
    "Ride",
    "Tom 1",
    "Tom 2"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_volume(0.7f)
{
    qDebug() << "AudioEngine initialisé";
}

AudioEngine::~AudioEngine() {
    // Nettoyer les effets sonores
    for (auto* effect : m_instruments) {
        if (effect) {
            effect->stop();
            delete effect;
        }
    }
    m_instruments.clear();
}

bool AudioEngine::loadSamples(const QString& samplesPath) {
    QDir samplesDir(samplesPath);
    bool allLoaded = true;

    if (!samplesDir.exists()) {
        qWarning() << "Dossier samples non trouvé:" << samplesPath;
        qWarning() << "Chemin absolu testé:" << samplesDir.absolutePath();
        qWarning() << "Utilisation du mode silencieux";
        setupDefaultInstruments();
        return false;
    }

    qDebug() << "Chargement des samples depuis:" << samplesDir.absolutePath();

    // Méthode 1: Essayer de charger les fichiers par défaut dans l'ordre
    for (int i = 0; i < DEFAULT_SAMPLES.size() && i < 8; ++i) {
        QString fileName = DEFAULT_SAMPLES[i].mid(8); // Enlever "samples/"
        QString filePath = samplesDir.filePath(fileName);

        if (QFile::exists(filePath)) {
            loadSample(i, filePath, DEFAULT_NAMES[i]);
            qDebug() << "Sample chargé:" << fileName << "-> instrument" << i;
        } else {
            qWarning() << "Fichier non trouvé:" << filePath;
            // Créer un instrument vide pour cet ID
            QSoundEffect* effect = new QSoundEffect(this);
            effect->setVolume(m_volume);
            m_instruments[i] = effect;
            m_instrumentNames[i] = DEFAULT_NAMES[i];
            allLoaded = false;
        }
    }

    // Méthode 2: Si certains fichiers manquent, essayer de charger ce qui existe
    if (!allLoaded) {
        QStringList filters;
        filters << "*.wav" << "*.mp3" << "*.ogg";
        QFileInfoList files = samplesDir.entryInfoList(filters, QDir::Files);

        qDebug() << "Fichiers trouvés dans samples:" << files.size();

        for (const QFileInfo& fileInfo : files) {
            QString fileName = fileInfo.baseName().toLower();
            int instrumentId = -1;

            // Mapping basé sur le nom du fichier
            if (fileName.contains("kick") || fileName.contains("bd")) instrumentId = 0;
            else if (fileName.contains("snare") || fileName.contains("sd")) instrumentId = 1;
            else if (fileName.contains("hihat") || fileName.contains("hh")) {
                if (fileName.contains("open")) instrumentId = 3;
                else instrumentId = 2;
            }
            else if (fileName.contains("hat")) {
                if (fileName.contains("open")) instrumentId = 3;
                else instrumentId = 2;
            }
            else if (fileName.contains("crash")) instrumentId = 4;
            else if (fileName.contains("ride")) instrumentId = 5;
            else if (fileName.contains("tom")) {
                if (fileName.contains("1") || fileName.contains("high")) instrumentId = 6;
                else if (fileName.contains("2") || fileName.contains("mid")) instrumentId = 7;
                else instrumentId = 6; // Par défaut tom1
            }

            if (instrumentId >= 0 && instrumentId < 8 && !m_instruments.contains(instrumentId)) {
                loadSample(instrumentId, fileInfo.filePath(), fileInfo.baseName());
                qDebug() << "Mapping automatique:" << fileInfo.fileName() << "-> instrument" << instrumentId;
            }
        }
    }

    // S'assurer que tous les instruments 0-7 existent
    for (int i = 0; i < 8; ++i) {
        if (!m_instruments.contains(i)) {
            qWarning() << "Instrument" << i << "manquant, création d'un instrument silencieux";
            QSoundEffect* effect = new QSoundEffect(this);
            effect->setVolume(m_volume);
            m_instruments[i] = effect;
            m_instrumentNames[i] = DEFAULT_NAMES[i];
            allLoaded = false;
        }
    }

    qDebug() << "Instruments chargés:" << m_instruments.keys();
    return allLoaded;
}

void AudioEngine::setupDefaultInstruments() {
    // Configuration par défaut sans fichiers audio
    for (int i = 0; i < 8; ++i) {
        m_instrumentNames[i] = DEFAULT_NAMES[i];

        // Créer un QSoundEffect vide (sera muet)
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setVolume(m_volume);
        connect(effect, &QSoundEffect::playingChanged, this, &AudioEngine::onSoundFinished);
        m_instruments[i] = effect;

        emit sampleLoaded(i, DEFAULT_NAMES[i]);
    }

    qDebug() << "Instruments par défaut configurés (mode silencieux)";
}

void AudioEngine::loadSample(int instrumentId, const QString& filePath, const QString& name) {
    if (instrumentId < 0 || instrumentId >= 8) {
        qWarning() << "ID d'instrument invalide:" << instrumentId;
        return;
    }

    // Supprimer l'ancien effet s'il existe
    if (m_instruments.contains(instrumentId)) {
        QSoundEffect* oldEffect = m_instruments[instrumentId];
        oldEffect->stop();
        delete oldEffect;
    }

    QSoundEffect* effect = new QSoundEffect(this);
    QUrl url = QUrl::fromLocalFile(filePath);
    effect->setSource(url);
    effect->setVolume(m_volume);

    connect(effect, &QSoundEffect::playingChanged, this, &AudioEngine::onSoundFinished);

    m_instruments[instrumentId] = effect;
    m_instrumentNames[instrumentId] = name;

    qDebug() << "Sample chargé:" << name << "pour l'instrument" << instrumentId << "depuis" << filePath;
    emit sampleLoaded(instrumentId, name);
}

void AudioEngine::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);

    // Appliquer le volume à tous les instruments
    for (auto* effect : m_instruments) {
        if (effect) {
            effect->setVolume(m_volume);
        }
    }
}

void AudioEngine::playInstrument(int instrumentId) {
    if (!m_instruments.contains(instrumentId)) {
        qWarning() << "Instrument non trouvé:" << instrumentId;
        qWarning() << "Instruments disponibles:" << m_instruments.keys();
        return;
    }

    QSoundEffect* effect = m_instruments[instrumentId];
    if (effect) {
        if (!effect->source().isValid()) {
            qDebug() << "Instrument" << instrumentId << "n'a pas de source audio (mode silencieux)";
            return;
        }

        // Arrêter et redémarrer pour permettre la répétition rapide
        if (effect->isPlaying()) {
            effect->stop();
        }
        effect->play();
        qDebug() << "Lecture instrument" << instrumentId << ":" << m_instrumentNames[instrumentId];
    }
}

void AudioEngine::playMultipleInstruments(const QList<int>& instruments) {
    for (int instrumentId : instruments) {
        playInstrument(instrumentId);
    }
}

void AudioEngine::setInstrumentSample(int instrumentId, const QString& samplePath) {
    if (QFile::exists(samplePath)) {
        QFileInfo fileInfo(samplePath);
        loadSample(instrumentId, samplePath, fileInfo.baseName());
    } else {
        qWarning() << "Fichier sample non trouvé:" << samplePath;
        emit loadingError(QString("Fichier non trouvé: %1").arg(samplePath));
    }
}

QString AudioEngine::getInstrumentName(int instrumentId) const {
    return m_instrumentNames.value(instrumentId, QString("Instrument %1").arg(instrumentId));
}

QStringList AudioEngine::getInstrumentNames() const {
    QStringList names;
    for (int i = 0; i < 8; ++i) {
        names.append(getInstrumentName(i));
    }
    return names;
}

void AudioEngine::onSoundFinished() {
    // Slot pour gérer la fin de lecture des sons si nécessaire
    // Actuellement pas d'action spécifique requise
}
