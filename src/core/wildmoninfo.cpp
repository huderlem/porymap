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

WildMonInfo copyMonInfoFromTab(QTableWidget *monTable, EncounterField monField) {
    WildMonInfo newInfo;
    QVector<WildPokemon> newWildMons;

    bool extraColumn = !monField.groups.empty();
    for (int row = 0; row < monTable->rowCount(); row++) {
        WildPokemon newWildMon;
        newWildMon.species = monTable->cellWidget(row, extraColumn ? 2 : 1)->findChild<QComboBox *>()->currentText();
        newWildMon.minLevel = monTable->cellWidget(row, extraColumn ? 3 : 2)->findChild<QSpinBox *>()->value();
        newWildMon.maxLevel = monTable->cellWidget(row, extraColumn ? 4 : 3)->findChild<QSpinBox *>()->value();
        newWildMons.append(newWildMon);
    }
    newInfo.active = true;
    newInfo.wildPokemon = newWildMons;
    newInfo.encounterRate = monTable->findChild<QSpinBox *>()->value();

    return newInfo;
}
