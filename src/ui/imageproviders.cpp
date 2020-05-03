#include "imageproviders.h"
#include "log.h"
#include <QPainter>

QImage getCollisionMetatileImage(Block block) {
    return getCollisionMetatileImage(block.collision, block.elevation);
}

QImage getCollisionMetatileImage(int collision, int elevation) {
    int x = collision * 16;
    int y = elevation * 16;
    QPixmap collisionImage = QPixmap(":/images/collisions.png").copy(x, y, 16, 16);
    return collisionImage.toImage();
}

QImage getMetatileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset, bool useTruePalettes) {
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);

    Metatile* metatile = Tileset::getMetatile(tile, primaryTileset, secondaryTileset);
    if (!metatile || !metatile->tiles) {
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }

    Tileset* blockTileset = Tileset::getBlockTileset(tile, primaryTileset, secondaryTileset);
    if (!blockTileset) {
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }
    QList<QList<QRgb>> palettes = Tileset::getBlockPalettes(primaryTileset, secondaryTileset, useTruePalettes);

    QPainter metatile_painter(&metatile_image);
    for (int layer = 0; layer < 2; layer++)
    for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
        Tile tile_ = metatile->tiles->value((y * 2) + x + (layer * 4));
        QImage tile_image = getTileImage(tile_.tile, primaryTileset, secondaryTileset);
        if (tile_image.isNull()) {
            // Some metatiles specify tiles that are outside the valid range.
            // These are treated as completely transparent, so they can be skipped without
            // being drawn unless they're on the bottom layer, in which case we need
            // a placeholder because garbage will be drawn otherwise.
            if (layer == 0) {
                metatile_painter.fillRect(x * 8, y * 8, 8, 8, palettes.value(0).value(0));
            }
            continue;
        }

        // Colorize the metatile tiles with its palette.
        if (tile_.palette < palettes.length()) {
            QList<QRgb> palette = palettes.value(tile_.palette);
            for (int j = 0; j < palette.length(); j++) {
                tile_image.setColor(j, palette.value(j));
            }
        } else {
            logWarn(QString("Tile '%1' is referring to invalid palette number: '%2'").arg(tile_.tile).arg(tile_.palette));
        }

        // The top layer of the metatile has its first color displayed at transparent.
        if (layer > 0) {
            QColor color(tile_image.color(0));
            color.setAlpha(0);
            tile_image.setColor(0, color.rgba());
        }

        QPoint origin = QPoint(x*8, y*8);
        metatile_painter.drawImage(origin, tile_image.mirrored(tile_.xflip, tile_.yflip));
    }
    metatile_painter.end();

    return metatile_image;
}

QImage getTileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getBlockTileset(tile, primaryTileset, secondaryTileset);
    int local_index = Metatile::getBlockIndex(tile);
    if (!tileset || !tileset->tiles) {
        return QImage();
    }
    return tileset->tiles->value(local_index, QImage());
}

QImage getColoredTileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset, QList<QRgb> palette) {
    QImage tileImage = getTileImage(tile, primaryTileset, secondaryTileset);
    if (tileImage.isNull()) {
        tileImage = QImage(16, 16, QImage::Format_RGBA8888);
        QPainter painter(&tileImage);
        painter.fillRect(0, 0, 16, 16, palette.at(0));
    } else {
        for (int i = 0; i < 16; i++) {
            tileImage.setColor(i, palette.at(i));
        }
    }

    return tileImage;
}

QImage getPalettedTileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset, int paletteId, bool useTruePalettes) {
    QList<QRgb> palette = Tileset::getPalette(paletteId, primaryTileset, secondaryTileset, useTruePalettes);
    return getColoredTileImage(tile, primaryTileset, secondaryTileset, palette);
}

QImage getGreyscaleTileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return getColoredTileImage(tile, primaryTileset, secondaryTileset, greyscalePalette);
}
