#include "GridState.h"

GridState::GridState(int instruments, int steps)
    : m_instruments(instruments), m_steps(steps)
{
    // Init toutes les cases à inactif
    for (int row = 0; row < m_instruments; ++row)
        for (int col = 0; col < m_steps; ++col)
            m_cells[qMakePair(row, col)] = GridCellState();
}

void GridState::setCell(int row, int col, bool active, const QString& userId)
{
    m_cells[qMakePair(row, col)] = {active, userId};
}

GridCellState GridState::getCell(int row, int col) const
{
    return m_cells.value(qMakePair(row, col), GridCellState());
}

void GridState::setInstrumentCount(int count)
{
    m_instruments = count;
    // (Optionnel : redimensionner, ajouter/supprimer des lignes)
}

int GridState::instrumentCount() const
{
    return m_instruments;
}

void GridState::setStepCount(int count)
{
    m_steps = count;
    // (Optionnel : redimensionner, ajouter/supprimer des colonnes)
}

int GridState::stepCount() const
{
    return m_steps;
}

QJsonObject GridState::toJson() const
{
    QJsonObject obj;
    obj["instruments"] = m_instruments;
    obj["steps"] = m_steps;
    QJsonArray cellsArray;
    for (auto it = m_cells.begin(); it != m_cells.end(); ++it) {
        QJsonObject cellObj;
        cellObj["row"] = it.key().first;
        cellObj["col"] = it.key().second;
        cellObj["active"] = it.value().active;
        cellObj["userId"] = it.value().userId;
        cellsArray.append(cellObj);
    }
    obj["cells"] = cellsArray;
    return obj;
}

void GridState::fromJson(const QJsonObject& obj)
{
    m_instruments = obj["instruments"].toInt(8);
    m_steps = obj["steps"].toInt(16);
    m_cells.clear();
    QJsonArray cellsArray = obj["cells"].toArray();
    for (const auto& cellValue : cellsArray) {
        QJsonObject cellObj = cellValue.toObject();
        int row = cellObj["row"].toInt();
        int col = cellObj["col"].toInt();
        bool active = cellObj["active"].toBool();
        QString userId = cellObj["userId"].toString();
        m_cells[qMakePair(row, col)] = {active, userId};
    }
}
