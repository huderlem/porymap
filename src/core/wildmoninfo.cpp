#include "wildmoninfo.h"
#include "montabwidget.h"



WildMonInfo getDefaultMonInfo(void) {
    WildMonInfo newInfo;
    newInfo.active = true;
    newInfo.encounterRate = 0;

    for (QVector<WildPokemon> wildPokemon : newInfo.wildPokemon) {
        int size = 15;
        while (size--)
            wildPokemon.append(WildPokemon());
    }

    return newInfo;
}

WildMonInfo copyMonInfoFromTab(QTableWidget *monTable) {
    WildMonInfo newInfo;
    QVector<QVector<WildPokemon>> newTimes;
    int numTimes = monTable->columnCount() - 5;
    for (int time = 0; time < numTimes; time++) {
        QVector<WildPokemon> newWildMons;

        for (int row = 0; row < monTable->rowCount(); row++) {
            
            WildPokemon newWildMon;
            int colCount = monTable->columnCount();
            auto four = monTable->cellWidget(row, 4);
            newWildMon.species = monTable->cellWidget(row, 1)->findChild<QComboBox *>()->currentText();
            if (newWildMon.species == "SPECIES_NONE") continue;
            newWildMon.minLevel = monTable->cellWidget(row, 2)->findChild<QSpinBox *>()->value();
            newWildMon.maxLevel = monTable->cellWidget(row, 3)->findChild<QSpinBox *>()->value();
            newWildMon.encounterRate = monTable->cellWidget(row, 4 + time)->findChild<QDoubleSpinBox *>()->value() * (255.0 / 100.0);
            if (newWildMon.encounterRate > 0.001) {
                newWildMons.append(newWildMon);
            }
        }
        if (!newWildMons.empty()) {
            // Ensure that the encounter rate adds up to 256
            int totalEncounterRate = 0;
            for (WildPokemon mon : newWildMons) {
                totalEncounterRate += mon.encounterRate;
            }
            for (int i = 0; totalEncounterRate < 256; i = (i + 1) % newWildMons.size()) {
                newWildMons[i].encounterRate++;
                totalEncounterRate++;
            }
            for (int i = 0; totalEncounterRate > 256; i = (i + 1) % newWildMons.size()) {
                newWildMons[i].encounterRate--;
                totalEncounterRate--;
            }
        }
        newTimes.append(newWildMons);
    }
    newInfo.active = true;
    newInfo.wildPokemon = newTimes;
    newInfo.encounterRate = monTable->findChild<QSpinBox *>()->value();

    return newInfo;
}
