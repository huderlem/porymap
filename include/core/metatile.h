#pragma once
#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include "config.h"
#include <QImage>
#include <QPoint>
#include <QString>

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
    Metatile();
    Metatile(const Metatile &other) = default;
    Metatile &operator=(const Metatile &other) = default;
    Metatile(const int numTiles);

public:
    QList<Tile> tiles;
    uint32_t behavior;
    uint32_t layerType;
    uint32_t encounterType;
    uint32_t terrainType;
    uint32_t unusedAttributes;
    QString label;

    uint32_t getAttributes();
    void setAttributes(uint32_t data);
    void setAttributes(uint32_t data, BaseGameVersion version);

    static int getIndexInTileset(int);
    static QPoint coordFromPixmapCoord(const QPointF &pixelCoord);
    static int getAttributesSize(BaseGameVersion version);
    static void calculateAttributeLayout();

private:
    static uint32_t behaviorMask;
    static uint32_t terrainTypeMask;
    static uint32_t encounterTypeMask;
    static uint32_t layerTypeMask;
    static uint32_t unusedAttrMask;
    static int behaviorShift;
    static int terrainTypeShift;
    static int encounterTypeShift;
    static int layerTypeShift;

    static int getShiftValue(uint32_t mask);
};

inline bool operator==(const Metatile &a, const Metatile &b) {
    return a.behavior         == b.behavior &&
           a.layerType        == b.layerType &&
           a.encounterType    == b.encounterType &&
           a.terrainType      == b.terrainType &&
           a.unusedAttributes == b.unusedAttributes &&
           a.label            == b.label &&
           a.tiles            == b.tiles;
}

inline bool operator!=(const Metatile &a, const Metatile &b) {
    return !(operator==(a, b));
}

#endif // METATILE_H
