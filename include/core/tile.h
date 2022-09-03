#pragma once
#ifndef TILE_H
#define TILE_H

#include <QObject>

class Tile
{
public:
    Tile();
    Tile(uint16_t tileId, uint16_t xflip, uint16_t yflip, uint16_t palette);
    Tile(uint16_t raw);

public:
    uint16_t tileId:10;
    uint16_t xflip:1;
    uint16_t yflip:1;
    uint16_t palette:4;
    uint16_t rawValue() const;

    static int getIndexInTileset(int);
};

inline bool operator==(const Tile &a, const Tile &b) {
    return a.tileId  == b.tileId &&
           a.xflip   == b.xflip &&
           a.yflip   == b.yflip &&
           a.palette == b.palette;
}

inline bool operator!=(const Tile &a, const Tile &b) {
    return !(operator==(a, b));
}

#endif // TILE_H
