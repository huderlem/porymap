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

struct EncounterField {
    QString name;
    QVector<int> encounterRates;
    QMap<QString, QVector<int>> groups;
};

typedef QVector<EncounterField> EncounterFields;

WildMonInfo getDefaultMonInfo(EncounterField field);
WildMonInfo copyMonInfoFromTab(QTableWidget *table, EncounterField monField);

#endif // GUARD_WILDMONINFO_H
