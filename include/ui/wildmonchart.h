#ifndef WILDMONCHART_H
#define WILDMONCHART_H

#include "encountertablemodel.h"

#include <QWidget>

namespace Ui {
class WildMonChart;
}

class WildMonChart : public QWidget
{
    Q_OBJECT
public:
    explicit WildMonChart(QWidget *parent, EncounterTableModel *data);
    ~WildMonChart();

public slots:
    void setChartData(EncounterTableModel *data);
    void updateChart();

private:
    Ui::WildMonChart *ui;
    EncounterTableModel *data;
};

#endif // WILDMONCHART_H
