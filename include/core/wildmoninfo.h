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

struct WildPokemonHeader {
    WildMonInfo landMons;
    WildMonInfo waterMons;
    WildMonInfo rockSmashMons;
    WildMonInfo fishingMons;
};

class Project;
QWidget *newSpeciesTableEntry(Project *project, WildPokemon mon, int index);
void createSpeciesTableRow(Project *, QTableWidget *, WildPokemon, int);
void clearTabWidget(QLayout *tab);

#endif // GUARD_WILDMONINFO_H
