#include "tileset.h"
#include "metatile.h"
#include "project.h"

#include <QPainter>
#include <QImage>
#include <QDebug>

Tileset::Tileset()
{

}

Tileset* Tileset::getBlockTileset(int metatile_index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatile_index < Project::getNumMetatilesPrimary()) {
        return primaryTileset;
    } else {
        return secondaryTileset;
    }
}

Metatile* Tileset::getMetatile(int index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getBlockTileset(index, primaryTileset, secondaryTileset);
    int local_index = Metatile::getBlockIndex(index);
    if (!tileset || !tileset->metatiles) {
        return nullptr;
    }
    Metatile *metatile = tileset->metatiles->value(local_index, nullptr);
    return metatile;
}

QList<QList<QRgb>> Tileset::getBlockPalettes(Tileset *primaryTileset, Tileset *secondaryTileset) {
    QList<QList<QRgb>> palettes;
    for (int i = 0; i < Project::getNumPalettesPrimary(); i++) {
        palettes.append(primaryTileset->palettes->at(i));
    }
    for (int i = Project::getNumPalettesPrimary(); i < Project::getNumPalettesTotal(); i++) {
        palettes.append(secondaryTileset->palettes->at(i));
    }
    return palettes;
}

QList<QRgb> Tileset::getPalette(int paletteId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    QList<QRgb> paletteTable;
    Tileset *tileset = paletteId < Project::getNumPalettesPrimary()
            ? primaryTileset
            : secondaryTileset;
    for (int i = 0; i < tileset->palettes->at(paletteId).length(); i++) {
        paletteTable.append(tileset->palettes->at(paletteId).at(i));
    }
    return paletteTable;
}
