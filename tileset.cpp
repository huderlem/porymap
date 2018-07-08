#include "tileset.h"

#include <QPainter>
#include <QImage>
#include <QDebug>

Tileset::Tileset()
{

}

Metatile::Metatile()
{
    tiles = new QList<Tile>;
}

QImage Metatile::getMetatileImage(int tile, Tileset *primaryTileset, Tileset *secondaryTileset) {
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);

    Metatile* metatile = Metatile::getMetatile(tile, primaryTileset, secondaryTileset);
    if (!metatile || !metatile->tiles) {
        metatile_image.fill(0xffffffff);
        return metatile_image;
    }

    Tileset* blockTileset = Metatile::getBlockTileset(tile, primaryTileset, secondaryTileset);
    if (!blockTileset) {
        metatile_image.fill(0xffffffff);
        return metatile_image;
    }
    QList<QList<QRgb>> palettes = Metatile::getBlockPalettes(primaryTileset, secondaryTileset);

    QPainter metatile_painter(&metatile_image);
    for (int layer = 0; layer < 2; layer++)
    for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
        Tile tile_ = metatile->tiles->value((y * 2) + x + (layer * 4));
        QImage tile_image = Metatile::getMetatileTile(tile_.tile, primaryTileset, secondaryTileset);
        if (tile_image.isNull()) {
            // Some metatiles specify tiles that are outside the valid range.
            // These are treated as completely transparent, so they can be skipped without
            // being drawn.
            continue;
        }

        // Colorize the metatile tiles with its palette.
        if (tile_.palette < palettes.length()) {
            QList<QRgb> palette = palettes.value(tile_.palette);
            for (int j = 0; j < palette.length(); j++) {
                tile_image.setColor(j, palette.value(j));
            }
        } else {
            qDebug() << "Tile is referring to invalid palette number: " << tile_.palette;
        }

        // The top layer of the metatile has its last color displayed at transparent.
        if (layer > 0) {
            QColor color(tile_image.color(15));
            color.setAlpha(0);
            tile_image.setColor(15, color.rgba());
        }

        QPoint origin = QPoint(x*8, y*8);
        metatile_painter.drawImage(origin, tile_image.mirrored(tile_.xflip == 1, tile_.yflip == 1));
    }
    metatile_painter.end();

    return metatile_image;
}

Metatile* Metatile::getMetatile(int index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Metatile::getBlockTileset(index, primaryTileset, secondaryTileset);
    int local_index = Metatile::getBlockIndex(index);
    if (!tileset || !tileset->metatiles) {
        return NULL;
    }
    Metatile *metatile = tileset->metatiles->value(local_index, NULL);
    return metatile;
}

QImage Metatile::getMetatileTile(int tile, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Metatile::getBlockTileset(tile, primaryTileset, secondaryTileset);
    int local_index = Metatile::getBlockIndex(tile);
    if (!tileset || !tileset->tiles) {
        return QImage();
    }
    return tileset->tiles->value(local_index, QImage());
}

Tileset* Metatile::getBlockTileset(int metatile_index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    int primary_size = 0x200;
    if (metatile_index < primary_size) {
        return primaryTileset;
    } else {
        return secondaryTileset;
    }
}

int Metatile::getBlockIndex(int index) {
    int primary_size = 0x200;
    if (index < primary_size) {
        return index;
    } else {
        return index - primary_size;
    }
}

QList<QList<QRgb>> Metatile::getBlockPalettes(Tileset *primaryTileset, Tileset *secondaryTileset) {
    QList<QList<QRgb>> palettes;
    for (int i = 0; i < 6; i++) {
        palettes.append(primaryTileset->palettes->at(i));
    }
    for (int i = 6; i < 12; i++) {
        palettes.append(secondaryTileset->palettes->at(i));
    }
    return palettes;
}
