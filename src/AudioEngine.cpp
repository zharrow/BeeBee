#include "AudioEngine.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>
#include <QCollator>

// Définition des constantes statiques
const QStringList AudioEngine::SUPPORTED_EXTENSIONS = {
    "*.wav", "*.mp3", "*.ogg", "*.flac", "*.aac", "*.m4a"
};

const QStringList AudioEngine::DEFAULT_NAMES = {
    "Kick", "Snare", "Hi-Hat", "Open Hat",
    "Crash", "Ride", "Tom 1", "Tom 2"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_volume(0.7f)
    , m_maxInstruments(DEFAULT_MAX_INSTRUMENTS) // 0 = illimité
{
    qDebug() << "AudioEngine initialisé avec système illimité";
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
    if (m_maxInstruments > 0) {
        qDebug() << "Limite d'instruments:" << m_maxInstruments;
    } else {
        qDebug() << "Aucune limite d'instruments";
    }

    bool allLoaded = loadSamplesFromDirectory(samplesDir);

    // Si aucun fichier n'a été chargé, utiliser les instruments par défaut
    if (m_instruments.isEmpty()) {
        qWarning() << "Aucun fichier audio trouvé - Utilisation d'instruments silencieux";
        setupDefaultInstruments();
        return false;
    }

    // Trier les instruments par nom pour un affichage cohérent
    sortInstrumentsByName();

    qDebug() << "Instruments chargés:" << m_instruments.size();
    emit instrumentCountChanged(m_instruments.size());
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
    loadAllAvailableSamples(dir);
    return !m_instruments.isEmpty();
}

void AudioEngine::loadAllAvailableSamples(const QDir& dir) {
    QFileInfoList files = dir.entryInfoList(SUPPORTED_EXTENSIONS, QDir::Files, QDir::Name);

    qDebug() << "Fichiers audio trouvés:" << files.size();

    int instrumentId = 0;
    int skippedFiles = 0;

    for (const QFileInfo& fileInfo : files) {
        // Vérifier la limite si elle est définie
        if (m_maxInstruments > 0 && instrumentId >= m_maxInstruments) {
            skippedFiles = files.size() - instrumentId;
            qWarning() << "Limite d'instruments atteinte (" << m_maxInstruments
                       << "), " << skippedFiles << "fichiers ignorés";
            emit maxInstrumentsReached(m_maxInstruments, files.size());
            break;
        }

        QString cleanName = cleanFileName(fileInfo.baseName());
        loadSample(instrumentId, fileInfo.filePath(), cleanName);
        instrumentId++;
    }

    if (skippedFiles > 0) {
        qDebug() << "Statistiques de chargement:";
        qDebug() << "  - Fichiers trouvés:" << files.size();
        qDebug() << "  - Fichiers chargés:" << instrumentId;
        qDebug() << "  - Fichiers ignorés:" << skippedFiles;
        qDebug() << "  - Limite actuelle:" << (m_maxInstruments > 0 ? QString::number(m_maxInstruments) : "Illimitée");
    } else {
        qDebug() << "Tous les" << instrumentId << "instruments chargés depuis" << files.size() << "fichiers trouvés";
    }
}

QString AudioEngine::cleanFileName(const QString& fileName) const {
    QString cleaned = fileName;

    // Enlever les underscores et les remplacer par des espaces
    cleaned = cleaned.replace('_', ' ');

    // Capitaliser la première lettre de chaque mot
    QStringList words = cleaned.split(' ', Qt::SkipEmptyParts);
    for (QString& word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
        }
    }

    return words.join(' ');
}

void AudioEngine::sortInstrumentsByName() {
    // Créer une liste temporaire pour trier
    QList<QPair<QString, InstrumentPlayer*>> sortedInstruments;

    for (auto it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        sortedInstruments.append({it.value()->name, it.value()});
    }

    // Trier par nom en utilisant QCollator pour un tri naturel
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(sortedInstruments.begin(), sortedInstruments.end(),
              [&collator](const QPair<QString, InstrumentPlayer*>& a,
                          const QPair<QString, InstrumentPlayer*>& b) {
                  return collator.compare(a.first, b.first) < 0;
              });

    // Reconstruire la map avec les nouveaux IDs
    m_instruments.clear();
    for (int i = 0; i < sortedInstruments.size(); ++i) {
        m_instruments[i] = sortedInstruments[i].second;
    }
}

void AudioEngine::setupDefaultInstruments() {
    for (int i = 0; i < DEFAULT_NAMES.size(); ++i) {
        createSilentInstrument(i, DEFAULT_NAMES[i]);
    }
    emit instrumentCountChanged(DEFAULT_NAMES.size());
    qDebug() << "Instruments par défaut configurés (mode silencieux)";
}

void AudioEngine::loadSample(int instrumentId, const QString& filePath, const QString& name) {
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
            this, [this, instrumentId, name](QMediaPlayer::Error error, const QString& errorString) {
                qWarning() << "Erreur instrument" << instrumentId << "(" << name << "):" << errorString;
                emit loadingError(QString("Erreur instrument %1 (%2): %3")
                                      .arg(instrumentId).arg(name).arg(errorString));
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

    qWarning() << "Instrument" << instrumentId << "(" << name << ") créé sans fichier audio (silencieux)";
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
        qDebug() << "Instrument" << instrumentId << "(" << instrument->name << ") en mode silencieux";
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
        QString cleanName = cleanFileName(fileInfo.baseName());
        loadSample(instrumentId, samplePath, cleanName);
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
    for (int i = 0; i < m_instruments.size(); ++i) {
        if (m_instruments.contains(i)) {
            names.append(getInstrumentName(i));
        }
    }
    return names;
}
