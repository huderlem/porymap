#include "tileset.h"
#include "metatile.h"
#include "project.h"

#include <QPainter>
#include <QImage>
#include <QDebug>

Tileset::Tileset()
{

}

Tileset* Tileset::copy() {
    Tileset *copy = new Tileset;
    copy->name = this->name;
    copy->is_compressed = this->is_compressed;
    copy->is_secondary = this->is_secondary;
    copy->padding = this->padding;
    copy->tiles_label = this->tiles_label;
    copy->palettes_label = this->palettes_label;
    copy->metatiles_label = this->metatiles_label;
    copy->metatiles_path = this->metatiles_path;
    copy->callback_label = this->callback_label;
    copy->metatile_attrs_label = this->metatile_attrs_label;
    copy->metatile_attrs_path = this->metatile_attrs_path;
    copy->tiles = new QList<QImage>;
    for (QImage tile : *this->tiles) {
        copy->tiles->append(tile.copy());
    }
    copy->metatiles = new QList<Metatile*>;
    for (Metatile *metatile : *this->metatiles) {
        copy->metatiles->append(metatile->copy());
    }
    copy->palettes = new QList<QList<QRgb>>;
    for (QList<QRgb> palette : *this->palettes) {
        QList<QRgb> copyPalette;
        for (QRgb color : palette) {
            copyPalette.append(color);
        }
        copy->palettes->append(copyPalette);
    }
    return copy;
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
