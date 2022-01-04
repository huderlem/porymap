#include "metatile.h"
#include "tileset.h"
#include "project.h"

Metatile::Metatile() :
    behavior(0),
    layerType(0),
    encounterType(0),
    terrainType(0)
{  }

int Metatile::getIndexInTileset(int metatileId) {
    if (metatileId < Project::getNumMetatilesPrimary()) {
        return metatileId;
    } else {
        return metatileId - Project::getNumMetatilesPrimary();
    }
}

QPoint Metatile::coordFromPixmapCoord(const QPointF &pixelCoord) {
    int x = static_cast<int>(pixelCoord.x()) / 16;
    int y = static_cast<int>(pixelCoord.y()) / 16;
    return QPoint(x, y);
}
