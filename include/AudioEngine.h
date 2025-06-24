#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMap>
#include <QDir>
#include <memory>

/**
 * @brief Moteur audio pour la lecture des échantillons de batterie
 * Version sans limite - charge tous les fichiers disponibles
 */
class AudioEngine : public QObject {
    Q_OBJECT

public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    // Configuration
    bool loadSamples(const QString& samplesPath = "samples/");
    void setVolume(float volume); // 0.0 - 1.0
    float getVolume() const { return m_volume; }

    // Lecture
    void playInstrument(int instrumentId);
    void playMultipleInstruments(const QList<int>& instruments);

    // Mapping des instruments
    void setInstrumentSample(int instrumentId, const QString& samplePath);
    QString getInstrumentName(int instrumentId) const;
    QStringList getInstrumentNames() const;
    int getInstrumentCount() const { return m_instruments.size(); }

    // Configuration par défaut
    void setupDefaultInstruments();

    // Nouveau : Configuration de la limite (0 = illimitée)
    void setMaxInstruments(int maxInstruments) { m_maxInstruments = maxInstruments; }
    int getMaxInstruments() const { return m_maxInstruments; }

signals:
    void sampleLoaded(int instrumentId, const QString& name);
    void loadingError(const QString& error);
    void instrumentCountChanged(int newCount);
    void maxInstrumentsReached(int maxCount, int totalFiles);

private:
    // Structure pour gérer un instrument
    struct InstrumentPlayer {
        std::unique_ptr<QMediaPlayer> player;
        std::unique_ptr<QAudioOutput> audioOutput;
        QString name;
        QString filePath;

        InstrumentPlayer() = default;
        ~InstrumentPlayer() = default;

        // Désactiver la copie
        InstrumentPlayer(const InstrumentPlayer&) = delete;
        InstrumentPlayer& operator=(const InstrumentPlayer&) = delete;

        // Activer le déplacement
        InstrumentPlayer(InstrumentPlayer&&) = default;
        InstrumentPlayer& operator=(InstrumentPlayer&&) = default;
    };

    void loadSample(int instrumentId, const QString& filePath, const QString& name);
    void createSilentInstrument(int instrumentId, const QString& name);
    QString findSamplesDirectory(const QString& basePath) const;
    bool loadSamplesFromDirectory(const QString& dirPath);
    void loadAllAvailableSamples(const QDir& dir);
    QString cleanFileName(const QString& fileName) const;
    void sortInstrumentsByName();

    QMap<int, InstrumentPlayer*> m_instruments;
    float m_volume;
    int m_maxInstruments; // 0 = illimité

    // Extensions audio supportées
    static const QStringList SUPPORTED_EXTENSIONS;
    static const QStringList DEFAULT_NAMES;
    static constexpr int MIN_INSTRUMENTS = 1;
    static constexpr int DEFAULT_MAX_INSTRUMENTS = 0; // Illimité par défaut
};
