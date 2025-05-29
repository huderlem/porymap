#if __has_include(<QtCharts>)
#include "wildmonchart.h"
#include "ui_wildmonchart.h"
#include "config.h"
#include "utility.h"
#include "message.h"

static const QString baseWindowTitle = QString("Wild Pokémon Summary Charts");

static const QList<QPair<QString, QChart::ChartTheme>> themes = {
    {"Light",         QChart::ChartThemeLight},
    {"Dark",          QChart::ChartThemeDark},
    {"Blue Cerulean", QChart::ChartThemeBlueCerulean},
    {"Brown Sand",    QChart::ChartThemeBrownSand},
    {"Blue NCS",      QChart::ChartThemeBlueNcs},
    {"High Contrast", QChart::ChartThemeHighContrast},
    {"Blue Icy",      QChart::ChartThemeBlueIcy},
    {"Qt",            QChart::ChartThemeQt},
};

WildMonChart::WildMonChart(QWidget *parent, const EncounterTableModel *table) :
    QWidget(parent),
    ui(new Ui::WildMonChart)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Window);

    connect(ui->button_Help, &QAbstractButton::clicked, this, &WildMonChart::showHelpDialog);

    // Changing these settings changes which level distribution chart is shown
    connect(ui->groupBox_Species, &QGroupBox::clicked, this, &WildMonChart::refreshLevelDistributionChart);
    connect(ui->comboBox_Species, &QComboBox::currentTextChanged, this, &WildMonChart::refreshLevelDistributionChart);
    connect(ui->comboBox_Group, &QComboBox::currentTextChanged, this, &WildMonChart::refreshLevelDistributionChart);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &WildMonChart::limitChartAnimation);

    // Set up Theme combo box
    for (auto i : themes)
        ui->comboBox_Theme->addItem(i.first, i.second);
    connect(ui->comboBox_Theme, &QComboBox::currentTextChanged, this, &WildMonChart::updateTheme);

    // User's theme choice is saved in the config
    int configThemeIndex = ui->comboBox_Theme->findText(porymapConfig.wildMonChartTheme);
    if (configThemeIndex >= 0) {
        const QSignalBlocker blocker(ui->comboBox_Theme);
        ui->comboBox_Theme->setCurrentIndex(configThemeIndex);
    } else {
        porymapConfig.wildMonChartTheme = ui->comboBox_Theme->currentText();
    }

    restoreGeometry(porymapConfig.wildMonChartGeometry);

    setTable(table);
};

WildMonChart::~WildMonChart() {
    delete ui;
};

void WildMonChart::setTable(const EncounterTableModel *table) {
    if (this->table == table)
        return;
    this->table = table;
    refresh();
}

void WildMonChart::clearTable() {
    setTable(nullptr);
}

void WildMonChart::clearTableData() {
    this->groupNames.clear();
    this->groupNamesReversed.clear();
    this->speciesInLegendOrder.clear();
    this->tableIndexToGroupName.clear();
    this->groupedLevelRanges.clear();
    this->speciesToGroupedData.clear();
    this->speciesToColor.clear();
    setWindowTitle(baseWindowTitle);

    const QSignalBlocker blocker1(ui->comboBox_Species);
    const QSignalBlocker blocker2(ui->comboBox_Group);
    ui->comboBox_Species->clear();
    ui->comboBox_Group->clear();
    ui->comboBox_Group->setEnabled(false);
    ui->label_Group->setEnabled(false);
}

// Extract all the data from the table that we need for the charts
void WildMonChart::readTable() {
    clearTableData();
    if (!this->table)
        return;

    setWindowTitle(QString("%1 - %2").arg(baseWindowTitle).arg(this->table->encounterField().name));

    // Read data about encounter groups, e.g. for "fishing_mons" we want to know table indexes 2-4 belong to "good_rod"
    for (auto groupPair : this->table->encounterField().groups) {
        // Frustratingly when adding categories to the charts they insert bottom-to-top, but the table and combo box
        // insert top-to-bottom. To keep the order visually the same across displays we keep separate ordered lists.
        this->groupNames.append(groupPair.first);
        this->groupNamesReversed.prepend(groupPair.first);
        for (auto i : groupPair.second)
            this->tableIndexToGroupName.insert(i, groupPair.first);
    }
    // Implicitly 1 unnamed group when none are listed
    if (this->groupNames.isEmpty()) {
        this->groupNames.append(QString());
        this->groupNamesReversed.append(QString());
    }
    
    // Read data from the table, combining data for duplicate entries
    const QVector<double> tableFrequencies = this->table->percentages();
    const QVector<WildPokemon> tablePokemon = this->table->encounterData().wildPokemon;
    const int numRows = qMin(tableFrequencies.length(), tablePokemon.length());
    const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix);
    for (int i = 0; i < numRows; i++) {
        const double frequency = tableFrequencies.at(i);
        const WildPokemon pokemon = tablePokemon.at(i);
        const QString groupName = this->tableIndexToGroupName.value(i);

        // Create species label (strip 'SPECIES_' prefix).
        QString label = Util::stripPrefix(pokemon.species, speciesPrefix);

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
    const QSignalBlocker blocker1(ui->comboBox_Species);
    const QSignalBlocker blocker2(ui->comboBox_Group);
    ui->comboBox_Species->clear();
    ui->comboBox_Species->addItems(getSpeciesNamesAlphabetical());
    ui->comboBox_Group->clear();
    ui->comboBox_Group->addItems(this->groupNames);
    bool enableGroupSelection = usesGroupLabels();
    ui->comboBox_Group->setEnabled(enableGroupSelection);
    ui->label_Group->setEnabled(enableGroupSelection);
}

void WildMonChart::refresh() {
    const QSignalBlocker blocker1(ui->comboBox_Species);
    const QSignalBlocker blocker2(ui->comboBox_Group);
    const QString oldSpecies = ui->comboBox_Species->currentText();
    const QString oldGroup = ui->comboBox_Group->currentText();

    readTable();

    // If the old UI settings are still valid we should restore them
    int index = ui->comboBox_Species->findText(oldSpecies);
    if (index >= 0) ui->comboBox_Species->setCurrentIndex(index);

    index = ui->comboBox_Group->findText(oldGroup);
    if (index >= 0) ui->comboBox_Group->setCurrentIndex(index);

    refreshSpeciesDistributionChart();
    refreshLevelDistributionChart();
}

void WildMonChart::refreshSpeciesDistributionChart() {
    if (ui->chartView_SpeciesDistribution->chart())
        ui->chartView_SpeciesDistribution->chart()->deleteLater();
    ui->chartView_SpeciesDistribution->setChart(createSpeciesDistributionChart());
    if (ui->tabWidget->currentWidget() == ui->tabSpecies)
        limitChartAnimation();
}

void WildMonChart::refreshLevelDistributionChart() {
    if (ui->chartView_LevelDistribution->chart())
        ui->chartView_LevelDistribution->chart()->deleteLater();
    ui->chartView_LevelDistribution->setChart(createLevelDistributionChart());
    if (ui->tabWidget->currentWidget() == ui->tabLevels)
        limitChartAnimation();
}

QStringList WildMonChart::getSpeciesNamesAlphabetical() const {
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

QChart* WildMonChart::createSpeciesDistributionChart() {
    QList<QBarSet*> barSets;
    for (const auto &species : getSpeciesNamesAlphabetical()) {
        // Add encounter chance data
        auto set = new QBarSet(species);
        for (auto groupName : this->groupNamesReversed)
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
        connect(set, &QBarSet::hovered, [set] (bool on, int i) {
            QString text = on ? QString("%1 (%2%)").arg(set->label()).arg(set->at(i)) : "";
            QToolTip::showText(QCursor::pos(), text);
        });
    }

    // Preserve the order we set earlier so that the legend isn't shuffling around for the other all-species charts.
    this->speciesInLegendOrder.clear();
    for (auto set : barSets)
        this->speciesInLegendOrder.append(set->label());

    // Set up series
    auto series = new QHorizontalPercentBarSeries();
    series->setLabelsVisible();
    series->append(barSets);

    // Set up chart
    auto chart = new QChart();
    chart->addSeries(series);
    chart->setTheme(currentTheme());
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    saveSpeciesColors(barSets);

    // X-axis is the % frequency. We're already showing percentages on the bar, so we just display 0/50/100%
    auto axisX = new QValueAxis();
    axisX->setLabelFormat("%u%%");
    axisX->setTickCount(3);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y-axis is the names of encounter groups (e.g. Old Rod, Good Rod...)
    if (usesGroupLabels()) {
        auto axisY = new QBarCategoryAxis();
        axisY->setCategories(this->groupNamesReversed);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
    }

    return chart;
}

QBarSet* WildMonChart::createLevelDistributionBarSet(const QString &species, const QString &groupName, bool individual) {
    const double totalFrequency = individual ? getSpeciesFrequency(species, groupName) : 1.0;
    const QMap<int, double> levelFrequencies = getLevelFrequencies(species, groupName);

    auto set = new QBarSet(species);
    LevelRange levelRange = individual ? getLevelRange(species, groupName) : this->groupedLevelRanges.value(groupName);
    for (int i = levelRange.min; i <= levelRange.max; i++) {
        set->append(levelFrequencies.value(i, 0) / totalFrequency * 100);
    }

    // Show data when hovering over a bar set. This covers some shortfalls in our ability to control the chart design.
    connect(set, &QBarSet::hovered, [=] (bool on, int i) {
        QString text = on ? QString("%1 Lv%2 (%3%)")
                            .arg(individual ? "" : set->label())
                            .arg(QString::number(i + levelRange.min))
                            .arg(set->at(i))
                          : "";
        QToolTip::showText(QCursor::pos(), text);
    });

    // Clicking on a bar set in the stacked chart opens its individual chart
    if (!individual) {
        connect(set, &QBarSet::clicked, [this, species](int) {
            const QSignalBlocker blocker1(ui->groupBox_Species);
            const QSignalBlocker blocker2(ui->comboBox_Species);
            ui->groupBox_Species->setChecked(true);
            ui->comboBox_Species->setTextItem(species);
            refreshLevelDistributionChart();
        });
    }

    return set;
}

QChart* WildMonChart::createLevelDistributionChart() {
    const QString groupName = ui->comboBox_Group->currentText();

    LevelRange levelRange;
    QList<QBarSet*> barSets;

    // Create bar sets
    if (ui->groupBox_Species->isChecked()) {
        // Species box is active, we just display data for the selected species.
        const QString species = ui->comboBox_Species->currentText();
        barSets.append(createLevelDistributionBarSet(species, groupName, true));
        levelRange = getLevelRange(species, groupName);
    } else {
        // Species box is inactive, we display data for all species in the table.
        for (const auto &species : this->speciesInLegendOrder)
            barSets.append(createLevelDistributionBarSet(species, groupName, false));
        levelRange = this->groupedLevelRanges.value(groupName);
    }

    // Set up series
    auto series = new QStackedBarSeries();
    //series->setLabelsVisible();
    series->append(barSets);

    // Set up chart
    auto chart = new QChart();
    chart->addSeries(series);
    chart->setTheme(currentTheme());
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setShowToolTips(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    applySpeciesColors(barSets); // Has to happen after theme is set

    // X-axis is the level range.
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    for (int i = levelRange.min; i <= levelRange.max; i++)
        axisX->append(QString::number(i));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y-axis is the % frequency
    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("%u%%");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // We round the y-axis max up to a multiple of 5.
    axisY->setMax(Util::roundUp(qCeil(axisY->max()), 5));

    return chart;
}

QChart::ChartTheme WildMonChart::currentTheme() const {
    return static_cast<QChart::ChartTheme>(ui->comboBox_Theme->currentData().toInt());
}

void WildMonChart::updateTheme() {
    auto theme = currentTheme();
    porymapConfig.wildMonChartTheme = ui->comboBox_Theme->currentText();

    // In order to keep the color of each species in the legend consistent across
    // charts we save species->color mappings. The legend colors are overwritten
    // when we change themes, so we need to recalculate them. We let the species
    // distribution chart determine what those mapping are (it always includes every
    // species in the table) and then we apply those mappings to subsequent charts.
    QChart *chart = ui->chartView_SpeciesDistribution->chart();
    if (!chart || chart->series().isEmpty())
        return;
    chart->setTheme(theme);
    saveSpeciesColors(static_cast<QAbstractBarSeries*>(chart->series().at(0))->barSets());

    chart = ui->chartView_LevelDistribution->chart();
    if (chart) {
        chart->setTheme(theme);
        applySpeciesColors(static_cast<QAbstractBarSeries*>(chart->series().at(0))->barSets());
    }
}

void WildMonChart::saveSpeciesColors(const QList<QBarSet*> &barSets) {
    this->speciesToColor.clear();
    for (auto set : barSets)
        this->speciesToColor.insert(set->label(), set->color());
}

void WildMonChart::applySpeciesColors(const QList<QBarSet*> &barSets) {
    for (auto set : barSets)
        set->setColor(this->speciesToColor.value(set->label()));
}

// Turn off the chart animation once it's played, otherwise it replays any time the window changes size.
// The animation only begins when it's first displayed, so we'll only ever consider the chart for the current tab,
// and when the tab changes we'll call this again.
void WildMonChart::limitChartAnimation() {
    // Chart may be destroyed before the animation finishes
    QPointer<QChart> chart;
    if (ui->tabWidget->currentWidget() == ui->tabSpecies) {
        chart = ui->chartView_SpeciesDistribution->chart();
    } else if (ui->tabWidget->currentWidget() == ui->tabLevels) {
        chart = ui->chartView_LevelDistribution->chart();
    }

    if (!chart || chart->animationOptions() == QChart::NoAnimation)
        return;

    // QChart has no signal for when the animation is finished, so we use a timer to stop the animation.
    // It is technically possible to get the chart to freeze mid-animation by resizing the window after
    // the timer starts but before it finishes, but 1. animations are short so this is difficult to do,
    // and 2. the issue resolves if the window is resized afterwards, so it's probably fine.
    QTimer::singleShot(chart->animationDuration(), Qt::PreciseTimer, [chart] {
        if (chart) chart->setAnimationOptions(QChart::NoAnimation);
    });
}

void WildMonChart::showHelpDialog() {
    static const QString text = "This window provides some visualizations of the data in your current Wild Pokémon tab";

    // Describe the Species Distribution tab
    static const QString speciesTabInfo =
       "The <b>Species Distribution</b> tab shows the cumulative encounter chance for each species "
       "in the table. In other words, it answers the question \"What is the likelihood of encountering "
       "each species in a single encounter?\"";

    // Describe the Level Distribution tab
    static const QString levelTabInfo =
       "The <b>Level Distribution</b> tab shows the chance of encountering each species at a particular level. "
       "In the top left under <b>Group</b> you can select which encounter group to show data for. "
       "In the top right you can enable <b>Individual Mode</b>. When enabled data will be shown for only the selected species."
       "<br><br>"
       "In other words, while <b>Individual Mode</b> is checked the chart is answering the question \"If a species x "
       "is encountered, what is the likelihood that it will be level y\", and while <b>Individual Mode</b> is not checked, "
       "it answers the question \"For a single encounter, what is the likelihood of encountering a species x at level y.\"";

    QString informativeText;
    if (ui->tabWidget->currentWidget() == ui->tabSpecies) {
        informativeText = speciesTabInfo;
    } else if (ui->tabWidget->currentWidget() == ui->tabLevels) {
        informativeText = levelTabInfo;
    }
    InfoMessage::show(text, informativeText, this);
}

void WildMonChart::closeEvent(QCloseEvent *event) {
    porymapConfig.wildMonChartGeometry = saveGeometry();
    QWidget::closeEvent(event);
}

#endif // __has_include(<QtCharts>)
