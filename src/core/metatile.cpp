#include "metatile.h"
#include "tileset.h"
#include "project.h"

Metatile::Metatile()
{
    tiles = new QList<Tile>;
}

Metatile* Metatile::copy() {
    Metatile *copy = new Metatile;
    copy->behavior = this->behavior;
    copy->layerType = this->layerType;
    copy->tiles = new QList<Tile>;
    for (Tile tile : *this->tiles) {
        copy->tiles->append(tile);
    }
    return copy;
}

void Metatile::copyInPlace(Metatile *other) {
    this->behavior = other->behavior;
    this->layerType = other->layerType;
    for (int i = 0; i < this->tiles->length(); i++) {
        (*this->tiles)[i] = other->tiles->at(i);
    }
}

int Metatile::getBlockIndex(int index) {
    if (index < Project::getNumMetatilesPrimary()) {
        return index;
    } else {
        return index - Project::getNumMetatilesPrimary();
    }
}
