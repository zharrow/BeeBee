#include "DrumGrid.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QJsonArray>
#include <QScrollBar>
#include <QRandomGenerator>


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

void DrumGrid::applyGridUpdate(const GridCell& cell) {
    // Appliquer la mise à jour de la cellule
    setCellActive(cell.row, cell.col, cell.active, cell.userId);

    // Si la cellule appartient à un autre utilisateur, définir sa couleur si nécessaire
    if (!cell.userId.isEmpty() && !m_userColors.contains(cell.userId)) {
        // Générer une couleur aléatoire pour cet utilisateur
        QColor userColor = QColor::fromHsv(
            QRandomGenerator::global()->bounded(360),
            200 + QRandomGenerator::global()->bounded(56),
            200 + QRandomGenerator::global()->bounded(56)
            );
        setUserColor(cell.userId, userColor);
    }
}

void DrumGrid::setupGrid(int instruments, int steps) {
    m_instruments = qBound(MIN_INSTRUMENTS, instruments, MAX_INSTRUMENTS);
    m_steps = qBound(MIN_STEPS, steps, MAX_STEPS);
    m_cellStates.clear();

    m_table->setRowCount(m_instruments);
    m_table->setColumnCount(m_steps);

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
    for (int row = 0; row < m_instruments; ++row) {
        for (int col = 0; col < m_steps; ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setFlags(Qt::ItemIsEnabled);
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, col, item);
            updateCellAppearance(row, col);
        }
    }

    updateTableSize();
    emit instrumentCountChanged(m_instruments);
}

void DrumGrid::setInstrumentCount(int instrumentCount) {
    int newCount = qBound(MIN_INSTRUMENTS, instrumentCount, MAX_INSTRUMENTS);
    if (newCount == m_instruments) return;

    m_instruments = newCount;
    resizeGridForInstruments();
    emit instrumentCountChanged(m_instruments);
}

void DrumGrid::resizeGridForInstruments() {
    // Sauvegarder l'état actuel des cellules pour les instruments existants
    QMap<QPair<int,int>, QPair<bool,QString>> oldStates = m_cellStates;

    // Redimensionner la table
    m_table->setRowCount(m_instruments);

    // Nettoyer les états des cellules pour les lignes supprimées
    if (m_instruments < m_table->rowCount()) {
        for (auto it = m_cellStates.begin(); it != m_cellStates.end();) {
            if (it.key().first >= m_instruments) {
                it = m_cellStates.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Initialiser les nouvelles cellules
    for (int row = 0; row < m_instruments; ++row) {
        for (int col = 0; col < m_steps; ++col) {
            if (!m_table->item(row, col)) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setFlags(Qt::ItemIsEnabled);
                item->setTextAlignment(Qt::AlignCenter);
                m_table->setItem(row, col, item);
            }
            updateCellAppearance(row, col);
        }
    }

    // Mettre à jour les headers verticaux avec les noms d'instruments
    for (int i = 0; i < m_instruments; ++i) {
        QString name = (i < m_instrumentNames.size()) ? m_instrumentNames[i] : QString("Instrument %1").arg(i + 1);
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(name));
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

    // Ajuster le nombre d'instruments si nécessaire
    if (names.size() != m_instruments) {
        setInstrumentCount(names.size());
    }

    // Mise à jour des headers verticaux
    for (int i = 0; i < qMin(names.size(), m_instruments); ++i) {
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(names[i]));
    }

    // Pour les instruments sans nom, utiliser un nom par défaut
    for (int i = names.size(); i < m_instruments; ++i) {
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(QString("Instrument %1").arg(i + 1)));
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
    if (col >= m_steps || row >= m_instruments) return; // Protection contre les indices invalides

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
    state["instrumentCount"] = m_instruments;

    return state;
}

void DrumGrid::setGridState(const QJsonObject& state) {
    // Réinitialisation
    m_cellStates.clear();

    // Charger le nombre de steps si présent
    if (state.contains("stepCount")) {
        setStepCount(state["stepCount"].toInt());
    }

    // Charger le nombre d'instruments si présent
    if (state.contains("instrumentCount")) {
        setInstrumentCount(state["instrumentCount"].toInt());
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
        // Coloration alternée pour mieux visualiser les mesures
        QColor bgColor = (col % 4 == 0) ? QColor(220, 220, 220) : QColor(240, 240, 240);
        item->setBackground(QBrush(bgColor));
        item->setText("");
    }
}


