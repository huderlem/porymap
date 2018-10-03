#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include <QImage>

class Metatile
{
public:
    Metatile();
public:
    QList<Tile> *tiles = nullptr;
    uint8_t behavior;
    uint8_t layerType;

    static int getBlockIndex(int);
};

#endif // METATILE_H
