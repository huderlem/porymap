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
    uint8_t behavior;
    uint8_t layerType;
    QString label;

    Metatile *copy();
    void copyInPlace(Metatile*);
    static int getBlockIndex(int);
};

#endif // METATILE_H
