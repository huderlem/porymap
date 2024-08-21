#include "wildmonchart.h"
#include "ui_wildmonchart.h"

#include <QtCharts>

WildMonChart::WildMonChart(QWidget *parent, EncounterTableModel *data) :
    QWidget(parent),
    ui(new Ui::WildMonChart)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Window);

    ui->chartView->setRenderHint(QPainter::Antialiasing);

    setChartData(data);
};

WildMonChart::~WildMonChart() {
    delete ui;
};

void WildMonChart::setChartData(EncounterTableModel *data) {
    this->data = data;
    updateChart();
}

void WildMonChart::updateChart() {
    // TODO: Handle empty chart
    if (!this->data)
        return;

    const QList<double> inputValues = data->percentages();
    const QVector<WildPokemon> inputPokemon = data->encounterData().wildPokemon;

    QList<double> chartValues;
    QList<WildPokemon> chartPokemon;
    
    // Combine data for duplicate species entries
    QList<QString> seenSpecies;
    for (int i = 0; i < qMin(inputValues.length(), inputPokemon.length()); i++) {
        const double percent = inputValues.at(i);
        const WildPokemon pokemon = inputPokemon.at(i);

        int existingIndex = seenSpecies.indexOf(pokemon.species);
        if (existingIndex >= 0) {
            // Duplicate species entry
            chartValues[existingIndex] += percent;
            if (pokemon.minLevel < chartPokemon.at(existingIndex).minLevel)
                chartPokemon[existingIndex].minLevel = pokemon.minLevel;
            if (pokemon.maxLevel > chartPokemon.at(existingIndex).maxLevel)
                chartPokemon[existingIndex].maxLevel = pokemon.maxLevel;
        } else {
            // New species entry
            chartValues.append(percent);
            chartPokemon.append(pokemon);
            seenSpecies.append(pokemon.species);
        }
    }

    // TODO: If pokemon < values, fill remainder with an "empty" slice

    // Populate chart
    //const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::regex_species); // TODO: Change regex to prefix
    const QString speciesPrefix = "SPECIES_";
    QPieSeries *series = new QPieSeries();
    for (int i = 0; i < qMin(chartValues.length(), chartPokemon.length()); i++) {
        const double percent = chartValues.at(i);
        const WildPokemon pokemon = chartPokemon.at(i);
        
        // Strip 'SPECIES_' prefix
        QString name = pokemon.species;
        if (name.startsWith(speciesPrefix))
            name.remove(0, speciesPrefix.length());

        QString label = QString("%1\nLv %2").arg(name).arg(pokemon.minLevel);
        if (pokemon.minLevel != pokemon.maxLevel)
            label.append(QString("-%1").arg(pokemon.maxLevel));
        label.append(QString(" (%1%)").arg(percent * 100));

        QPieSlice *slice = new QPieSlice(label, percent);
        //slice->setLabelPosition(QPieSlice::LabelInsideNormal);
        slice->setLabelVisible();
        series->append(slice);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();

    ui->chartView->setChart(chart); // TODO: Leaking old chart

    // TODO: Draw icons onto slices
}
