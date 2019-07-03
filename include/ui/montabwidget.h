#ifndef MONTABWIDGET_H
#define MONTABWIDGET_H

#include "wildmoninfo.h"

#include <QtWidgets>
#include <QVector>

class Project;

class MonTabWidget : public QTabWidget {

    Q_OBJECT

public:
    explicit MonTabWidget(Project *project = nullptr, QWidget *parent = nullptr);
    ~MonTabWidget(){};

    void populate();
    void populateTab(int tabIndex, WildMonInfo monInfo, QString fieldName);
    void clear();

    void createSpeciesTableRow(QTableWidget *table, WildPokemon mon, int index, QString fieldName);

    void clearTableAt(int index);

    QTableWidget *tableAt(int tabIndex);

public slots:
    void setTabActive(int index, bool active = true);

private:
    bool eventFilter(QObject *object, QEvent *event);
    void askActivateTab(int tabIndex, QPoint menuPos);

    QVector<bool> activeTabs;

    Project *project;
};

#endif // MONTABWIDGET_H
