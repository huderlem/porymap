#include "metatile.h"
#include "tileset.h"
#include "project.h"

Metatile::Metatile()
{
    tiles = new QList<Tile>;
}

int Metatile::getBlockIndex(int index) {
    if (index < Project::getNumMetatilesPrimary()) {
        return index;
    } else {
        return index - Project::getNumMetatilesPrimary();
    }
}
