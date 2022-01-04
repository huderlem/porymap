#pragma once
#ifndef TILE_H
#define TILE_H


class Tile
{
public:
    Tile();
    Tile(int tileId, bool xflip, bool yflip, int palette);

public:
    int tileId;
    bool xflip;
    bool yflip;
    int palette;

    static int getIndexInTileset(int);
};

#endif // TILE_H
