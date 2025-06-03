#pragma once
#include <QObject>
#include <QSoundEffect>
#include <QMap>
#include <QDir>

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
    
private slots:
    void onSoundFinished();
    
private:
    void loadSample(int instrumentId, const QString& filePath, const QString& name);
    
    QMap<int, QSoundEffect*> m_instruments;
    QMap<int, QString> m_instrumentNames;
    float m_volume;
    
    // Samples par défaut (chemins relatifs)
    static const QStringList DEFAULT_SAMPLES;
    static const QStringList DEFAULT_NAMES;
};