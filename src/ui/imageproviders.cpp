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

QImage getMetatileImage(uint16_t metatileId, Layout *layout, bool useTruePalettes) {
    Metatile* metatile = Tileset::getMetatile(metatileId,
                                              layout ? layout->tileset_primary : nullptr,
                                              layout ? layout->tileset_secondary : nullptr);
    return getMetatileImage(metatile, layout, useTruePalettes);
}

QImage getMetatileImage(Metatile *metatile, Layout *layout, bool useTruePalettes) {
    if (!layout) {
        return getMetatileImage(metatile, nullptr, nullptr, {}, {}, useTruePalettes);
    }
    return getMetatileImage(metatile,
                            layout->tileset_primary,
                            layout->tileset_secondary,
                            layout->metatileLayerOrder(),
                            layout->metatileLayerOpacity(),
                            useTruePalettes);
}

QImage getMetatileImage(
        uint16_t metatileId,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        const QList<int> &layerOrder,
        const QList<float> &layerOpacity,
        bool useTruePalettes)
{
    return getMetatileImage(Tileset::getMetatile(metatileId, primaryTileset, secondaryTileset),
                            primaryTileset,
                            secondaryTileset,
                            layerOrder,
                            layerOpacity,
                            useTruePalettes);
}

QImage getMetatileImage(
        Metatile *metatile,
        Tileset *primaryTileset,
        Tileset *secondaryTileset,
        const QList<int> &layerOrder,
        const QList<float> &layerOpacity,
        bool useTruePalettes)
{
    const int numTilesWide = 2;
    const int numTilesTall = 2;
    QImage metatile_image(8 * numTilesWide, 8 * numTilesTall, QImage::Format_RGBA8888);
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

    uint32_t layerType = metatile->layerType();
    const int numTilesPerLayer = numTilesWide * numTilesTall;
    for (const auto &layer : layerOrder)
    for (int y = 0; y < numTilesTall; y++)
    for (int x = 0; x < numTilesWide; x++) {
        // Get the tile to render next
        Tile tile;
        int tileOffset = (y * numTilesWide) + x;
        if (projectConfig.tripleLayerMetatilesEnabled) {
            tile = metatile->tiles.value(tileOffset + (layer * numTilesPerLayer));
        } else {
            // "Vanilla" metatiles only have 8 tiles, but render 12.
            // The remaining 4 tiles are rendered using user-specified tiles depending on layer type.
            switch (layerType)
            {
            default:
            case Metatile::LayerType::Normal:
                if (layer == 0)
                    tile = Tile(projectConfig.unusedTileNormal);
                else // Tiles are on layers 1 and 2
                    tile = metatile->tiles.value(tileOffset + ((layer - 1) * numTilesPerLayer));
                break;
            case Metatile::LayerType::Covered:
                if (layer == 2)
                    tile = Tile(projectConfig.unusedTileCovered);
                else // Tiles are on layers 0 and 1
                    tile = metatile->tiles.value(tileOffset + (layer * numTilesPerLayer));
                break;
            case Metatile::LayerType::Split:
                if (layer == 1)
                    tile = Tile(projectConfig.unusedTileSplit);
                else // Tiles are on layers 0 and 2
                    tile = metatile->tiles.value(tileOffset + ((layer == 0 ? 0 : 1) * numTilesPerLayer));
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
        float opacity = layerOpacity.value(layer, 1.0);
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
        tile.flip(&tile_image);
        metatile_painter.drawImage(origin, tile_image);
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
