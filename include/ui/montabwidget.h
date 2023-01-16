#ifndef MONTABWIDGET_H
#define MONTABWIDGET_H

#include "wildmoninfo.h"

#include <QtWidgets>
#include <QVector>

class Editor;

struct MonTableEntry: WildPokemon {
    QVector<double> encounterRateByTime;

    // Constructor from a WildPokemon
    MonTableEntry(WildPokemon mon, int timeCount = 2) {
        species = mon.species;
        minLevel = mon.minLevel;
        maxLevel = mon.maxLevel;
        encounterRate = mon.encounterRate;
        encounterRateByTime = QVector<double>(timeCount, 0);
    }
};

class MonTabWidget : public QTabWidget {

    Q_OBJECT

public:
    explicit MonTabWidget(Editor *editor = nullptr, QWidget *parent = nullptr);
    ~MonTabWidget(){};

    void populate();
    void populateTab(int tabIndex, WildMonInfo &monInfo, QString fieldName);
    void clear();

    void createSpeciesTableRow(QTableWidget *table, MonTableEntry mon, int index, QString fieldName);

    void clearTableAt(int index);

    QTableWidget *tableAt(int tabIndex);

public slots:
    void setTabActive(int index, bool active = true);

private:
    bool eventFilter(QObject *object, QEvent *event);
    void askActivateTab(int tabIndex, QPoint menuPos);

    QVector<bool> activeTabs;

    Editor *editor;
};

#endif // MONTABWIDGET_H
