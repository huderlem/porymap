#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include <QImage>
#include <QString>

class Metatile
{
public:
    Metatile();
public:
    QList<Tile> *tiles = nullptr;
    uint16_t behavior;     // 8 bits RSE, 9 bits FRLG
    uint8_t layerType;
    uint8_t encounterType; // FRLG only
    uint8_t terrainType;   // FRLG only
    QString label;

    Metatile *copy();
    void copyInPlace(Metatile*);
    static int getBlockIndex(int);
};

#endif // METATILE_H
