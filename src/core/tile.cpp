#include "tile.h"

Tile::Tile(int tile, bool xflip, bool yflip, int palette)
{
    this->tile = tile;
    this->xflip = xflip;
    this->yflip = yflip;
    this->palette = palette;
}
