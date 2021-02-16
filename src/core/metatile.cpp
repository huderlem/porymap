#include "metatile.h"
#include "tileset.h"
#include "project.h"

Metatile::Metatile() :
    behavior(0),
    layerType(0),
    encounterType(0),
    terrainType(0)
{  }

Metatile::Metatile(const Metatile &other) :
    tiles(other.tiles),
    behavior(other.behavior),
    layerType(other.layerType),
    encounterType(other.encounterType),
    terrainType(other.terrainType),
    label(other.label)
{  }

Metatile* Metatile::copy() {
    Metatile *copy = new Metatile;
    copy->behavior = this->behavior;
    copy->layerType = this->layerType;
    copy->encounterType = this->encounterType;
    copy->terrainType = this->terrainType;
    copy->label = this->label;
    for (const Tile &tile : this->tiles) {
        copy->tiles.append(tile);
    }
    return copy;
}

void Metatile::copyInPlace(Metatile *other) {
    this->behavior = other->behavior;
    this->layerType = other->layerType;
    this->encounterType = other->encounterType;
    this->terrainType = other->terrainType;
    this->label = other->label;
    for (int i = 0; i < this->tiles.length(); i++) {
        this->tiles[i] = other->tiles.at(i);
    }
}

int Metatile::getBlockIndex(int index) {
    if (index < Project::getNumMetatilesPrimary()) {
        return index;
    } else {
        return index - Project::getNumMetatilesPrimary();
    }
}

QPoint Metatile::coordFromPixmapCoord(const QPointF &pixelCoord) {
    int x = static_cast<int>(pixelCoord.x()) / 16;
    int y = static_cast<int>(pixelCoord.y()) / 16;
    return QPoint(x, y);
}
