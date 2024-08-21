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
    explicit WildMonChart(QWidget *parent, EncounterTableModel *table);
    ~WildMonChart();

public slots:
    void setTable(EncounterTableModel *table);
    void updateChart();

private:
    Ui::WildMonChart *ui;
    EncounterTableModel *table;
};

#endif // WILDMONCHART_H
