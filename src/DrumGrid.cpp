#include "DrumGrid.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QJsonArray>
#include <QScrollBar>

DrumGrid::DrumGrid(QWidget* parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_scrollArea(new QScrollArea(this))
    , m_stepTimer(new QTimer(this))
    , m_instruments(8)
    , m_steps(DEFAULT_STEPS)
    , m_currentStep(0)
    , m_tempo(120)
    , m_playing(false)
{
    setupGrid();

    // Configuration du scroll area
    m_scrollArea->setWidget(m_table);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scrollArea);

    // Configuration du timer
    connect(m_stepTimer, &QTimer::timeout, this, &DrumGrid::onStepTimer);

    // Configuration de la table
    connect(m_table, &QTableWidget::cellClicked, this, &DrumGrid::onCellClicked);

    // Configuration des instruments par défaut
    QStringList defaultNames = {"Kick", "Snare", "Hi-Hat", "Open Hat",
                                "Crash", "Ride", "Tom 1", "Tom 2"};
    setInstrumentNames(defaultNames);
}

void DrumGrid::setupGrid(int instruments, int steps) {
    m_instruments = instruments;
    m_steps = qBound(MIN_STEPS, steps, MAX_STEPS);
    m_cellStates.clear();

    m_table->setRowCount(instruments);
    m_table->setColumnCount(m_steps);

    // Configuration de l'apparence
    m_table->horizontalHeader()->setDefaultSectionSize(40);
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);

    // Headers des colonnes (numéros des steps)
    for (int col = 0; col < m_steps; ++col) {
        m_table->setHorizontalHeaderItem(col, new QTableWidgetItem(QString::number(col + 1)));
    }

    // Initialisation des cellules
    for (int row = 0; row < instruments; ++row) {
        for (int col = 0; col < m_steps; ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setFlags(Qt::ItemIsEnabled);
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, col, item);
            updateCellAppearance(row, col);
        }
    }

    updateTableSize();
}

void DrumGrid::addColumn() {
    if (m_steps >= MAX_STEPS) return;

    m_steps++;
    m_table->setColumnCount(m_steps);

    // Ajouter l'en-tête de la nouvelle colonne
    m_table->setHorizontalHeaderItem(m_steps - 1, new QTableWidgetItem(QString::number(m_steps)));

    // Initialiser les cellules de la nouvelle colonne
    for (int row = 0; row < m_instruments; ++row) {
        QTableWidgetItem* item = new QTableWidgetItem("");
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, m_steps - 1, item);
        updateCellAppearance(row, m_steps - 1);
    }

    updateTableSize();
    emit stepCountChanged(m_steps);
}

void DrumGrid::removeColumn() {
    if (m_steps <= MIN_STEPS) return;

    // Supprimer les états des cellules de la dernière colonne
    for (int row = 0; row < m_instruments; ++row) {
        QPair<int, int> key(row, m_steps - 1);
        m_cellStates.remove(key);
    }

    m_steps--;
    m_table->setColumnCount(m_steps);

    // Si le step actuel est au-delà de la nouvelle limite, le réinitialiser
    if (m_currentStep >= m_steps) {
        m_currentStep = 0;
    }

    updateTableSize();
    emit stepCountChanged(m_steps);
}

void DrumGrid::setStepCount(int steps) {
    int newSteps = qBound(MIN_STEPS, steps, MAX_STEPS);
    if (newSteps == m_steps) return;

    if (newSteps > m_steps) {
        // Ajouter des colonnes
        while (m_steps < newSteps) {
            addColumn();
        }
    } else {
        // Enlever des colonnes
        while (m_steps > newSteps) {
            removeColumn();
        }
    }
}

void DrumGrid::updateTableSize() {
    // Calculer la taille optimale de la table
    int tableWidth = m_table->verticalHeader()->width() +
                     m_table->horizontalHeader()->length() +
                     m_table->frameWidth() * 2;

    int tableHeight = m_table->horizontalHeader()->height() +
                      m_table->verticalHeader()->length() +
                      m_table->frameWidth() * 2;

    m_table->setFixedSize(tableWidth, tableHeight);

    // Ajuster la taille minimale du widget pour afficher au moins 8 colonnes
    int minWidth = m_table->verticalHeader()->width() +
                   (40 * qMin(8, m_steps)) +
                   m_table->frameWidth() * 2 + 20; // +20 pour la scrollbar

    setMinimumWidth(minWidth);
}

void DrumGrid::setInstrumentNames(const QStringList& names) {
    m_instrumentNames = names;

    // Mise à jour des headers verticaux
    for (int i = 0; i < qMin(names.size(), m_instruments); ++i) {
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(names[i]));
    }
}

void DrumGrid::setPlaying(bool playing) {
    if (m_playing == playing) return;

    m_playing = playing;

    if (playing) {
        int interval = 60000 / (m_tempo * 4); // 16th notes
        m_stepTimer->start(interval);
    } else {
        m_stepTimer->stop();
    }
}

void DrumGrid::setTempo(int bpm) {
    m_tempo = bpm;
    if (m_playing) {
        int interval = 60000 / (bpm * 4);
        m_stepTimer->setInterval(interval);
    }
}

void DrumGrid::setCurrentStep(int step) {
    m_currentStep = step % m_steps;

    // Mise à jour de l'affichage
    highlightCurrentStep();

    // Auto-scroll pour suivre la lecture
    if (m_playing) {
        int columnX = m_table->columnViewportPosition(m_currentStep);
        int columnWidth = m_table->columnWidth(m_currentStep);
        QScrollBar* hScrollBar = m_scrollArea->horizontalScrollBar();

        if (columnX < 0) {
            // La colonne est à gauche de la zone visible
            hScrollBar->setValue(hScrollBar->value() + columnX);
        } else if (columnX + columnWidth > m_scrollArea->viewport()->width()) {
            // La colonne est à droite de la zone visible
            hScrollBar->setValue(hScrollBar->value() + columnX + columnWidth - m_scrollArea->viewport()->width());
        }
    }

    // Déclenchement des instruments actifs
    QList<int> activeInstruments;
    for (int row = 0; row < m_instruments; ++row) {
        if (isCellActive(row, m_currentStep)) {
            activeInstruments.append(row);
        }
    }

    if (!activeInstruments.isEmpty()) {
        emit stepTriggered(m_currentStep, activeInstruments);
    }
}

bool DrumGrid::isCellActive(int row, int col) const {
    QPair<int,int> key(row, col);
    return m_cellStates.value(key, {false, QString()}).first;
}

void DrumGrid::setCellActive(int row, int col, bool active, const QString& userId) {
    if (col >= m_steps) return; // Protection contre les indices invalides

    QPair<int,int> key(row, col);
    m_cellStates[key] = {active, userId};
    updateCellAppearance(row, col);
}

void DrumGrid::setUserColor(const QString& userId, const QColor& color) {
    m_userColors[userId] = color;

    // Mise à jour de toutes les cellules de cet utilisateur
    for (auto it = m_cellStates.begin(); it != m_cellStates.end(); ++it) {
        if (it.value().second == userId) {
            updateCellAppearance(it.key().first, it.key().second);
        }
    }
}

QJsonObject DrumGrid::getGridState() const {
    QJsonObject state;
    QJsonArray cells;

    for (auto it = m_cellStates.begin(); it != m_cellStates.end(); ++it) {
        if (it.value().first) { // Seulement les cellules actives
            QJsonObject cell;
            cell["row"] = it.key().first;
            cell["col"] = it.key().second;
            cell["active"] = true;
            cell["userId"] = it.value().second;
            cells.append(cell);
        }
    }

    state["cells"] = cells;
    state["tempo"] = m_tempo;
    state["playing"] = m_playing;
    state["currentStep"] = m_currentStep;
    state["stepCount"] = m_steps;

    return state;
}

void DrumGrid::setGridState(const QJsonObject& state) {
    // Réinitialisation
    m_cellStates.clear();

    // Charger le nombre de steps si présent
    if (state.contains("stepCount")) {
        setStepCount(state["stepCount"].toInt());
    }

    // Chargement des cellules
    QJsonArray cells = state["cells"].toArray();
    for (const auto& cellValue : cells) {
        QJsonObject cell = cellValue.toObject();
        int row = cell["row"].toInt();
        int col = cell["col"].toInt();
        bool active = cell["active"].toBool();
        QString userId = cell["userId"].toString();

        setCellActive(row, col, active, userId);
    }

    // Mise à jour des paramètres
    if (state.contains("tempo")) {
        setTempo(state["tempo"].toInt());
    }

    if (state.contains("currentStep")) {
        setCurrentStep(state["currentStep"].toInt());
    }
}

void DrumGrid::onCellClicked(int row, int column) {
    bool currentState = isCellActive(row, column);
    bool newState = !currentState;

    setCellActive(row, column, newState, QString()); // ID utilisateur sera défini par l'appelant
    emit cellClicked(row, column, newState);
}

void DrumGrid::onStepTimer() {
    setCurrentStep(m_currentStep + 1);
}

void DrumGrid::updateCellAppearance(int row, int col) {
    QTableWidgetItem* item = m_table->item(row, col);
    if (!item) return;

    QPair<int,int> key(row, col);
    auto cellData = m_cellStates.value(key, {false, QString()});
    bool active = cellData.first;
    QString userId = cellData.second;

    if (active) {
        QColor color = m_userColors.value(userId, QColor(100, 150, 255));
        item->setBackground(QBrush(color));
        item->setText("●");
    } else {
        // Coloration alternée pour mieux visualiser les mesures
        QColor bgColor = (col % 4 == 0) ? QColor(220, 220, 220) : QColor(240, 240, 240);
        item->setBackground(QBrush(bgColor));
        item->setText("");
    }
}

void DrumGrid::highlightCurrentStep() {
    // Mise à jour de l'en-tête de colonne pour montrer le step actuel
    for (int col = 0; col < m_steps; ++col) {
        QTableWidgetItem* header = m_table->horizontalHeaderItem(col);
        if (header) {
            if (col == m_currentStep && m_playing) {
                header->setBackground(QBrush(QColor(255, 100, 100)));
            } else {
                header->setBackground(QBrush());
            }
        }
    }
}
