#include "DrumGrid.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QJsonArray>

DrumGrid::DrumGrid(QWidget* parent)
    : QWidget(parent)
    , m_table(new QTableWidget(this))
    , m_stepTimer(new QTimer(this))
    , m_instruments(8)
    , m_steps(16)
    , m_currentStep(0)
    , m_tempo(120)
    , m_playing(false)
{
    setupGrid();
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_table);
    
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
    m_steps = steps;
    m_cellStates.clear();

    m_table->setRowCount(instruments);
    m_table->setColumnCount(steps);

    // Style moderne pour la table
    m_table->setStyleSheet(R"(
        QTableWidget {
            background: rgba(255, 255, 255, 0.02);
            border: 2px solid rgba(255, 255, 255, 0.1);
            border-radius: 10px;
            gridline-color: rgba(255, 255, 255, 0.05);
        }

        QHeaderView::section {
            background: rgba(255, 255, 255, 0.05);
            color: #e2e8f0;
            border: none;
            padding: 8px;
            font-weight: bold;
        }

        QTableWidget::item {
            background: rgba(255, 255, 255, 0.03);
            border: 1px solid rgba(255, 255, 255, 0.05);
        }

        QTableWidget::item:hover {
            background: rgba(255, 255, 255, 0.08);
            border: 1px solid rgba(59, 130, 246, 0.3);
        }
    )");

    // Configuration de l'apparence
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);

    // Headers des colonnes (numéros des steps)
    for (int col = 0; col < steps; ++col) {
        m_table->setHorizontalHeaderItem(col, new QTableWidgetItem(QString::number(col + 1)));
    }

    // Initialisation des cellules
    for (int row = 0; row < instruments; ++row) {
        for (int col = 0; col < steps; ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setFlags(Qt::ItemIsEnabled);
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, col, item);
            updateCellAppearance(row, col);
        }
    }
}

void DrumGrid::highlightCurrentStep() {
    // Mise à jour de l'en-tête de colonne pour montrer le step actuel
    for (int col = 0; col < m_steps; ++col) {
        QTableWidgetItem* header = m_table->horizontalHeaderItem(col);
        if (header) {
            if (col == m_currentStep && m_playing) {
                header->setBackground(QBrush(QColor(239, 68, 68)));
                header->setForeground(Qt::white);

                // Animer les cellules actives du step courant
                for (int row = 0; row < m_instruments; ++row) {
                    if (isCellActive(row, col)) {
                        QTableWidgetItem* item = m_table->item(row, col);
                        if (item) {
                            item->setFont(QFont("Arial", 20, QFont::Bold));
                        }
                    }
                }
            } else {
                header->setBackground(QBrush());
                header->setForeground(QColor(226, 232, 240));

                // Restaurer la taille normale
                for (int row = 0; row < m_instruments; ++row) {
                    QTableWidgetItem* item = m_table->item(row, col);
                    if (item && !item->text().isEmpty()) {
                        item->setFont(QFont("Arial", 16, QFont::Bold));
                    }
                }
            }
        }
    }
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
    int oldStep = m_currentStep;
    m_currentStep = step % m_steps;
    
    // Mise à jour de l'affichage
    highlightCurrentStep();
    
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
    
    return state;
}

void DrumGrid::setGridState(const QJsonObject& state) {
    // Réinitialisation
    m_cellStates.clear();
    
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
        // Ajouter un effet de lueur pour les cellules actives
        QString colorStyle = QString(R"(
            background: %1;
            border: 2px solid %2;
            border-radius: 4px;
        )").arg(color.name(), color.lighter(150).name());

        item->setBackground(QBrush(color));
        item->setText("●");
        item->setForeground(Qt::white);
        item->setFont(QFont("Arial", 16, QFont::Bold));
    } else {
        item->setBackground(QBrush(QColor(255, 255, 255, 8)));
        item->setText("");
    }
}
