#pragma once
#ifndef TILE_H
#define TILE_H

#include <QObject>

class Tile
{
public:
    Tile();
    Tile(int tileId, bool xflip, bool yflip, int palette);
    Tile(uint16_t raw);

public:
    int tileId;
    bool xflip;
    bool yflip;
    int palette;

    uint16_t rawValue() const;

    static int getIndexInTileset(int);
};

#endif // TILE_H
