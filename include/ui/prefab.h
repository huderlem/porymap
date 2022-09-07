#ifndef PREFAB_H
#define PREFAB_H

#include "ui/metatileselector.h"
#include "map.h"

#include <QString>
#include <QLabel>

struct PrefabItem
{
    QString name;
    QString primaryTileset;
    QString secondaryTileset;
    MetatileSelection selection;
};

class Prefab
{
public:
    void initPrefabUI(MetatileSelector *selector, QWidget *prefabWidget, QLabel *emptyPrefabLabel, QString primaryTileset, QString secondaryTileset, Map *map);

private:
    QList<PrefabItem> items;
    void loadPrefabs();
    QList<PrefabItem> getPrefabsForTilesets(QString primaryTileset, QString secondaryTileset);
};

extern Prefab prefab;

#endif // PREFAB_H
