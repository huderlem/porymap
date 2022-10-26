#include "metatile.h"
#include "tileset.h"
#include "project.h"

QHash<Metatile::Attr, Metatile::AttrLayout> Metatile::customLayout = {};
uint32_t Metatile::unusedAttrMask = 0;

const QHash<Metatile::Attr, Metatile::AttrLayout> Metatile::defaultLayoutFRLG = {
    {Metatile::Attr::Behavior,      { .mask = 0x000001FF, .shift = 0} },
    {Metatile::Attr::TerrainType,   { .mask = 0x00003E00, .shift = 9} },
    {Metatile::Attr::EncounterType, { .mask = 0x07000000, .shift = 24} },
    {Metatile::Attr::LayerType,     { .mask = 0x60000000, .shift = 29} },
};

const QHash<Metatile::Attr, Metatile::AttrLayout> Metatile::defaultLayoutRSE = {
    {Metatile::Attr::Behavior,      { .mask = 0x000000FF, .shift = 0} },
    {Metatile::Attr::TerrainType,   { .mask = 0x00000000, .shift = 0} },
    {Metatile::Attr::EncounterType, { .mask = 0x00000000, .shift = 0} },
    {Metatile::Attr::LayerType,     { .mask = 0x0000F000, .shift = 12} },
};

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

// Get the vanilla attribute sizes based on version.
// Used as a default in the config and for AdvanceMap import.
int Metatile::getDefaultAttributesSize(BaseGameVersion version) {
    return (version == BaseGameVersion::pokefirered) ? 4 : 2;
}

QPoint Metatile::coordFromPixmapCoord(const QPointF &pixelCoord) {
    int x = static_cast<int>(pixelCoord.x()) / 16;
    int y = static_cast<int>(pixelCoord.y()) / 16;
    return QPoint(x, y);
}

void Metatile::setCustomAttributeLayout(Metatile::AttrLayout * layout, uint32_t mask, uint32_t max, QString errName) {
    if (mask > max) {
        logWarn(QString("Metatile %1 mask '%2' exceeds maximum size '%3'").arg(errName).arg(mask).arg(max));
        mask &= max;
    }
    layout->mask = mask;
    layout->shift = log2(mask & ~(mask - 1)); // Get the position of the rightmost set bit
}

bool Metatile::doMasksOverlap(QList<uint32_t> masks) {
    for (int i = 0; i < masks.length(); i++)
    for (int j = i + 1; j < masks.length(); j++) {
        if (masks.at(i) & masks.at(j))
            return true;
    }
    return false;
}

void Metatile::setCustomLayout() {
    // Get the maximum size of any attribute mask
    const QHash<int, uint32_t> maxMasks = {
        {1, 0xFF},
        {2, 0xFFFF},
        {4, 0xFFFFFFFF},
    };
    const uint32_t maxMask = maxMasks.value(projectConfig.getMetatileAttributesSize(), 0);

    // Set custom attribute masks from the config file
    setCustomAttributeLayout(&customLayout[Attr::Behavior], projectConfig.getMetatileBehaviorMask(), maxMask, "behavior");
    setCustomAttributeLayout(&customLayout[Attr::TerrainType], projectConfig.getMetatileTerrainTypeMask(), maxMask, "terrain type");
    setCustomAttributeLayout(&customLayout[Attr::EncounterType], projectConfig.getMetatileEncounterTypeMask(), maxMask, "encounter type");
    setCustomAttributeLayout(&customLayout[Attr::LayerType], projectConfig.getMetatileLayerTypeMask(), maxMask, "layer type");

    // Set mask for preserving any attribute bits not used by Porymap
    Metatile::unusedAttrMask = ~(customLayout[Attr::Behavior].mask
                               | customLayout[Attr::TerrainType].mask
                               | customLayout[Attr::EncounterType].mask
                               | customLayout[Attr::LayerType].mask);
    Metatile::unusedAttrMask &= maxMask;

    // Overlapping masks are legal, but probably not intended
    if (doMasksOverlap({customLayout[Attr::Behavior].mask,
                        customLayout[Attr::TerrainType].mask,
                        customLayout[Attr::EncounterType].mask,
                        customLayout[Attr::LayerType].mask})) {
        logWarn("Metatile attribute masks are overlapping.");
    }
}

uint32_t Metatile::getAttributes() {
    uint32_t attributes = this->unusedAttributes & Metatile::unusedAttrMask;

    // Behavior
    Metatile::AttrLayout attr = Metatile::customLayout[Attr::Behavior];
    attributes |= (this->behavior << attr.shift) & attr.mask;

    // Terrain Type
    attr = Metatile::customLayout[Attr::TerrainType];
    attributes |= (this->terrainType << attr.shift) & attr.mask;

    // Encounter Type
    attr = Metatile::customLayout[Attr::EncounterType];
    attributes |= (this->encounterType << attr.shift) & attr.mask;

    // Layer Type
    attr = Metatile::customLayout[Attr::LayerType];
    attributes |= (this->layerType << attr.shift) & attr.mask;

    return attributes;
}

void Metatile::setAttributes(uint32_t data, const QHash<Metatile::Attr, Metatile::AttrLayout> * layout) {
    // Behavior
    Metatile::AttrLayout attr = layout->value(Attr::Behavior);
    this->behavior = (data & attr.mask) >> attr.shift;

    // Terrain Type
    attr = layout->value(Attr::TerrainType);
    this->terrainType = (data & attr.mask) >> attr.shift;

    // Encounter Type
    attr = layout->value(Attr::EncounterType);
    this->encounterType = (data & attr.mask) >> attr.shift;

    // Layer Type
    attr = layout->value(Attr::LayerType);
    this->layerType = (data & attr.mask) >> attr.shift;

    this->unusedAttributes = data & Metatile::unusedAttrMask;
}

void Metatile::setAttributes(uint32_t data) {
    this->setAttributes(data, &Metatile::customLayout);
}

// Read attributes using a vanilla layout, then set them using the user's layout. For AdvanceMap import
void Metatile::convertAttributes(uint32_t data, BaseGameVersion version) {
    if (version == BaseGameVersion::pokefirered) {
        this->setAttributes(data, &Metatile::defaultLayoutFRLG);
    } else {
        this->setAttributes(data, &Metatile::defaultLayoutRSE);
    }
    // Clean data to fit the user's custom masks
    this->setAttributes(this->getAttributes());
}
