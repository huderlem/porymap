#ifndef PREFAB_H
#define PREFAB_H

#include "ui/metatileselector.h"
#include "map.h"

#include <QString>
#include <QLabel>
#include <QUuid>
#include <QPointer>

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
    void initPrefabUI(QPointer<MetatileSelector> selector, QPointer<QWidget> prefabWidget, QPointer<QLabel> emptyPrefabLabel, Layout *layout);
    void addPrefab(MetatileSelection selection, Layout *layout, QString name);
    void updatePrefabUi(QPointer<Layout> layout);
    void clearPrefabUi();
    bool tryImportDefaultPrefabs(QWidget * parent, BaseGameVersion version, QString filepath = "");

private:
    QPointer<MetatileSelector> selector;
    QPointer<QWidget> prefabWidget;
    QPointer<QLabel> emptyPrefabLabel;
    QList<PrefabItem> items;
    void loadPrefabs();
    void savePrefabs();
    QList<PrefabItem> getPrefabsForTilesets(QString primaryTileset, QString secondaryTileset);
};

extern Prefab prefab;

#endif // PREFAB_H
