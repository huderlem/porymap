#include "metatile.h"
#include "tileset.h"
#include "project.h"

const QHash<QString, MetatileAttr> Metatile::defaultLayoutFRLG = {
    {"behavior",      MetatileAttr(0x000001FF, 0) },
    {"terrainType",   MetatileAttr(0x00003E00, 9) },
    {"encounterType", MetatileAttr(0x07000000, 24) },
    {"layerType",     MetatileAttr(0x60000000, 29) },
};

const QHash<QString, MetatileAttr> Metatile::defaultLayoutRSE = {
    {"behavior",      MetatileAttr(0x00FF, 0) },
    {"terrainType",   MetatileAttr() },
    {"encounterType", MetatileAttr() },
    {"layerType",     MetatileAttr(0xF000, 12) },
};

const QHash<BaseGameVersion, const QHash<QString, MetatileAttr>*> Metatile::defaultLayouts = {
    { BaseGameVersion::pokeruby,    &defaultLayoutRSE },
    { BaseGameVersion::pokefirered, &defaultLayoutFRLG },
    { BaseGameVersion::pokeemerald, &defaultLayoutRSE },
};

MetatileAttr Metatile::behaviorAttr;
MetatileAttr Metatile::terrainTypeAttr;
MetatileAttr Metatile::encounterTypeAttr;
MetatileAttr Metatile::layerTypeAttr;

uint32_t Metatile::unusedAttrMask = 0;

MetatileAttr::MetatileAttr() :
    mask(0),
    shift(0)
{  }

MetatileAttr::MetatileAttr(uint32_t mask, int shift) :
    mask(mask),
    shift(shift)
{  }

Metatile::Metatile() :
    behavior(0),
    terrainType(0),
    encounterType(0),
    layerType(0),
    unusedAttributes(0)
{  }

Metatile::Metatile(const int numTiles) :
    behavior(0),
    terrainType(0),
    encounterType(0),
    layerType(0),
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

// Set the layout of a metatile attribute using the mask read from the config file
void Metatile::setCustomAttributeLayout(MetatileAttr * attr, uint32_t mask, uint32_t max) {
    if (mask > max) {
        uint32_t oldMask = mask;
        mask &= max;
        logWarn(QString("Metatile attribute mask '0x%1' has been truncated to '0x%2'")
                .arg(QString::number(oldMask, 16).toUpper())
                .arg(QString::number(mask, 16).toUpper()));
    }
    attr->mask = mask;
    attr->shift = mask ? log2(mask & ~(mask - 1)) : 0; // Get position of the least significant set bit
}

// For checking whether a metatile attribute mask can contain all the available hard-coded options
bool Metatile::isMaskTooSmall(MetatileAttr * attr, int max) {
    if (attr->mask == 0 || max <= 0) return false;

    // Get position of the most significant set bit
    uint32_t n = log2(max);

    // Get a mask for all values 0 to max.
    // This may fail for n > 30, but that's not possible here.
    uint32_t rangeMask = (1 << (n + 1)) - 1;

    return attr->getClamped(rangeMask) != rangeMask;
}

bool Metatile::doMasksOverlap(QList<uint32_t> masks) {
    for (int i = 0; i < masks.length(); i++)
    for (int j = i + 1; j < masks.length(); j++) {
        if (masks.at(i) & masks.at(j))
            return true;
    }
    return false;
}

void Metatile::setCustomLayout(Project * project) {
    // Get the maximum size of any attribute mask
    const QHash<int, uint32_t> maxMasks = {
        {1, 0xFF},
        {2, 0xFFFF},
        {4, 0xFFFFFFFF},
    };
    const uint32_t maxMask = maxMasks.value(projectConfig.getMetatileAttributesSize(), 0);

    // Set custom attribute masks from the config file
    setCustomAttributeLayout(&Metatile::behaviorAttr, projectConfig.getMetatileBehaviorMask(), maxMask);
    setCustomAttributeLayout(&Metatile::terrainTypeAttr, projectConfig.getMetatileTerrainTypeMask(), maxMask);
    setCustomAttributeLayout(&Metatile::encounterTypeAttr, projectConfig.getMetatileEncounterTypeMask(), maxMask);
    setCustomAttributeLayout(&Metatile::layerTypeAttr, projectConfig.getMetatileLayerTypeMask(), maxMask);

    // Set mask for preserving any attribute bits not used by Porymap
    Metatile::unusedAttrMask = ~(getBehaviorMask() | getTerrainTypeMask() | getEncounterTypeMask() | getLayerTypeMask());
    Metatile::unusedAttrMask &= maxMask;

    // Overlapping masks are technically ok, but probably not intended.
    // Additionally, Porymap will not properly reflect that the values are linked.
    if (doMasksOverlap({getBehaviorMask(), getTerrainTypeMask(), getEncounterTypeMask(), getLayerTypeMask()})) {
        logWarn("Metatile attribute masks are overlapping. This may result in unexpected attribute values.");
    }

    // Warn the user if they have set a nonzero mask that is too small to contain its available options.
    // They'll be allowed to select the options, but they'll be truncated to a different value when revisited.
    if (!project->metatileBehaviorMapInverse.isEmpty()) {
        int maxBehavior = project->metatileBehaviorMapInverse.lastKey();
        if (isMaskTooSmall(&Metatile::behaviorAttr, maxBehavior))
            logWarn(QString("Metatile Behavior mask is too small to contain all %1 available options.").arg(maxBehavior));
    }
    if (isMaskTooSmall(&Metatile::terrainTypeAttr, NUM_METATILE_TERRAIN_TYPES - 1))
        logWarn(QString("Metatile Terrain Type mask is too small to contain all %1 available options.").arg(NUM_METATILE_TERRAIN_TYPES));
    if (isMaskTooSmall(&Metatile::encounterTypeAttr, NUM_METATILE_ENCOUNTER_TYPES - 1))
        logWarn(QString("Metatile Encounter Type mask is too small to contain all %1 available options.").arg(NUM_METATILE_ENCOUNTER_TYPES));
    if (isMaskTooSmall(&Metatile::layerTypeAttr, NUM_METATILE_LAYER_TYPES - 1))
        logWarn(QString("Metatile Layer Type mask is too small to contain all %1 available options.").arg(NUM_METATILE_LAYER_TYPES));
}

uint32_t Metatile::getAttributes() {
    uint32_t attributes = this->unusedAttributes & Metatile::unusedAttrMask;
    attributes |= Metatile::behaviorAttr.toRaw(this->behavior);
    attributes |= Metatile::terrainTypeAttr.toRaw(this->terrainType);
    attributes |= Metatile::encounterTypeAttr.toRaw(this->encounterType);
    attributes |= Metatile::layerTypeAttr.toRaw(this->layerType);
    return attributes;
}

void Metatile::setAttributes(uint32_t data) {
    this->behavior = Metatile::behaviorAttr.fromRaw(data);
    this->terrainType = Metatile::terrainTypeAttr.fromRaw(data);
    this->encounterType = Metatile::encounterTypeAttr.fromRaw(data);
    this->layerType = Metatile::layerTypeAttr.fromRaw(data);
    this->unusedAttributes = data & Metatile::unusedAttrMask;
}

// Read attributes using a vanilla layout, then set them using the user's layout. For AdvanceMap import
void Metatile::setAttributes(uint32_t data, BaseGameVersion version) {
    const auto defaultLayout = Metatile::defaultLayouts.value(version);
    this->setBehavior(defaultLayout->value("behavior").fromRaw(data));
    this->setTerrainType(defaultLayout->value("terrainType").fromRaw(data));
    this->setEncounterType(defaultLayout->value("encounterType").fromRaw(data));
    this->setLayerType(defaultLayout->value("layerType").fromRaw(data));
}

int Metatile::getDefaultAttributesSize(BaseGameVersion version) {
    return (version == BaseGameVersion::pokefirered) ? 4 : 2;
}
uint32_t Metatile::getBehaviorMask(BaseGameVersion version) {
    return Metatile::defaultLayouts.value(version)->value("behavior").mask;
}
uint32_t Metatile::getTerrainTypeMask(BaseGameVersion version) {
    return Metatile::defaultLayouts.value(version)->value("terrainType").mask;
}
uint32_t Metatile::getEncounterTypeMask(BaseGameVersion version) {
    return Metatile::defaultLayouts.value(version)->value("encounterType").mask;
}
uint32_t Metatile::getLayerTypeMask(BaseGameVersion version) {
    return Metatile::defaultLayouts.value(version)->value("layerType").mask;
}
