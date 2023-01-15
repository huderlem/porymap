#include "wildmoninfo.h"
#include "montabwidget.h"



WildMonInfo getDefaultMonInfo(EncounterField field) {
    WildMonInfo newInfo;
    newInfo.active = true;
    newInfo.encounterRate = 0;

    int size = field.encounterRates.size();
    while (size--)
        newInfo.wildPokemon.append(WildPokemon());

    return newInfo;
}
