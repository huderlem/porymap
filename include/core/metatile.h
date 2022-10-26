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
    uint32_t terrainType;
    uint32_t encounterType;
    uint32_t layerType;
    uint32_t unusedAttributes;
    QString label;

    enum Attr {
        Behavior,
        TerrainType,
        EncounterType,
        LayerType,
    };

    struct AttrLayout {
        uint32_t mask;
        int shift;
    };

    static const QHash<Metatile::Attr, Metatile::AttrLayout> defaultLayoutFRLG;
    static const QHash<Metatile::Attr, Metatile::AttrLayout> defaultLayoutRSE;
    static QHash<Metatile::Attr, Metatile::AttrLayout> customLayout;

    uint32_t getAttributes();
    void setAttributes(uint32_t data);
    void convertAttributes(uint32_t data, BaseGameVersion version);

    void setBehavior(uint32_t);
    void setTerrainType(uint32_t);
    void setEncounterType(uint32_t);
    void setLayerType(uint32_t);

    static int getIndexInTileset(int);
    static QPoint coordFromPixmapCoord(const QPointF &pixelCoord);
    static int getDefaultAttributesSize(BaseGameVersion version);
    static void setCustomLayout();

private:
    static uint32_t unusedAttrMask;

    void setAttributes(uint32_t, const QHash<Metatile::Attr, Metatile::AttrLayout>*);
    static void setCustomAttributeLayout(Metatile::AttrLayout *, uint32_t, uint32_t);
    static bool isMaskTooSmall(Metatile::AttrLayout * layout, int n);
    static bool doMasksOverlap(QList<uint32_t>);
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
