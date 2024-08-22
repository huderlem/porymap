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
    explicit WildMonChart(QWidget *parent, const EncounterTableModel *table);
    ~WildMonChart();

public slots:
    void setTable(const EncounterTableModel *table);
    void updateChart();

private:
    Ui::WildMonChart *ui;
    const EncounterTableModel *table;
};

#endif // WILDMONCHART_H
