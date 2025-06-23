#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QColor>
#include <QMap>
#include <QScrollArea>
#include "Protocol.h"

class DrumGrid : public QWidget {
    Q_OBJECT

public:
    explicit DrumGrid(QWidget* parent = nullptr);

    // Configuration de la grille
    void setupGrid(int instruments = 8, int steps = 16);
    void setInstrumentCount(int instrumentCount);

    // Contrôle de lecture
    void setPlaying(bool playing);
    void setTempo(int bpm);
    void setCurrentStep(int step);

    // État de la grille
    bool isCellActive(int row, int col) const;
    void setCellActive(int row, int col, bool active, const QString& userId = QString());
    QJsonObject getGridState() const;
    void setGridState(const QJsonObject& state);

    // Configuration des instruments
    void setInstrumentNames(const QStringList& names);

    // Couleurs utilisateur
    void setUserColor(const QString& userId, const QColor& color);

    // Gestion des colonnes
    void addColumn();
    void removeColumn();
    int getStepCount() const { return m_steps; }
    void setStepCount(int steps);

    // Getters
    int getInstrumentCount() const { return m_instruments; }

signals:
    void cellClicked(int row, int col, bool active);
    void stepTriggered(int step, const QList<int>& activeInstruments);
    void stepCountChanged(int newCount);
    void instrumentCountChanged(int newCount);

private slots:
    void onCellClicked(int row, int column);
    void onStepTimer();

private:
    void updateCellAppearance(int row, int col);
    void highlightCurrentStep();
    void updateTableSize();
    void resizeGridForInstruments();

    QTableWidget* m_table;
    QScrollArea* m_scrollArea;
    QTimer* m_stepTimer;

    int m_instruments;
    int m_steps;
    int m_currentStep;
    int m_tempo; // BPM
    bool m_playing;

    static constexpr int MIN_STEPS = 8;
    static constexpr int MAX_STEPS = 64;
    static constexpr int DEFAULT_STEPS = 16;
    static constexpr int MIN_INSTRUMENTS = 1;
    static constexpr int MAX_INSTRUMENTS = 64;

    QStringList m_instrumentNames;
    QMap<QString, QColor> m_userColors;

    // État des cellules [row][col] -> {active, userId}
    QMap<QPair<int,int>, QPair<bool,QString>> m_cellStates;
};
