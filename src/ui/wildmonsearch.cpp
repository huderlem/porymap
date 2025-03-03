#include "wildmonsearch.h"
#include "ui_wildmonsearch.h"
#include "project.h"

enum ResultsColumn {
    Group,
    Field,
    Level,
    Chance,
};

enum ResultsDataRole {
    MapName = Qt::UserRole,
};

WildMonSearch::WildMonSearch(Project *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WildMonSearch),
    project(project)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    // Set up species combo box
    ui->comboBox_Search->addItems(project->speciesNames);
    ui->comboBox_Search->setCurrentText(QString());
    ui->comboBox_Search->lineEdit()->setPlaceholderText(Project::getEmptySpeciesName());
    connect(ui->comboBox_Search, &QComboBox::currentTextChanged, this, &WildMonSearch::updateResults);

    // Set up table header
    static const QStringList labels = {"Group", "Field", "Level", "Chance"};
    ui->table_Results->setHorizontalHeaderLabels(labels);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::Group,  QHeaderView::Stretch);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::Field,  QHeaderView::ResizeToContents);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::Level,  QHeaderView::ResizeToContents);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::Chance, QHeaderView::ResizeToContents);

    // Table is read-only
    ui->table_Results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_Results->setSelectionMode(QAbstractItemView::NoSelection);

    connect(ui->table_Results, &QTableWidget::cellDoubleClicked, this, &WildMonSearch::cellDoubleClicked);

    refresh();
}

WildMonSearch::~WildMonSearch() {
    delete ui;
}

void WildMonSearch::refresh() {
    this->resultsCache.clear();
    updatePercentageStrings();
    updateResults(ui->comboBox_Search->currentText());
}

void WildMonSearch::addTableEntry(const RowData &rowData) {
    int row = ui->table_Results->rowCount();
    ui->table_Results->insertRow(row);

    auto groupItem = new NumericSortTableItem(rowData.groupName);
    groupItem->setData(ResultsDataRole::MapName, rowData.mapName);

    ui->table_Results->setItem(row, ResultsColumn::Group, groupItem);
    ui->table_Results->setItem(row, ResultsColumn::Field, new NumericSortTableItem(rowData.fieldName));
    ui->table_Results->setItem(row, ResultsColumn::Level, new NumericSortTableItem(rowData.levelRange));
    ui->table_Results->setItem(row, ResultsColumn::Chance, new NumericSortTableItem(rowData.chance));
}

QList<WildMonSearch::RowData> WildMonSearch::search(const QString &species) const {
    QList<RowData> results;
    for (const auto &keyPair : this->project->wildMonData) {
        QString mapConstant = keyPair.first;
        for (const auto &grouplLabelPair : this->project->wildMonData[mapConstant]) {
            QString groupName = grouplLabelPair.first;
            WildPokemonHeader encounterHeader = this->project->wildMonData[mapConstant][groupName];
            for (const auto &fieldNamePair : encounterHeader.wildMons) {
                QString fieldName = fieldNamePair.first;
                WildMonInfo monInfo = encounterHeader.wildMons[fieldName];
                for (int slot = 0; slot < monInfo.wildPokemon.length(); slot++) {
                    const WildPokemon wildMon = monInfo.wildPokemon.at(slot);
                    if (wildMon.species == species) {
                        RowData rowData;
                        rowData.groupName = groupName;
                        rowData.fieldName = fieldName;
                        rowData.mapName = this->project->mapConstantsToMapNames.value(mapConstant, mapConstant);

                        // If min and max level are the same display a single number, otherwise display a level range.
                        rowData.levelRange = (wildMon.minLevel == wildMon.maxLevel) ? QString::number(wildMon.minLevel)
                                                                                    : QString("%1-%2").arg(wildMon.minLevel).arg(wildMon.maxLevel);
                        rowData.chance = this->percentageStrings[fieldName][slot];
                        results.append(rowData);
                    }
                }
            }
        }
    }
    return results;
}

void WildMonSearch::updatePercentageStrings() {
    this->percentageStrings.clear();
    for (const EncounterField &monField : this->project->wildMonFields) {
        QMap<int,QString> slotToPercentString;
        auto percentages = getWildEncounterPercentages(monField);
        for (int i = 0; i < percentages.length(); i++) {
            slotToPercentString.insert(i, QString::number(percentages.at(i) * 100, 'f', 2) + "%");
        }
        this->percentageStrings[monField.name] = slotToPercentString;
    }
}

void WildMonSearch::updateResults(const QString &species) {
    ui->speciesIcon->setPixmap(this->project->getSpeciesIcon(species));

    ui->table_Results->clearContents();
    ui->table_Results->setRowCount(0);

    // Note: Per Qt docs, sorting should be disabled while populating the table to avoid it interfering with insertion order.
    ui->table_Results->setSortingEnabled(false);

    if (ui->comboBox_Search->findText(species) < 0)
        return; // Not a species name, no need to search wild encounter data.

    const QList<RowData> results = this->resultsCache.value(species, search(species));
    if (results.isEmpty()) {
        static const RowData noResults = {
            .groupName = QStringLiteral("Species not found."),
            .fieldName = QStringLiteral("--"),
            .levelRange = QStringLiteral("--"),
            .chance = QStringLiteral("--"),
        };
        addTableEntry(noResults);
    } else {
        for (const auto &entry : results) {
            addTableEntry(entry);
        }
    }

    ui->table_Results->setSortingEnabled(true);

    this->resultsCache.insert(species, results);
}

// Double-clicking row data opens the corresponding map/table on the Wild Pokémon tab.
void WildMonSearch::cellDoubleClicked(int row, int) {
    auto groupItem = ui->table_Results->item(row, ResultsColumn::Group);
    auto fieldItem = ui->table_Results->item(row, ResultsColumn::Field);
    if (!groupItem || !fieldItem)
        return;

    const QString mapName = groupItem->data(ResultsDataRole::MapName).toString();
    if (mapName.isEmpty())
        return;

    emit openWildMonTableRequested(mapName, groupItem->text(), fieldItem->text());
}
