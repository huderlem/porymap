#pragma once
#ifndef GUARD_WILDMONINFO_H
#define GUARD_WILDMONINFO_H

#include <QtWidgets>
#include "orderedmap.h"

struct WildPokemon {
    int minLevel = 5;
    int maxLevel = 5;
    int encounterRate = 0;
    QString species = "SPECIES_NONE";
};

struct WildMonInfo {
    bool active = true;
    int encounterRate = 0;
    QVector<QVector<WildPokemon>> wildPokemon;
};

struct WildPokemonHeader {
    QString map;
    QString baseLabel;
    tsl::ordered_map<QString, WildMonInfo> wildMons;
};

struct EncounterGroup {
    QString label;
    bool forMaps = false;
    QVector<WildPokemonHeader> encounters;
};

typedef QVector<EncounterGroup> EncounterGroups;

WildMonInfo getDefaultMonInfo(void);
WildMonInfo copyMonInfoFromTab(QTableWidget *table);

#endif // GUARD_WILDMONINFO_H
