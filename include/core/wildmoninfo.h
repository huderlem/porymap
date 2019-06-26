#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>

struct WildPokemon {
    int minLevel;
    int maxLevel;
    QString species;
};

struct WildMonInfo {
    //
    bool active = false;
    int encounterRate = 0;
    QVector<WildPokemon> wildPokemon;
};

// for extensibility, make a QVector<WildMonInfo>
// or QMap<QString, WildMonInfo>
struct WildPokemonHeader {
    QMap<QString, WildMonInfo> wildMons;
};

typedef QVector<QPair<QString, QVector<int>>> Fields;
typedef QPair<QString, QVector<int>> Field;

//class Project;
//class MonTabWidget;
//QWidget *newSpeciesTableEntry(Project *project, WildPokemon mon, int index);
//void createSpeciesTableRow(Project *, QTableWidget *, WildPokemon, int, QString);
void clearTabWidget(QLayout *tab);
//void clearTable(QTableWidget *table);
//void populateWildMonTabWidget(MonTabWidget *tabWidget, Fields /* QVector<QPair<QString, QVector<int>>> */ fields);

WildMonInfo getDefaultMonInfo(Field field);

#endif // GUARD_WILDMONINFO_H
