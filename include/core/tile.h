#pragma once
#ifndef TILE_H
#define TILE_H


class Tile
{
public:
    Tile() :
        tileId(0),
        xflip(false),
        yflip(false),
        palette(0)
    {  }

    Tile(int tileId, bool xflip, bool yflip, int palette) :
        tileId(tileId),
        xflip(xflip),
        yflip(yflip),
        palette(palette)
    {  }

public:
    int tileId;
    bool xflip;
    bool yflip;
    int palette;
};

#endif // TILE_H
