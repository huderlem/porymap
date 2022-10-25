#include "metatile.h"
#include "tileset.h"
#include "project.h"

uint32_t Metatile::behaviorMask = 0;
uint32_t Metatile::terrainTypeMask = 0;
uint32_t Metatile::encounterTypeMask = 0;
uint32_t Metatile::layerTypeMask = 0;
uint32_t Metatile::unusedAttrMask = 0;

int Metatile::behaviorShift = 0;
int Metatile::terrainTypeShift = 0;
int Metatile::encounterTypeShift = 0;
int Metatile::layerTypeShift = 0;

Metatile::Metatile() :
    behavior(0),
    layerType(0),
    encounterType(0),
    terrainType(0),
    unusedAttributes(0)
{  }

Metatile::Metatile(const int numTiles) :
    behavior(0),
    layerType(0),
    encounterType(0),
    terrainType(0),
    unusedAttributes(0)
{
    Tile tile = Tile();
    for (int i = 0; i < numTiles; i++) {
        this->tiles.append(tile);
    }
}

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

// Returns the position of the rightmost set bit
int Metatile::getShiftValue(uint32_t mask) {
    return log2(mask & ~(mask - 1));;
}


void Metatile::calculateAttributeLayout() {
    // Get the maximum size of any attribute mask
    const QHash<int, uint32_t> maxMasks = {
        {1, 0xFF},
        {2, 0xFFFF},
        {4, 0xFFFFFFFF},
    };
    const uint32_t maxMask = maxMasks.value(projectConfig.getMetatileAttributesSize(), 0);

    // Behavior
    uint32_t mask = projectConfig.getMetatileBehaviorMask();
    if (mask > maxMask) {
        logWarn(QString("Metatile behavior mask '%1' exceeds maximum size '%2'").arg(mask).arg(maxMask));
        mask &= maxMask;
    }
    Metatile::behaviorMask = mask;
    Metatile::behaviorShift = getShiftValue(mask);

    // Terrain Type
    mask = projectConfig.getMetatileTerrainTypeMask();
    if (mask > maxMask) {
        logWarn(QString("Metatile terrain type mask '%1' exceeds maximum size '%2'").arg(mask).arg(maxMask));
        mask &= maxMask;
    }
    Metatile::terrainTypeMask = mask;
    Metatile::terrainTypeShift = getShiftValue(mask);

    // Encounter Type
    mask = projectConfig.getMetatileEncounterTypeMask();
    if (mask > maxMask) {
        logWarn(QString("Metatile encounter type mask '%1' exceeds maximum size '%2'").arg(mask).arg(maxMask));
        mask &= maxMask;
    }
    Metatile::encounterTypeMask = mask;
    Metatile::encounterTypeShift = getShiftValue(mask);

    // Layer Type
    mask = projectConfig.getMetatileLayerTypeMask();
    if (mask > maxMask) {
        logWarn(QString("Metatile layer type mask '%1' exceeds maximum size '%2'").arg(mask).arg(maxMask));
        mask &= maxMask;
    }
    Metatile::layerTypeMask = mask;
    Metatile::layerTypeShift = getShiftValue(mask);

    Metatile::unusedAttrMask = ~(Metatile::behaviorMask | Metatile::terrainTypeMask | Metatile::layerTypeMask | Metatile::encounterTypeMask);
    Metatile::unusedAttrMask &= maxMask;

    // Warn user if any mask overlaps
    if (Metatile::behaviorMask & Metatile::terrainTypeMask
     || Metatile::behaviorMask & Metatile::encounterTypeMask
     || Metatile::behaviorMask & Metatile::layerTypeMask
     || Metatile::terrainTypeMask & Metatile::encounterTypeMask
     || Metatile::terrainTypeMask & Metatile::layerTypeMask
     || Metatile::encounterTypeMask & Metatile::layerTypeMask) {
        logWarn("Metatile attribute masks are overlapping.");
    }
}

uint32_t Metatile::getAttributes() {
    uint32_t attributes = this->unusedAttributes & Metatile::unusedAttrMask;
    attributes |= (behavior << Metatile::behaviorShift) & Metatile::behaviorMask;
    attributes |= (terrainType << Metatile::terrainTypeShift) & Metatile::terrainTypeMask;
    attributes |= (encounterType << Metatile::encounterTypeShift) & Metatile::encounterTypeMask;
    attributes |= (layerType << Metatile::layerTypeShift) & Metatile::layerTypeMask;
    return attributes;
}

void Metatile::setAttributes(uint32_t data) {
    this->behavior = (data & Metatile::behaviorMask) >> Metatile::behaviorShift;
    this->terrainType = (data & Metatile::terrainTypeMask) >> Metatile::terrainTypeShift;
    this->encounterType = (data & Metatile::encounterTypeMask) >> Metatile::encounterTypeShift;
    this->layerType = (data & Metatile::layerTypeMask) >> Metatile::layerTypeShift;
    this->unusedAttributes = data & Metatile::unusedAttrMask;
}

// Get the vanilla attribute sizes based on version. For AdvanceMap import
int Metatile::getAttributesSize(BaseGameVersion version) {
    return (version == BaseGameVersion::pokefirered) ? 4 : 2;
}

// Set the attributes using the vanilla layout based on version. For AdvanceMap import
void Metatile::setAttributes(uint32_t data, BaseGameVersion version) {
    if (version == BaseGameVersion::pokefirered) {
        const uint32_t behaviorMask      = 0x000001FF;
        const uint32_t terrainTypeMask   = 0x00003E00;
        const uint32_t encounterTypeMask = 0x07000000;
        const uint32_t layerTypeMask     = 0x60000000;

        this->behavior = data & behaviorMask;
        this->terrainType = (data & terrainTypeMask) >> 9;
        this->encounterType = (data & encounterTypeMask) >> 24;
        this->layerType = (data & layerTypeMask) >> 29;
        this->unusedAttributes = data & ~(behaviorMask | terrainTypeMask | layerTypeMask | encounterTypeMask);
    } else {
        const uint16_t behaviorMask  = 0x00FF;
        const uint16_t layerTypeMask = 0xF000;

        this->behavior = data & behaviorMask;
        this->layerType = (data & layerTypeMask) >> 12;
        this->unusedAttributes = data & ~(behaviorMask | layerTypeMask);
    }

    // Clean data to fit the user's custom masks
    this->setAttributes(this->getAttributes());
}
