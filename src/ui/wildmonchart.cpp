#include "wildmonchart.h"
#include "ui_wildmonchart.h"
#include "config.h"

#include "log.h"

#include <QtCharts>

// TODO: Make level range its own chart(s)?
// TODO: Draw species icons below legend icons?
// TODO: Add hover behavior to display species name (and click prompt?)

struct ChartData {
    int minLevel;
    int maxLevel;
    QMap<int, double> values; // One value for each wild encounter group
};

WildMonChart::WildMonChart(QWidget *parent, const EncounterTableModel *table) :
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

void WildMonChart::setTable(const EncounterTableModel *table) {
    this->table = table;
    updateChart();
}

static const bool showLevelRange = false;

void WildMonChart::updateChart() {
    if (!this->table)
        return;

    setWindowTitle(QString("Wild Pokémon Summary -- %1").arg(this->table->encounterField().name));

    // Read data about encounter groups, e.g. for "fishing_mons" we want to know indexes 2-4 belong to good_rod (group index 1).
    // Each group will be represented as a separate bar on the graph.
    QList<QString> groupNames;
    QMap<int, int> tableIndexToGroupIndex;
    int groupIndex = 0;
    for (auto groupPair : this->table->encounterField().groups) {
        groupNames.prepend(groupPair.first);
        for (auto i : groupPair.second) {
            tableIndexToGroupIndex.insert(i, groupIndex);
        }
        groupIndex++;
    }
    const int numGroups = qMax(1, groupNames.length()); // Implicitly 1 group when none are listed
    
    // Read data from the table, combining data for duplicate species entries
    const QList<double> tableValues = this->table->percentages();
    const QVector<WildPokemon> tablePokemon = this->table->encounterData().wildPokemon;
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
    QList<QBarSet*> barSets;
    const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix);
    for (auto mapPair = speciesToChartData.cbegin(), end = speciesToChartData.cend(); mapPair != end; mapPair++) {
        const ChartData entry = mapPair.value();

        // Strip 'SPECIES_' prefix
        QString label = mapPair.key();
        if (label.startsWith(speciesPrefix))
            label.remove(0, speciesPrefix.length());

        // Add level range to label
        if (showLevelRange) {
            if (entry.minLevel == entry.maxLevel)
                label.append(QString(" (Lv %1)").arg(entry.minLevel));
            else
                label.append(QString(" (Lv %1-%2)").arg(entry.minLevel).arg(entry.maxLevel));
        }

        auto set = new QBarSet(label);

        // Add encounter chance data (in reverse order, to match the table's group order visually)
        for (int i = numGroups - 1; i >= 0; i--)
            set->append(entry.values.value(i, 0));

        // Insert bar set. We order them from lowest to highest total, left-to-right.
        int i = 0;
        for (; i < barSets.length(); i++){
            if (barSets.at(i)->sum() > set->sum())
                break;
        }
        barSets.insert(i, set);
    }

    auto series = new QHorizontalPercentBarSeries();
    series->setLabelsVisible();
    //series->setLabelsPrecision(x); // This appears to have no effect for any value 'x'? Ideally we'd display 1-2 decimal places
    series->append(barSets);

    auto chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // X-axis is the values (percentages). We're already showing percentages on the bar, so we just display 0/50/100%
    auto axisX = new QValueAxis();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
    // Not critical, but the percentage ticks on the x-axis have no need for decimals.
    // This property doesn't exist prior to Qt 6.7
    axisX->setLabelDecimals(0);
#endif
    axisX->setTickCount(3);
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
    
    // Turn off the animation once it's played, otherwise it replays any time the window changes size.
    QTimer::singleShot(chart->animationDuration() + 500, [this] {
        if (ui->chartView->chart())
            ui->chartView->chart()->setAnimationOptions(QChart::NoAnimation);
    });
}
