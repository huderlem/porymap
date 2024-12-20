#pragma once
#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>
#include "orderedmap.h"

struct WildPokemon {
    int minLevel = 5;
    int maxLevel = 5;
    QString species = "SPECIES_NONE"; // TODO: Use define_species_prefix
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
WildMonInfo getDefaultMonInfo(EncounterField field);
void combineEncounters(WildMonInfo &to, WildMonInfo from);

#endif // GUARD_WILDMONINFO_H
