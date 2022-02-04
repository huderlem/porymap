#include "tile.h"
#include "project.h"

 Tile::Tile() :
        tileId(0),
        xflip(false),
        yflip(false),
        palette(0)
    {  }

 Tile::Tile(int tileId, bool xflip, bool yflip, int palette) :
        tileId(tileId),
        xflip(xflip),
        yflip(yflip),
        palette(palette)
    {  }

 Tile::Tile(uint16_t raw) :
        tileId(raw & 0x3FF),
        xflip((raw >> 10) & 1),
        yflip((raw >> 11) & 1),
        palette((raw >> 12) & 0xF)
    {  }

uint16_t Tile::rawValue() const {
    return static_cast<uint16_t>(
            (this->tileId & 0x3FF)
         | ((this->xflip & 1) << 10)
         | ((this->yflip & 1) << 11)
         | ((this->palette & 0xF) << 12));
}

int Tile::getIndexInTileset(int tileId) {
    if (tileId < Project::getNumTilesPrimary()) {
        return tileId;
    } else {
        return tileId - Project::getNumTilesPrimary();
    }
}
