#pragma once
#ifndef TILE_H
#define TILE_H


class Tile
{
public:
    Tile() {}
    Tile(int tile, bool xflip, bool yflip, int palette);
public:
    int tile;
    bool xflip;
    bool yflip;
    int palette;
};

#endif // TILE_H
