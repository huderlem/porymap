#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include <QList>

class Metatile
{
public:
    Metatile();
public:
    QList<Tile> *tiles = NULL;
    int attr;
};

#endif // METATILE_H
