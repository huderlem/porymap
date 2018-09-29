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

    static int getBlockIndex(int);
};

#endif // METATILE_H
