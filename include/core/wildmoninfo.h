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
    int encounterRate;
    QVector<WildPokemon> wildPokemon;
};

// for extensibility, make a QVector<WildMonInfo>
// or QMap<QString, WildMonInfo>
struct WildPokemonHeader {
    WildMonInfo landMons;
    WildMonInfo waterMons;
    WildMonInfo rockSmashMons;
    WildMonInfo fishingMons;

    QMap<QString, WildMonInfo> wildMons;
};

class Project;
QWidget *newSpeciesTableEntry(Project *project, WildPokemon mon, int index);
void createSpeciesTableRow(Project *, QTableWidget *, WildPokemon, int, QString);
void clearTabWidget(QLayout *tab);
void clearTable(QTableWidget *table);
void populateWildMonTabWidget(QTabWidget *tabWidget, QVector<QPair<QString, QVector<int>>> fields);

#endif // GUARD_WILDMONINFO_H
