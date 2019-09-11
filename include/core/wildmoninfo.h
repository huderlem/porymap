#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>

struct WildPokemon {
    int minLevel;
    int maxLevel;
    QString species;
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
