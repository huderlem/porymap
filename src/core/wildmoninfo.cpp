//

#include "wildmoninfo.h"
#include "montabwidget.h"
#include "project.h"



// TODO: remove this necessity
static QMap<int, QString> landPercentages = QMap<int, QString>({
    {1, "20"}, {2, "20"},
    {3, "10"}, {4, "10"}, {5, "10"}, {6, "10"},
    {7, "5"}, {8, "5"},
    {9, "4"}, {10, "4"},
    {11, "1"}, {12, "1"}
});

void clearTabWidget(QLayout *tab) {
    QLayoutItem *item = tab->itemAt(0);
    if (item) tab->removeItem(item);
}

WildMonInfo getDefaultMonInfo(Field field) {
    WildMonInfo newInfo;
    newInfo.active = true;
    newInfo.encounterRate = 0;

    for (int row : field.second)
        newInfo.wildPokemon.append({5, 5, "SPECIES_NONE"});

    return newInfo;
}


