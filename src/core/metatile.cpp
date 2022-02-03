#include "metatile.h"
#include "tileset.h"
#include "project.h"

Metatile::Metatile() :
    behavior(0),
    layerType(0),
    encounterType(0),
    terrainType(0),
    unusedAttributes(0)
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

int Metatile::getAttributesSize(BaseGameVersion version) {
    return (version == BaseGameVersion::pokefirered) ? 4 : 2;
}

// RSE attributes
const uint16_t behaviorMask_RSE  = 0x00FF;
const uint16_t layerTypeMask_RSE = 0xF000;
const int behaviorShift_RSE = 0;
const int layerTypeShift_RSE = 12;

// FRLG attributes
const uint32_t behaviorMask_FRLG  = 0x000001FF;
const uint32_t terrainTypeMask    = 0x00003E00;
const uint32_t encounterTypeMask  = 0x07000000;
const uint32_t layerTypeMask_FRLG = 0x60000000;
const int behaviorShift_FRLG = 0;
const int terrainTypeShift = 9;
const int encounterTypeShift = 24;
const int layerTypeShift_FRLG = 29;

uint32_t Metatile::getAttributes(BaseGameVersion version) {
    uint32_t attributes = this->unusedAttributes;
    if (version == BaseGameVersion::pokefirered) {
        attributes |= (behavior << behaviorShift_FRLG) & behaviorMask_FRLG;
        attributes |= (terrainType << terrainTypeShift) & terrainTypeMask;
        attributes |= (encounterType << encounterTypeShift) & encounterTypeMask;
        attributes |= (layerType << layerTypeShift_FRLG) & layerTypeMask_FRLG;
    } else {
        attributes |= (behavior << behaviorShift_RSE) & behaviorMask_RSE;
        attributes |= (layerType << layerTypeShift_RSE) & layerTypeMask_RSE;
    }
    return attributes;
}

void Metatile::setAttributes(uint32_t data, BaseGameVersion version) {
    if (version == BaseGameVersion::pokefirered) {
        this->behavior = (data & behaviorMask_FRLG) >> behaviorShift_FRLG;
        this->terrainType = (data & terrainTypeMask) >> terrainTypeShift;
        this->encounterType = (data & encounterTypeMask) >> encounterTypeShift;
        this->layerType = (data & layerTypeMask_FRLG) >> layerTypeShift_FRLG;
        this->unusedAttributes = data & ~(behaviorMask_FRLG | terrainTypeMask | layerTypeMask_FRLG | encounterTypeMask);
    } else {
        this->behavior = (data & behaviorMask_RSE) >> behaviorShift_RSE;
        this->layerType = (data & layerTypeMask_RSE) >> layerTypeShift_RSE;
        this->unusedAttributes = data & ~(behaviorMask_RSE | layerTypeMask_RSE);
    }
}
