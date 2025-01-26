#pragma once
#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>
#include "orderedmap.h"

class WildPokemon {
public:
    WildPokemon();
    WildPokemon(int minLevel, int maxLevel, const QString &species);

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
    tsl::ordered_map<QString, WildMonInfo> wildMons;
};

struct EncounterField {
    QString name; // Ex: "fishing_mons"
    QVector<int> encounterRates;
    tsl::ordered_map<QString, QVector<int>> groups; // Ex: "good_rod", {2, 3, 4}
};

typedef QVector<EncounterField> EncounterFields;

void setDefaultEncounterRate(QString fieldName, int rate);
WildMonInfo getDefaultMonInfo(const EncounterField &field);
QVector<double> getWildEncounterPercentages(const EncounterField &field);
void combineEncounters(WildMonInfo &to, WildMonInfo from);

#endif // GUARD_WILDMONINFO_H
