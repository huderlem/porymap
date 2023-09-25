#pragma once
#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include "config.h"
#include <QImage>
#include <QPoint>
#include <QString>

class Project;

enum {
    METATILE_LAYER_MIDDLE_TOP,
    METATILE_LAYER_BOTTOM_MIDDLE,
    METATILE_LAYER_BOTTOM_TOP,
    NUM_METATILE_LAYER_TYPES
};

enum {
    ENCOUNTER_NONE,
    ENCOUNTER_LAND,
    ENCOUNTER_WATER,
    NUM_METATILE_ENCOUNTER_TYPES
};

enum {
    TERRAIN_NONE,
    TERRAIN_GRASS,
    TERRAIN_WATER,
    TERRAIN_WATERFALL,
    NUM_METATILE_TERRAIN_TYPES
};

class MetatileAttr
{
public:
    MetatileAttr();
    MetatileAttr(uint32_t mask, int shift);

public:
    uint32_t mask;
    int shift;

    // Given the raw value for all attributes of a metatile
    // Returns the extracted value for this attribute
    uint32_t fromRaw(uint32_t raw) const { return (raw & this->mask) >> this->shift; }

    // Given a value for this attribute
    // Returns the raw value to OR together with the other attributes
    uint32_t toRaw(uint32_t value) const { return (value << this->shift) & this->mask; }

    // Given an arbitrary value to set for an attribute
    // Returns a bounded value for that attribute
    uint32_t getClamped(int value) const { return static_cast<uint32_t>(value) & (this->mask >> this->shift); }
};

class Metatile
{
public:
    Metatile();
    Metatile(const Metatile &other) = default;
    Metatile &operator=(const Metatile &other) = default;
    Metatile(const int numTiles);

public:
    QList<Tile> tiles;
    uint32_t behavior;
    uint32_t terrainType;
    uint32_t encounterType;
    uint32_t layerType;
    uint32_t unusedAttributes;

    uint32_t getAttributes();
    void setAttributes(uint32_t data);
    void setAttributes(uint32_t data, BaseGameVersion version);

    void setBehavior(int value) { this->behavior = behaviorAttr.getClamped(value); }
    void setTerrainType(int value) { this->terrainType = terrainTypeAttr.getClamped(value); }
    void setEncounterType(int value) { this->encounterType = encounterTypeAttr.getClamped(value); }
    void setLayerType(int value) { this->layerType = layerTypeAttr.getClamped(value); }

    static uint32_t getBehaviorMask() { return behaviorAttr.mask; }
    static uint32_t getTerrainTypeMask() { return terrainTypeAttr.mask; }
    static uint32_t getEncounterTypeMask() { return encounterTypeAttr.mask; }
    static uint32_t getLayerTypeMask() { return layerTypeAttr.mask; }
    static uint32_t getBehaviorMask(BaseGameVersion version);
    static uint32_t getTerrainTypeMask(BaseGameVersion version);
    static uint32_t getEncounterTypeMask(BaseGameVersion version);
    static uint32_t getLayerTypeMask(BaseGameVersion version);

    static int getIndexInTileset(int);
    static QPoint coordFromPixmapCoord(const QPointF &pixelCoord);
    static int getDefaultAttributesSize(BaseGameVersion version);
    static void setCustomLayout(Project*);
    static QString getMetatileIdString(uint16_t metatileId) {
        return "0x" + QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper();
    };
    static QString getMetatileIdStringList(const QList<uint16_t> metatileIds) {
        QStringList metatiles;
        for (auto metatileId : metatileIds)
            metatiles << Metatile::getMetatileIdString(metatileId);
        return metatiles.join(",");
    };

private:
    // Stores how each attribute should be laid out for all metatiles, according to the user's config
    static MetatileAttr behaviorAttr;
    static MetatileAttr terrainTypeAttr;
    static MetatileAttr encounterTypeAttr;
    static MetatileAttr layerTypeAttr;

    static uint32_t unusedAttrMask;

    // Stores how each attribute should be laid out for all metatiles, according to the vanilla games
    // Used to set default config values and import maps with AdvanceMap
    static const QHash<QString, MetatileAttr> defaultLayoutFRLG;
    static const QHash<QString, MetatileAttr> defaultLayoutRSE;
    static const QHash<BaseGameVersion, const QHash<QString, MetatileAttr>*> defaultLayouts;

    static void setCustomAttributeLayout(MetatileAttr *, uint32_t, uint32_t);
    static bool isMaskTooSmall(MetatileAttr *, int);
    static bool doMasksOverlap(QList<uint32_t>);
};

inline bool operator==(const Metatile &a, const Metatile &b) {
    return a.behavior         == b.behavior &&
           a.layerType        == b.layerType &&
           a.encounterType    == b.encounterType &&
           a.terrainType      == b.terrainType &&
           a.unusedAttributes == b.unusedAttributes &&
           a.tiles            == b.tiles;
}

inline bool operator!=(const Metatile &a, const Metatile &b) {
    return !(operator==(a, b));
}

#endif // METATILE_H
