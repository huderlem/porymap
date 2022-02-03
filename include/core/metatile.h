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

public:
    QList<Tile> tiles;
    uint16_t behavior;     // 8 bits RSE, 9 bits FRLG
    uint8_t layerType;
    uint8_t encounterType; // FRLG only
    uint8_t terrainType;   // FRLG only
    uint32_t unusedAttributes;
    QString label;

    void setAttributes(uint32_t data, BaseGameVersion version);
    uint32_t getAttributes(BaseGameVersion version);

    static int getIndexInTileset(int);
    static QPoint coordFromPixmapCoord(const QPointF &pixelCoord);
    static int getAttributesSize(BaseGameVersion version);
};

#endif // METATILE_H
