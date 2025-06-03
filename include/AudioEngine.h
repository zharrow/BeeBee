#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMap>
#include <QDir>
#include <memory>

/**
 * @brief Moteur audio pour la lecture des échantillons de batterie
 * Utilise QMediaPlayer pour une meilleure compatibilité avec différents formats audio
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

    // Configuration par défaut
    void setupDefaultInstruments();

signals:
    void sampleLoaded(int instrumentId, const QString& name);
    void loadingError(const QString& error);

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
    int mapFileNameToInstrument(const QString& fileName) const;
    bool loadSamplesFromDirectory(const QString& dirPath);
    void loadAvailableSamples(const QDir& dir);
    void ensureAllInstrumentsExist();

    QMap<int, InstrumentPlayer*> m_instruments; // Utiliser des pointeurs bruts pour éviter les problèmes de copie
    float m_volume;

    // Samples par défaut
    static const QStringList DEFAULT_SAMPLES;
    static const QStringList DEFAULT_NAMES;
    static constexpr int MAX_INSTRUMENTS = 8;
};
