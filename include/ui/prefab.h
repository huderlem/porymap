#ifndef PREFAB_H
#define PREFAB_H

#include "ui/metatileselector.h"
#include "map.h"

#include <QString>
#include <QLabel>
#include <QUuid>

struct PrefabItem
{
    QUuid id;
    QString name;
    QString primaryTileset;
    QString secondaryTileset;
    MetatileSelection selection;
};

class Prefab
{
public:
    void initPrefabUI(MetatileSelector *selector, QWidget *prefabWidget, QLabel *emptyPrefabLabel, Layout *layout);
    void addPrefab(MetatileSelection selection, Layout *layout, QString name);
    void updatePrefabUi(Layout *layout);
    bool tryImportDefaultPrefabs(QWidget * parent, BaseGameVersion version, QString filepath = "");

private:
    MetatileSelector *selector;
    QWidget *prefabWidget;
    QLabel *emptyPrefabLabel;
    QList<PrefabItem> items;
    void loadPrefabs();
    void savePrefabs();
    QList<PrefabItem> getPrefabsForTilesets(QString primaryTileset, QString secondaryTileset);
};

extern Prefab prefab;

#endif // PREFAB_H
