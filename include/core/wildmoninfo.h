#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>

struct WildPokemon {
    int minLevel = 5;
    int maxLevel = 5;
    QString species = "SPECIES_NONE";
};

struct WildMonInfo {
    bool active = false;
    int encounterRate = 0;
    QVector<WildPokemon> wildPokemon;
};

struct WildPokemonHeader {
    QMap<QString, WildMonInfo> wildMons;
};

typedef QVector<QPair<QString, QVector<int>>> Fields;
typedef QPair<QString, QVector<int>> Field;

WildMonInfo getDefaultMonInfo(Field field);
WildMonInfo copyMonInfoFromTab(QTableWidget *table);

#endif // GUARD_WILDMONINFO_H
