#pragma once
#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include "config.h"
#include "bitpacker.h"
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

class Metatile
{
public:
    Metatile() = default;
    Metatile(const Metatile &other) = default;
    Metatile &operator=(const Metatile &other) = default;
    Metatile(const int numTiles);

    enum Attr {
        Behavior,
        TerrainType,
        EncounterType,
        LayerType,
        Unused, // Preserve bits not used by the other attributes
    };

public:
    QList<Tile> tiles;

    uint32_t getAttributes() const;
    uint32_t getAttribute(Metatile::Attr attr) const { return this->attributes.value(attr, 0); }
    void setAttributes(uint32_t data);
    void setAttributes(uint32_t data, BaseGameVersion version);
    void setAttribute(Metatile::Attr attr, uint32_t value);

    // For convenience
    uint32_t behavior()      const { return this->getAttribute(Attr::Behavior); }
    uint32_t terrainType()   const { return this->getAttribute(Attr::TerrainType); }
    uint32_t encounterType() const { return this->getAttribute(Attr::EncounterType); }
    uint32_t layerType()     const { return this->getAttribute(Attr::LayerType); }
    void setBehavior(int value)      { this->setAttribute(Attr::Behavior, static_cast<uint32_t>(value)); }
    void setTerrainType(int value)   { this->setAttribute(Attr::TerrainType, static_cast<uint32_t>(value)); }
    void setEncounterType(int value) { this->setAttribute(Attr::EncounterType, static_cast<uint32_t>(value)); }
    void setLayerType(int value)     { this->setAttribute(Attr::LayerType, static_cast<uint32_t>(value)); }

    static int getIndexInTileset(int);
    static QPoint coordFromPixmapCoord(const QPointF &pixelCoord);
    static uint32_t getDefaultAttributesMask(BaseGameVersion version, Metatile::Attr attr);
    static uint32_t getMaxAttributesMask();
    static int getDefaultAttributesSize(BaseGameVersion version);
    static void setLayout(Project*);
    static QString getMetatileIdString(uint16_t metatileId);
    static QString getMetatileIdStrings(const QList<uint16_t> metatileIds);

    inline bool operator==(const Metatile &other) {
        return this->tiles == other.tiles && this->attributes == other.attributes;
    }

    inline bool operator!=(const Metatile &other) {
        return !(operator==(other));
    }

private:
    QMap<Metatile::Attr, uint32_t> attributes;
};

#endif // METATILE_H
