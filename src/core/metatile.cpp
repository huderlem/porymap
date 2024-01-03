#include "metatile.h"
#include "tileset.h"
#include "project.h"

// Stores how each attribute should be laid out for all metatiles, according to the vanilla games.
// Used to set default config values and import maps with AdvanceMap.
static const QMap<Metatile::Attr, BitPacker> attributePackersFRLG = {
    {Metatile::Attr::Behavior,      BitPacker(0x000001FF) },
    {Metatile::Attr::TerrainType,   BitPacker(0x00003E00) },
    {Metatile::Attr::EncounterType, BitPacker(0x07000000) },
    {Metatile::Attr::LayerType,     BitPacker(0x60000000) },
  //{Metatile::Attr::Unused,        BitPacker(0x98FFC000) },
};
static const QMap<Metatile::Attr, BitPacker> attributePackersRSE = {
    {Metatile::Attr::Behavior,      BitPacker(0x00FF) },
  //{Metatile::Attr::Unused,        BitPacker(0x0F00) },
    {Metatile::Attr::LayerType,     BitPacker(0xF000) },
};

static QMap<Metatile::Attr, BitPacker> attributePackers;

Metatile::Metatile(const int numTiles) {
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

static int numMetatileIdChars = 4;
QString Metatile::getMetatileIdString(uint16_t metatileId) {
    return "0x" + QString("%1").arg(metatileId, numMetatileIdChars, 16, QChar('0')).toUpper();
};

QString Metatile::getMetatileIdStrings(const QList<uint16_t> metatileIds) {
    QStringList metatiles;
    for (auto metatileId : metatileIds)
        metatiles << Metatile::getMetatileIdString(metatileId);
    return metatiles.join(",");
};

// Read and pack together this metatile's attributes.
uint32_t Metatile::getAttributes() const {
    uint32_t data = 0;
    for (auto i = this->attributes.cbegin(), end = this->attributes.cend(); i != end; i++){
        const auto packer = attributePackers.value(i.key());
        data |= packer.pack(i.value());
    }
    return data;
}

// Unpack and insert metatile attributes from the given data.
void Metatile::setAttributes(uint32_t data) {
    for (auto i = attributePackers.cbegin(), end = attributePackers.cend(); i != end; i++){
        const auto packer = i.value();
        this->setAttribute(i.key(), packer.unpack(data));
    }
}

// Unpack and insert metatile attributes from the given data using a vanilla layout. For AdvanceMap import
void Metatile::setAttributes(uint32_t data, BaseGameVersion version) {
    const auto vanillaPackers = (version == BaseGameVersion::pokefirered) ? attributePackersFRLG : attributePackersRSE;
    for (auto i = vanillaPackers.cbegin(), end = vanillaPackers.cend(); i != end; i++){
        const auto packer = i.value();
        this->setAttribute(i.key(), packer.unpack(data));
    }
}

// Set the value for a metatile attribute, and fit it within the valid value range.
void Metatile::setAttribute(Metatile::Attr attr, uint32_t value) {
    const auto packer = attributePackers.value(attr);
    this->attributes.insert(attr, packer.clamp(value));
}

int Metatile::getDefaultAttributesSize(BaseGameVersion version) {
    return (version == BaseGameVersion::pokefirered) ? 4 : 2;
}

uint32_t Metatile::getDefaultAttributesMask(BaseGameVersion version, Metatile::Attr attr) {
    const auto vanillaPackers = (version == BaseGameVersion::pokefirered) ? attributePackersFRLG : attributePackersRSE;
    return vanillaPackers.value(attr).mask();
}

uint32_t Metatile::getMaxAttributesMask() {
    static const QHash<int, uint32_t> maxMasks = {
        {1, 0xFF},
        {2, 0xFFFF},
        {4, 0xFFFFFFFF},
    };
    return maxMasks.value(projectConfig.getMetatileAttributesSize(), 0);
}

void Metatile::setLayout(Project * project) {
    // Calculate the number of hex characters needed to display a metatile ID.
    numMetatileIdChars = 0;
    for (uint16_t i = Block::getMaxMetatileId(); i > 0; i /= 16)
        numMetatileIdChars++;

    uint32_t behaviorMask = projectConfig.getMetatileBehaviorMask();
    uint32_t terrainTypeMask = projectConfig.getMetatileTerrainTypeMask();
    uint32_t encounterTypeMask = projectConfig.getMetatileEncounterTypeMask();
    uint32_t layerTypeMask = projectConfig.getMetatileLayerTypeMask();

    // Calculate mask of bits not used by standard behaviors so we can preserve this data.
    uint32_t unusedMask = ~(behaviorMask | terrainTypeMask | encounterTypeMask | layerTypeMask);
    unusedMask &= Metatile::getMaxAttributesMask();

    BitPacker packer = BitPacker(unusedMask);
    attributePackers.clear();
    attributePackers.insert(Metatile::Attr::Unused, unusedMask);

    // Validate metatile behavior mask
    packer.setMask(behaviorMask);
    if (behaviorMask && !project->metatileBehaviorMapInverse.isEmpty()) {
        uint32_t maxBehavior = project->metatileBehaviorMapInverse.lastKey();
        if (packer.clamp(maxBehavior) != maxBehavior)
            logWarn(QString("Metatile Behavior mask '0x%1' is insufficient to contain all available options.")
                                .arg(QString::number(behaviorMask, 16).toUpper()));
    }
    attributePackers.insert(Metatile::Attr::Behavior, packer);

    // Validate terrain type mask
    packer.setMask(terrainTypeMask);
    const uint32_t maxTerrainType = NUM_METATILE_TERRAIN_TYPES - 1;
    if (terrainTypeMask && packer.clamp(maxTerrainType) != maxTerrainType) {
        logWarn(QString("Metatile Terrain Type mask '0x%1' is insufficient to contain all %2 available options.")
                            .arg(QString::number(terrainTypeMask, 16).toUpper())
                            .arg(maxTerrainType + 1));
    }
    attributePackers.insert(Metatile::Attr::TerrainType, packer);

    // Validate encounter type mask
    packer.setMask(encounterTypeMask);
    const uint32_t maxEncounterType = NUM_METATILE_ENCOUNTER_TYPES - 1;
    if (encounterTypeMask && packer.clamp(maxEncounterType) != maxEncounterType) {
        logWarn(QString("Metatile Encounter Type mask '0x%1' is insufficient to contain all %2 available options.")
                            .arg(QString::number(encounterTypeMask, 16).toUpper())
                            .arg(maxEncounterType + 1));
    }
    attributePackers.insert(Metatile::Attr::EncounterType, packer);

    // Validate terrain type mask
    packer.setMask(layerTypeMask);
    const uint32_t maxLayerType = NUM_METATILE_LAYER_TYPES - 1;
    if (layerTypeMask && packer.clamp(maxLayerType) != maxLayerType) {
        logWarn(QString("Metatile Layer Type mask '0x%1' is insufficient to contain all %2 available options.")
                            .arg(QString::number(layerTypeMask, 16).toUpper())
                            .arg(maxLayerType + 1));
    }
    attributePackers.insert(Metatile::Attr::LayerType, packer);
}
