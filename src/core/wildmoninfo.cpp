#include "wildmoninfo.h"
#include "montabwidget.h"
#include "project.h"

WildPokemon::WildPokemon(int minLevel, int maxLevel, const QString &species)
    : minLevel(minLevel),
      maxLevel(maxLevel),
      species(species)
{}

WildPokemon::WildPokemon() : WildPokemon(5, 5, Project::getEmptySpeciesName())
{}

QMap<QString, int> defaultEncounterRates;
void setDefaultEncounterRate(QString fieldName, int rate) {
    defaultEncounterRates[fieldName] = rate;
}

WildMonInfo getDefaultMonInfo(const EncounterField &field) {
    WildMonInfo newInfo;
    newInfo.active = true;
    newInfo.encounterRate = defaultEncounterRates.value(field.name, 1);

    int size = field.encounterRates.size();
    while (size--)
        newInfo.wildPokemon.append(WildPokemon());

    return newInfo;
}

QVector<double> getWildEncounterPercentages(const EncounterField &field) {
    QVector<double> percentages(field.encounterRates.size(), 0);

    if (!field.groups.empty()) {
        // This encounter field is broken up into groups (e.g. for fishing rod types).
        // Each group's percentages will be relative to the group total, not the overall total.
        for (auto groupKeyPair : field.groups) {
            int groupTotal = 0;
            for (int slot : groupKeyPair.second) {
                groupTotal += field.encounterRates.value(slot, 0);
            }
            if (groupTotal != 0) {
                for (int slot : groupKeyPair.second) {
                    percentages[slot] = static_cast<double>(field.encounterRates.value(slot, 0)) / static_cast<double>(groupTotal);
                }
            }
        }
    } else {
        // This encounter field has a single group, percentages are relative to the overall total.
        int groupTotal = 0;
        for (int chance : field.encounterRates) {
            groupTotal += chance;
        }
        if (groupTotal != 0) {
            for (int slot = 0; slot < percentages.count(); slot++) {
                percentages[slot] = static_cast<double>(field.encounterRates.value(slot, 0)) / static_cast<double>(groupTotal);
            }
        }
    }
    return percentages;
}

void combineEncounters(WildMonInfo &to, WildMonInfo from) {
    to.encounterRate = from.encounterRate;

    if (to.wildPokemon.size() == from.wildPokemon.size()) {
        to.wildPokemon = from.wildPokemon;
    }
    else if (to.wildPokemon.size() > from.wildPokemon.size()) {
        to.wildPokemon = from.wildPokemon + to.wildPokemon.mid(from.wildPokemon.size());
    }
    else {
        to.wildPokemon = from.wildPokemon.mid(0, to.wildPokemon.size());
    }
}
