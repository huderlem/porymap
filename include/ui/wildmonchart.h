#ifndef WILDMONCHART_H
#define WILDMONCHART_H

#include "encountertablemodel.h"

#include <QWidget>
#include <QtCharts>

namespace Ui {
class WildMonChart;
}

class WildMonChart : public QWidget
{
    Q_OBJECT
public:
    explicit WildMonChart(QWidget *parent, const EncounterTableModel *table);
    ~WildMonChart();

public slots:
    void setTable(const EncounterTableModel *table);
    void createCharts();

private:
    Ui::WildMonChart *ui;
    const EncounterTableModel *table;

    QStringList groupNames;
    QMap<int, QString> tableIndexToGroupName;

    struct Summary {
        double speciesFrequency = 0.0;
        QMap<int, double> levelFrequencies;
    };

    int tableMinLevel;
    int tableMaxLevel;

    // GroupedData maps a group name ("old_rod", "good_rod"...)
    // to any summarized data needed for the charts.
    typedef QMap<QString, Summary> GroupedData;

    QMap<QString, GroupedData> speciesToGroupedData;

    QStringList getSpeciesNames() const;
    double getSpeciesFrequency(const QString&, const QString&) const;
    QMap<int, double> getLevelFrequencies(const QString &, const QString &) const;
    bool usesGroupLabels() const;

    void clearTableData();
    void readTable();
    void createSpeciesDistributionChart();
    void createLevelDistributionChart();

    void stopChartAnimation();
};

#endif // WILDMONCHART_H
