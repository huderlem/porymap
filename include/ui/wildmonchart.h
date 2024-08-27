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

    virtual void closeEvent(QCloseEvent *event) override;

public slots:
    void setTable(const EncounterTableModel *table);
    void clearTable();
    void refresh();

private:
    Ui::WildMonChart *ui;
    const EncounterTableModel *table;

    QStringList groupNames;
    QStringList groupNamesReversed;
    QStringList speciesInLegendOrder;
    QMap<int, QString> tableIndexToGroupName;

    struct LevelRange {
        int min;
        int max;
    };
    QMap<QString, LevelRange> groupedLevelRanges;

    struct Summary {
        double speciesFrequency = 0.0;
        QMap<int, double> levelFrequencies;
    };
    typedef QMap<QString, Summary> GroupedData;

    QMap<QString, GroupedData> speciesToGroupedData;
    QMap<QString, QColor> speciesToColor;


    QStringList getSpeciesNamesAlphabetical() const;
    double getSpeciesFrequency(const QString&, const QString&) const;
    QMap<int, double> getLevelFrequencies(const QString &, const QString &) const;
    LevelRange getLevelRange(const QString &, const QString &) const;
    bool usesGroupLabels() const;

    void clearTableData();
    void readTable();
    QChart* createSpeciesDistributionChart();
    QChart* createLevelDistributionChart();
    QBarSet* createLevelDistributionBarSet(const QString &, const QString &, bool);
    void refreshSpeciesDistributionChart();
    void refreshLevelDistributionChart();

    void saveSpeciesColors(const QList<QBarSet*> &);
    void applySpeciesColors(const QList<QBarSet*> &);
    QChart::ChartTheme currentTheme() const;
    void updateTheme();
    void limitChartAnimation(QChart*);

    void showHelpDialog();
};

#endif // WILDMONCHART_H
