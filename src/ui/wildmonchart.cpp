#include "wildmonchart.h"
#include "ui_wildmonchart.h"
#include "config.h"

#include "log.h"

// TODO: Draw species icons below legend icons?
// TODO: NoScrollComboBoxes
// TODO: Consistent species->color across charts

static const QString baseWindowTitle = QString("Wild Pokémon Summary Charts");

WildMonChart::WildMonChart(QWidget *parent, const EncounterTableModel *table) :
    QWidget(parent),
    ui(new Ui::WildMonChart)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Window);

    // Changing these settings changes which level distribution chart is shown
    connect(ui->groupBox_Species, &QGroupBox::clicked, this, &WildMonChart::createLevelDistributionChart);
    connect(ui->comboBox_Species, &QComboBox::currentTextChanged, this, &WildMonChart::createLevelDistributionChart);
    connect(ui->comboBox_Group, &QComboBox::currentTextChanged, this, &WildMonChart::createLevelDistributionChart);

    setTable(table);
};

WildMonChart::~WildMonChart() {
    delete ui;
};

void WildMonChart::setTable(const EncounterTableModel *table) {
    this->table = table;
    readTable();
    createCharts();
}

void WildMonChart::clearTableData() {
    this->groupNames.clear();
    this->tableIndexToGroupName.clear();
    this->speciesToGroupedData.clear();
    this->groupedLevelRanges.clear();
    setWindowTitle(baseWindowTitle);
}

// Extract all the data from the table that we need for the charts
void WildMonChart::readTable() {
    clearTableData();
    if (!this->table)
        return;

    setWindowTitle(QString("%1 - %2").arg(baseWindowTitle).arg(this->table->encounterField().name));

    // Read data about encounter groups, e.g. for "fishing_mons" we want to know table indexes 2-4 belong to "good_rod"
    for (auto groupPair : this->table->encounterField().groups) {
        // Prepending names here instead of appending so that charts can match the order in the table visually.
        this->groupNames.prepend(groupPair.first);
        for (auto i : groupPair.second)
            this->tableIndexToGroupName.insert(i, groupPair.first);
    }
    if (this->groupNames.isEmpty())
        this->groupNames.append(QString()); // Implicitly 1 unnamed group when none are listed
    
    // Read data from the table, combining data for duplicate entries
    const QList<double> tableFrequencies = this->table->percentages();
    const QVector<WildPokemon> tablePokemon = this->table->encounterData().wildPokemon;
    const int numRows = qMin(tableFrequencies.length(), tablePokemon.length());
    const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix);
    for (int i = 0; i < numRows; i++) {
        const double frequency = tableFrequencies.at(i);
        const WildPokemon pokemon = tablePokemon.at(i);
        const QString groupName = this->tableIndexToGroupName.value(i);

        // Create species label (strip 'SPECIES_' prefix).
        QString label = pokemon.species;
        if (label.startsWith(speciesPrefix))
            label.remove(0, speciesPrefix.length());

        // Add species/level frequency data
        Summary *summary = &this->speciesToGroupedData[label][groupName];
        summary->speciesFrequency += frequency;
        if (pokemon.minLevel > pokemon.maxLevel)
            continue; // Invalid
        int numLevels = pokemon.maxLevel - pokemon.minLevel + 1;
        for (int level = pokemon.minLevel; level <= pokemon.maxLevel; level++)
            summary->levelFrequencies[level] += frequency / numLevels;

        // Update level min/max for showing level distribution across a group
        if (!this->groupedLevelRanges.contains(groupName)) {
            LevelRange *levelRange = &this->groupedLevelRanges[groupName];
            levelRange->min = pokemon.minLevel;
            levelRange->max = pokemon.maxLevel;
        } else {
            LevelRange *levelRange = &this->groupedLevelRanges[groupName];
            if (levelRange->min > pokemon.minLevel)
                levelRange->min = pokemon.minLevel;
            if (levelRange->max < pokemon.maxLevel)
                levelRange->max = pokemon.maxLevel;
        }
    }

    // Populate combo boxes
    // TODO: Limit width
    const QSignalBlocker blocker1(ui->comboBox_Species);
    const QSignalBlocker blocker2(ui->comboBox_Group);
    ui->comboBox_Species->clear();
    ui->comboBox_Species->addItems(getSpeciesNames());
    ui->comboBox_Group->clear();
    if (usesGroupLabels()) {
        ui->comboBox_Group->addItems(this->groupNames);
        ui->comboBox_Group->setEnabled(true);
    } else {
        ui->comboBox_Group->setEnabled(false);
    }
}

void WildMonChart::createCharts() {
    createSpeciesDistributionChart();
    createLevelDistributionChart();
    
    // Turn off the animation once it's played, otherwise it replays any time the window changes size.
    // TODO: Store timer, disable if closing or creating new chart
    //QTimer::singleShot(chart->animationDuration() + 500, this, &WildMonChart::stopChartAnimation);
}

QStringList WildMonChart::getSpeciesNames() const {
    return this->speciesToGroupedData.keys();
}

double WildMonChart::getSpeciesFrequency(const QString &species, const QString &groupName) const {
    return this->speciesToGroupedData[species][groupName].speciesFrequency;
}

QMap<int, double> WildMonChart::getLevelFrequencies(const QString &species, const QString &groupName) const {
    return this->speciesToGroupedData[species][groupName].levelFrequencies;
}

WildMonChart::LevelRange WildMonChart::getLevelRange(const QString &species, const QString &groupName) const {
    const QList<int> levels = getLevelFrequencies(species, groupName).keys();

    LevelRange range;
    if (levels.isEmpty()) {
        range.min = 0;
        range.max = 0;
    } else {
        range.min = levels.first();
        range.max = levels.last();
    }
    return range;
}

bool WildMonChart::usesGroupLabels() const {
    return this->groupNames.length() > 1;
}

void WildMonChart::createSpeciesDistributionChart() {
    QList<QBarSet*> barSets;
    for (const auto species : getSpeciesNames()) {
        // Add encounter chance data
        auto set = new QBarSet(species);
        for (auto groupName : this->groupNames)
            set->append(getSpeciesFrequency(species, groupName) * 100);

        // We order the bar sets from lowest to highest total, left-to-right.
        for (int i = 0; i < barSets.length() + 1; i++){
            if (i >= barSets.length() || barSets.at(i)->sum() > set->sum()) {
                barSets.insert(i, set);
                break;
            }
        }

        // Show species name and % when hovering over a bar set. This covers some shortfalls in our ability to control the chart design
        // (i.e. bar segments may be too narrow to see the % label, or colors may be hard to match to the legend).
        connect(set, &QBarSet::hovered, [set, species] (bool on, int i) {
            QString text = on ? QString("%1 (%2%)").arg(species).arg(set->at(i)) : "";
            QToolTip::showText(QCursor::pos(), text);
        });
    }

    // Set up series
    auto series = new QHorizontalPercentBarSeries();
    series->setLabelsVisible();
    series->append(barSets);

    // Set up chart
    auto chart = new QChart();
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // X-axis is the % frequency. We're already showing percentages on the bar, so we just display 0/50/100%
    auto axisX = new QValueAxis();
    axisX->setLabelFormat("%u%%");
    axisX->setTickCount(3);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y-axis is the names of encounter groups (e.g. Old Rod, Good Rod...)
    if (usesGroupLabels()) {
        auto axisY = new QBarCategoryAxis();
        axisY->setCategories(this->groupNames);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    // TODO: Delete old chart
    ui->chartView_SpeciesDistribution->setChart(chart);
}

QBarSet* WildMonChart::createLevelDistributionBarSet(const QString &species, const QString &groupName, bool individual, double *barMax) {
    const double totalFrequency = individual ? getSpeciesFrequency(species, groupName) : 1.0;
    const QMap<int, double> levelFrequencies = getLevelFrequencies(species, groupName);

    auto set = new QBarSet(species);
    LevelRange levelRange = individual ? getLevelRange(species, groupName) : this->groupedLevelRanges.value(groupName);
    for (int i = levelRange.min; i <= levelRange.max; i++) {
        double percent = levelFrequencies.value(i, 0) / totalFrequency * 100;
        if (*barMax < percent)
            *barMax = percent;
        set->append(percent);
    }

    // Show data when hovering over a bar set. This covers some shortfalls in our ability to control the chart design.
    connect(set, &QBarSet::hovered, [=] (bool on, int i) {
        QString text = on ? QString("%1 Lv%2 (%3%)")
                            .arg(individual ? "" : species)
                            .arg(QString::number(i + levelRange.min))
                            .arg(set->at(i))
                          : "";
        QToolTip::showText(QCursor::pos(), text);
    });

    return set;
}

void WildMonChart::createLevelDistributionChart() {
    const QString groupName = ui->comboBox_Group->currentText();

    LevelRange levelRange;
    double maxPercent = 0.0;
    QList<QBarSet*> barSets;

    // Create bar sets
    if (ui->groupBox_Species->isChecked()) {
        // Species box is active, we just display data for the selected species.
        const QString species = ui->comboBox_Species->currentText();
        barSets.append(createLevelDistributionBarSet(species, groupName, true, &maxPercent));
        levelRange = getLevelRange(species, groupName);
    } else {
        // Species box is inactive, we display data for all species in the table.
        for (const auto species : getSpeciesNames())
            barSets.append(createLevelDistributionBarSet(species, groupName, false, &maxPercent));
        levelRange = this->groupedLevelRanges.value(groupName);
    }

    // Set up chart
    auto chart = new QChart();
    //chart->setTitle("");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // X-axis is the level range.
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    for (int i = levelRange.min; i <= levelRange.max; i++)
        axisX->append(QString::number(i));
    chart->addAxis(axisX, Qt::AlignBottom);

    // Y-axis is the % frequency. We round the max up to a multiple of 5.
    auto roundUp = [](int num, int multiple) {
        auto remainder = num % multiple;
        if (remainder == 0)
            return num;
        return num + multiple - remainder;
    };
    QValueAxis *axisY = new QValueAxis();
    axisY->setMax(roundUp(qCeil(maxPercent), 5));
    //axisY->setTickType(QValueAxis::TicksDynamic);
    //axisY->setTickInterval(5);
    axisY->setLabelFormat("%u%%");
    chart->addAxis(axisY, Qt::AlignLeft);

    // Set up series. Grouped mode uses a stacked bar series.
    if (barSets.length() < 2) {
        auto series = new QBarSeries();
        series->append(barSets);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        //series->setLabelsVisible();
        chart->addSeries(series);
    } else {
        auto series = new QStackedBarSeries();
        series->append(barSets);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        //series->setLabelsVisible();
        chart->addSeries(series);
    }

    // TODO: Cache old chart
    ui->chartView_LevelDistribution->setChart(chart);
}

void WildMonChart::stopChartAnimation() {
    if (ui->chartView_SpeciesDistribution->chart())
        ui->chartView_SpeciesDistribution->chart()->setAnimationOptions(QChart::NoAnimation);
}
