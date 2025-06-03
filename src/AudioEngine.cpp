#include "AudioEngine.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>

// Définition des constantes statiques
const QStringList AudioEngine::DEFAULT_SAMPLES = {
    "kick.wav", "snare.wav", "hihat.wav", "openhat.wav",
    "crash.wav", "ride.wav", "tom1.wav", "tom2.wav"
};

const QStringList AudioEngine::DEFAULT_NAMES = {
    "Kick", "Snare", "Hi-Hat", "Open Hat",
    "Crash", "Ride", "Tom 1", "Tom 2"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_volume(0.7f)
{
    qDebug() << "AudioEngine initialisé avec QMediaPlayer";
}

AudioEngine::~AudioEngine() {
    // Nettoyer les instruments
    for (auto* instrument : m_instruments) {
        delete instrument;
    }
    m_instruments.clear();
}

bool AudioEngine::loadSamples(const QString& samplesPath) {
    QString samplesDir = findSamplesDirectory(samplesPath);

    if (samplesDir.isEmpty()) {
        qWarning() << "Aucun dossier samples trouvé - Mode silencieux activé";
        setupDefaultInstruments();
        return false;
    }

    qDebug() << "Chargement des samples depuis:" << samplesDir;
    bool allLoaded = loadSamplesFromDirectory(samplesDir);

    // S'assurer que tous les instruments existent
    ensureAllInstrumentsExist();

    qDebug() << "Instruments chargés:" << m_instruments.keys();
    return allLoaded;
}

QString AudioEngine::findSamplesDirectory(const QString& basePath) const {
    QStringList searchPaths = {
        basePath,
        QCoreApplication::applicationDirPath() + "/" + basePath,
        QCoreApplication::applicationDirPath() + "/../" + basePath,
        QDir::currentPath() + "/" + basePath
    };

    for (const QString& path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            return dir.absolutePath();
        }
    }

    return QString();
}

bool AudioEngine::loadSamplesFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    bool allLoaded = true;

    // Essayer de charger les fichiers par défaut
    for (int i = 0; i < DEFAULT_SAMPLES.size() && i < MAX_INSTRUMENTS; ++i) {
        QString filePath = dir.filePath(DEFAULT_SAMPLES[i]);

        if (QFile::exists(filePath)) {
            loadSample(i, filePath, DEFAULT_NAMES[i]);
        } else {
            qWarning() << "Fichier non trouvé:" << filePath;
            allLoaded = false;
        }
    }

    // Si certains manquent, charger ce qui existe
    if (!allLoaded) {
        loadAvailableSamples(dir);
    }

    return allLoaded;
}

void AudioEngine::loadAvailableSamples(const QDir& dir) {
    QStringList filters = {"*.wav", "*.mp3", "*.ogg"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& fileInfo : files) {
        int instrumentId = mapFileNameToInstrument(fileInfo.baseName());

        if (instrumentId >= 0 && instrumentId < MAX_INSTRUMENTS &&
            !m_instruments.contains(instrumentId)) {
            loadSample(instrumentId, fileInfo.filePath(), fileInfo.baseName());
        }
    }
}

int AudioEngine::mapFileNameToInstrument(const QString& fileName) const {
    QString lowerName = fileName.toLower();

    if (lowerName.contains("kick") || lowerName.contains("bd")) return 0;
    if (lowerName.contains("snare") || lowerName.contains("sd")) return 1;
    if (lowerName.contains("hihat") || lowerName.contains("hh")) {
        return lowerName.contains("open") ? 3 : 2;
    }
    if (lowerName.contains("hat")) {
        return lowerName.contains("open") ? 3 : 2;
    }
    if (lowerName.contains("crash")) return 4;
    if (lowerName.contains("ride")) return 5;
    if (lowerName.contains("tom")) {
        if (lowerName.contains("1") || lowerName.contains("high")) return 6;
        if (lowerName.contains("2") || lowerName.contains("mid")) return 7;
        return 6; // Par défaut tom1
    }

    return -1;
}

void AudioEngine::ensureAllInstrumentsExist() {
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        if (!m_instruments.contains(i)) {
            createSilentInstrument(i, DEFAULT_NAMES[i]);
        }
    }
}

void AudioEngine::setupDefaultInstruments() {
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        createSilentInstrument(i, DEFAULT_NAMES[i]);
    }
    qDebug() << "Instruments par défaut configurés (mode silencieux)";
}

void AudioEngine::loadSample(int instrumentId, const QString& filePath, const QString& name) {
    if (instrumentId < 0 || instrumentId >= MAX_INSTRUMENTS) {
        qWarning() << "ID d'instrument invalide:" << instrumentId;
        return;
    }

    // Supprimer l'ancien instrument s'il existe
    if (m_instruments.contains(instrumentId)) {
        delete m_instruments[instrumentId];
    }

    auto* instrument = new InstrumentPlayer();
    instrument->player = std::make_unique<QMediaPlayer>(this);
    instrument->audioOutput = std::make_unique<QAudioOutput>(this);
    instrument->name = name;
    instrument->filePath = filePath;

    // Configuration du player
    instrument->player->setAudioOutput(instrument->audioOutput.get());
    instrument->player->setSource(QUrl::fromLocalFile(filePath));
    instrument->audioOutput->setVolume(m_volume);

    // Connexion pour gérer les erreurs
    connect(instrument->player.get(), &QMediaPlayer::errorOccurred,
            this, [this, instrumentId](QMediaPlayer::Error error, const QString& errorString) {
                qWarning() << "Erreur instrument" << instrumentId << ":" << errorString;
                emit loadingError(QString("Erreur instrument %1: %2").arg(instrumentId).arg(errorString));
            });

    m_instruments[instrumentId] = instrument;

    qDebug() << "Sample chargé:" << name << "pour l'instrument" << instrumentId;
    emit sampleLoaded(instrumentId, name);
}

void AudioEngine::createSilentInstrument(int instrumentId, const QString& name) {
    // Supprimer l'ancien instrument s'il existe
    if (m_instruments.contains(instrumentId)) {
        delete m_instruments[instrumentId];
    }

    auto* instrument = new InstrumentPlayer();
    instrument->player = std::make_unique<QMediaPlayer>(this);
    instrument->audioOutput = std::make_unique<QAudioOutput>(this);
    instrument->name = name;

    instrument->player->setAudioOutput(instrument->audioOutput.get());
    instrument->audioOutput->setVolume(m_volume);

    m_instruments[instrumentId] = instrument;

    qWarning() << "Instrument" << instrumentId << "créé sans fichier audio (silencieux)";
    emit sampleLoaded(instrumentId, name);
}

void AudioEngine::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);

    // Appliquer le volume à tous les instruments
    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        InstrumentPlayer* instrument = it.value();
        if (instrument && instrument->audioOutput) {
            instrument->audioOutput->setVolume(m_volume);
        }
    }
}

void AudioEngine::playInstrument(int instrumentId) {
    auto it = m_instruments.find(instrumentId);
    if (it == m_instruments.end()) {
        qWarning() << "Instrument non trouvé:" << instrumentId;
        return;
    }

    InstrumentPlayer* instrument = it.value();
    if (!instrument || !instrument->player) {
        qWarning() << "Instrument" << instrumentId << "non initialisé";
        return;
    }

    // Vérifier si un fichier est chargé
    if (instrument->filePath.isEmpty()) {
        qDebug() << "Instrument" << instrumentId << "en mode silencieux";
        return;
    }

    // Redémarrer la lecture depuis le début
    instrument->player->setPosition(0);
    instrument->player->play();

    qDebug() << "Lecture instrument" << instrumentId << ":" << instrument->name;
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
    auto it = m_instruments.find(instrumentId);
    if (it != m_instruments.end()) {
        InstrumentPlayer* instrument = it.value();
        if (instrument) {
            return instrument->name;
        }
    }
    return QString("Instrument %1").arg(instrumentId);
}

QStringList AudioEngine::getInstrumentNames() const {
    QStringList names;
    for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
        names.append(getInstrumentName(i));
    }
    return names;
}
