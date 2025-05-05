#pragma once
#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>
#include "orderedjson.h"

class WildPokemon {
public:
    WildPokemon();
    WildPokemon(int minLevel, int maxLevel, const QString &species);

    int minLevel;
    int maxLevel;
    QString species;
    OrderedJson::object customData;
};

struct WildMonInfo {
    bool active = false;
    int encounterRate = 0;
    QVector<WildPokemon> wildPokemon;
    OrderedJson::object customData;
};

struct WildPokemonHeader {
    OrderedMap<QString, WildMonInfo> wildMons;
    OrderedJson::object customData;
};

struct EncounterField {
    QString name; // Ex: "fishing_mons"
    QVector<int> encounterRates;
    OrderedMap<QString, QVector<int>> groups; // Ex: "good_rod", {2, 3, 4}
    OrderedJson::object customData;
};

typedef QVector<EncounterField> EncounterFields;

void setDefaultEncounterRate(QString fieldName, int rate);
WildMonInfo getDefaultMonInfo(const EncounterField &field);
QVector<double> getWildEncounterPercentages(const EncounterField &field);
void combineEncounters(WildMonInfo &to, WildMonInfo from);

#endif // GUARD_WILDMONINFO_H
