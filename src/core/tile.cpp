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

int Tile::getIndexInTileset(int tileId) {
    if (tileId < Project::getNumTilesPrimary()) {
        return tileId;
    } else {
        return tileId - Project::getNumTilesPrimary();
    }
}
