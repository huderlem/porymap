#include "wildmonchart.h"
#include "ui_wildmonchart.h"

#include "log.h"

#include <QtCharts>

// TODO: Conditional QtCharts -> QtGraphs
// TODO: Handle if num pokemon < values
// TODO: Smarter value precision for %s
// TODO: Move level range onto graph?
// TODO: Draw species icons below legend icons?
// TODO: Match group order in chart visually to group order in table

struct ChartData {
    int minLevel;
    int maxLevel;
    QMap<int, double> values; // One value for each wild encounter group
};

WildMonChart::WildMonChart(QWidget *parent, EncounterTableModel *table) :
    QWidget(parent),
    ui(new Ui::WildMonChart)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Window);

    ui->chartView->setRenderHint(QPainter::Antialiasing);

    setTable(table);
};

WildMonChart::~WildMonChart() {
    delete ui;
};

void WildMonChart::setTable(EncounterTableModel *table) {
    this->table = table;
    updateChart();
}

void WildMonChart::updateChart() {
    if (!this->table)
        return;

    // Read data about encounter groups, e.g. for "fishing_mons" we want to know indexes 2-4 belong to good_rod (group index 1).
    // Each group will be represented as a separate bar on the graph.
    QList<QString> groupNames;
    QMap<int, int> tableIndexToGroupIndex;
    int groupIndex = 0;
    for (auto groupPair : table->encounterField().groups) {
        groupNames.append(groupPair.first);
        for (auto i : groupPair.second) {
            tableIndexToGroupIndex.insert(i, groupIndex);
        }
        groupIndex++;
    }
    const int numGroups = qMax(1, groupNames.length()); // Implicitly 1 group when none are listed
    
    // Read data from the table, combining data for duplicate species entries
    const QList<double> tableValues = table->percentages();
    const QVector<WildPokemon> tablePokemon = table->encounterData().wildPokemon;
    QMap<QString, ChartData> speciesToChartData;
    for (int i = 0; i < qMin(tableValues.length(), tablePokemon.length()); i++) {
        const double value = tableValues.at(i);
        const WildPokemon pokemon = tablePokemon.at(i);
        groupIndex = tableIndexToGroupIndex.value(i, 0);

        if (speciesToChartData.contains(pokemon.species)) {
            // Duplicate species entry
            ChartData *entry = &speciesToChartData[pokemon.species];
            entry->values[groupIndex] += value;
            if (entry->minLevel > pokemon.minLevel)
                entry->minLevel = pokemon.minLevel;
            if (entry->maxLevel < pokemon.maxLevel)
                entry->maxLevel = pokemon.maxLevel;
        } else {
            // New species entry
            ChartData entry;
            entry.minLevel = pokemon.minLevel;
            entry.maxLevel = pokemon.maxLevel;
            entry.values.insert(groupIndex, value);
            speciesToChartData.insert(pokemon.species, entry);
        }
    }

    // Populate chart
    //const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::regex_species); // TODO: Change regex to prefix
    QList<QBarSet*> barSets;
    const QString speciesPrefix = "SPECIES_";
    for (auto mapPair = speciesToChartData.cbegin(), end = speciesToChartData.cend(); mapPair != end; mapPair++) {
        const ChartData entry = mapPair.value();

        // Strip 'SPECIES_' prefix
        QString species = mapPair.key();
        if (species.startsWith(speciesPrefix))
            species.remove(0, speciesPrefix.length());

        // Create label for legend
        QString label = QString("%1\nLv %2").arg(species).arg(entry.minLevel);
        if (entry.minLevel != entry.maxLevel)
            label.append(QString("-%1").arg(entry.maxLevel));

        // Add encounter chance data
        auto set = new QBarSet(label);
        for (int i = 0; i < numGroups; i++)
            set->append(entry.values.value(i, 0));

        // Insert bar set in order of total value
        int i = 0;
        for (; i < barSets.length(); i++){
            if (barSets.at(i)->sum() > set->sum())
                break;
        }
        barSets.insert(i, set);
    }

    auto series = new QHorizontalPercentBarSeries();
    series->setLabelsVisible();
    series->append(barSets);

    auto chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // X-axis is the values (percentages)
    auto axisX = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y-axis is the names of encounter groups (e.g. Old Rod, Good Rod...)
    if (numGroups > 1) {
        auto axisY = new QBarCategoryAxis();
        axisY->setCategories(groupNames);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    delete ui->chartView->chart();
    ui->chartView->setChart(chart);
}
