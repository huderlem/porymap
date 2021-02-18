#pragma once
#ifndef TILE_H
#define TILE_H

class Tile {
public:
    Tile() : tile(0), xflip(false), yflip(false), palette(0) {
    }

    Tile(int tile, bool xflip, bool yflip, int palette) : tile(tile), xflip(xflip), yflip(yflip), palette(palette) {
    }

public:
    int tile;
    bool xflip;
    bool yflip;
    int palette;
};

#endif // TILE_H
