#include "config.h"
#include "imageproviders.h"
#include "log.h"
#include "editor.h"
#include <QPainter>

QImage getCollisionMetatileImage(Block block) {
    return getCollisionMetatileImage(block.collision(), block.elevation());
}

QImage getCollisionMetatileImage(int collision, int elevation) {
    const QImage * image = Editor::collisionIcons.at(collision).at(elevation);
    return image ? *image : QImage();
}

QImage getMetatileImage(
        uint16_t metatileId,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        const QList<int> &layerOrder,
        const QList<float> &layerOpacity,
        bool useTruePalettes)
{
    Metatile* metatile = Tileset::getMetatile(metatileId, primaryTileset, secondaryTileset);
    if (!metatile) {
        QImage metatile_image(16, 16, QImage::Format_RGBA8888);
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }
    return getMetatileImage(metatile, primaryTileset, secondaryTileset, layerOrder, layerOpacity, useTruePalettes);
}

QImage getMetatileImage(
        Metatile *metatile,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        const QList<int> &layerOrder,
        const QList<float> &layerOpacity,
        bool useTruePalettes)
{
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);
    if (!metatile) {
        metatile_image.fill(Qt::magenta);
        return metatile_image;
    }

    QList<QList<QRgb>> palettes = Tileset::getBlockPalettes(primaryTileset, secondaryTileset, useTruePalettes);

    // We need to fill the metatile image with something so that if any transparent
    // tile pixels line up across layers we will still have something to render.
    // The GBA renders transparent pixels using palette 0 color 0. We have this color,
    // but all 3 games actually overwrite it with black when loading the tileset palettes,
    // so we have a setting to choose between these two behaviors.
    metatile_image.fill(projectConfig.setTransparentPixelsBlack ? QColor("black") : QColor(palettes.value(0).value(0)));

    QPainter metatile_painter(&metatile_image);
    const int numLayers = 3; // When rendering, metatiles always have 3 layers
    uint32_t layerType = metatile->layerType();
    for (int layer = 0; layer < numLayers; layer++)
    for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
        int l = layerOrder.size() >= numLayers ? layerOrder[layer] : layer;

        // Get the tile to render next
        Tile tile;
        int tileOffset = (y * 2) + x;
        if (projectConfig.tripleLayerMetatilesEnabled) {
            tile = metatile->tiles.value(tileOffset + (l * 4));
        } else {
            // "Vanilla" metatiles only have 8 tiles, but render 12.
            // The remaining 4 tiles are rendered using user-specified tiles depending on layer type.
            switch (layerType)
            {
            default:
            case METATILE_LAYER_MIDDLE_TOP:
                if (l == 0)
                    tile = Tile(projectConfig.unusedTileNormal);
                else // Tiles are on layers 1 and 2
                    tile = metatile->tiles.value(tileOffset + ((l - 1) * 4));
                break;
            case METATILE_LAYER_BOTTOM_MIDDLE:
                if (l == 2)
                    tile = Tile(projectConfig.unusedTileCovered);
                else // Tiles are on layers 0 and 1
                    tile = metatile->tiles.value(tileOffset + (l * 4));
                break;
            case METATILE_LAYER_BOTTOM_TOP:
                if (l == 1)
                    tile = Tile(projectConfig.unusedTileSplit);
                else // Tiles are on layers 0 and 2
                    tile = metatile->tiles.value(tileOffset + ((l == 0 ? 0 : 1) * 4));
                break;
            }
        }

        QImage tile_image = getTileImage(tile.tileId, primaryTileset, secondaryTileset);
        if (tile_image.isNull()) {
            // Some metatiles specify tiles that are outside the valid range.
            // The way the GBA will render these depends on what's in memory (which Porymap can't know)
            // so we treat them as if they were transparent.
            continue;
        }

        // Colorize the metatile tiles with its palette.
        if (tile.palette < palettes.length()) {
            const QList<QRgb> palette = palettes.value(tile.palette);
            for (int j = 0; j < palette.length(); j++) {
                tile_image.setColor(j, palette.value(j));
            }
        } else {
            logWarn(QString("Tile '%1' is referring to invalid palette number: '%2'").arg(tile.tileId).arg(tile.palette));
        }

        QPoint origin = QPoint(x*8, y*8);
        float opacity = layerOpacity.size() >= numLayers ? layerOpacity[l] : 1.0;
        if (opacity < 1.0) {
            int alpha = 255 * opacity;
            for (int c = 0; c < tile_image.colorCount(); c++) {
                QColor color(tile_image.color(c));
                color.setAlpha(alpha);
                tile_image.setColor(c, color.rgba());
            }
        }

        // Color 0 is displayed as transparent.
        QColor color(tile_image.color(0));
        color.setAlpha(0);
        tile_image.setColor(0, color.rgba());

        metatile_painter.drawImage(origin, tile_image.mirrored(tile.xflip, tile.yflip));
    }
    metatile_painter.end();

    return metatile_image;
}

QImage getTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getTileTileset(tileId, primaryTileset, secondaryTileset);
    int index = Tile::getIndexInTileset(tileId);
    if (!tileset) {
        return QImage();
    }
    return tileset->tiles.value(index, QImage());
}

QImage getColoredTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset, const QList<QRgb> &palette) {
    QImage tileImage = getTileImage(tileId, primaryTileset, secondaryTileset);
    if (tileImage.isNull()) {
        tileImage = QImage(8, 8, QImage::Format_RGBA8888);
        QPainter painter(&tileImage);
        painter.fillRect(0, 0, 8, 8, palette.at(0));
    } else {
        for (int i = 0; i < 16; i++) {
            tileImage.setColor(i, palette.at(i));
        }
    }

    return tileImage;
}

QImage getPalettedTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset, int paletteId, bool useTruePalettes) {
    QList<QRgb> palette = Tileset::getPalette(paletteId, primaryTileset, secondaryTileset, useTruePalettes);
    return getColoredTileImage(tileId, primaryTileset, secondaryTileset, palette);
}

QImage getGreyscaleTileImage(uint16_t tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return getColoredTileImage(tileId, primaryTileset, secondaryTileset, greyscalePalette);
}

// gbagfx allows 4bpp image data to be represented with 8bpp .png files by considering only the lower 4 bits of each pixel.
// Reproduce that here to support this type of image use.
void flattenTo4bppImage(QImage * image) {
    if (!image) return;
    uchar * pixel = image->bits();
    for (int i = 0; i < image->sizeInBytes(); i++, pixel++)
        *pixel %= 16;
}
