#include "wildmoninfo.h"
#include "montabwidget.h"
#include "project.h"



WildMonInfo getDefaultMonInfo(Field field) {
    WildMonInfo newInfo;
    newInfo.active = true;
    newInfo.encounterRate = 0;

    for (int row : field.second)
        newInfo.wildPokemon.append({5, 5, "SPECIES_NONE"});

    return newInfo;
}

WildMonInfo copyMonInfoFromTab(QTableWidget *monTable) {
    WildMonInfo newInfo;
    QVector<WildPokemon> newWildMons;

    for (int row = 0; row < monTable->rowCount(); row++) {
        WildPokemon newWildMon;
        newWildMon.species = monTable->cellWidget(row, 1)->findChild<QComboBox *>()->currentText();
        newWildMon.minLevel = monTable->cellWidget(row, 2)->findChild<QSpinBox *>()->value();
        newWildMon.maxLevel = monTable->cellWidget(row, 3)->findChild<QSpinBox *>()->value();
        newWildMons.append(newWildMon);
    }
    newInfo.active = true;
    newInfo.wildPokemon = newWildMons;
    newInfo.encounterRate = monTable->findChild<QSpinBox *>()->value();

    return newInfo;
}
