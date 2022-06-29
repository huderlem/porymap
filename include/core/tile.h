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
