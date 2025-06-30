#pragma once

#include <QMap>
#include <QPair>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

struct GridCellState {
    bool active = false;
    QString userId;
};

class GridState
{
public:
    GridState(int instruments = 8, int steps = 16);

    void setCell(int row, int col, bool active, const QString& userId = QString());
    GridCellState getCell(int row, int col) const;

    void setInstrumentCount(int count);
    int instrumentCount() const;

    void setStepCount(int count);
    int stepCount() const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

private:
    int m_instruments;
    int m_steps;
    // Clé = (ligne, colonne), Valeur = état de la cellule
    QMap<QPair<int, int>, GridCellState> m_cells;
};
