#ifndef IMAGEPROVIDERS_H
#define IMAGEPROVIDERS_H

#include "block.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>

QImage getCollisionMetatileImage(Block);
QImage getCollisionMetatileImage(int, int);
QImage getMetatileImage(uint16_t, Tileset*, Tileset*, QList<int>, QList<float>, bool useTruePalettes = false);
QImage getMetatileImage(Metatile*, Tileset*, Tileset*, QList<int>, QList<float>, bool useTruePalettes = false);
QImage getTileImage(uint16_t, Tileset*, Tileset*);
QImage getPalettedTileImage(uint16_t, Tileset*, Tileset*, int, bool useTruePalettes = false);
QImage getGreyscaleTileImage(uint16_t tile, Tileset *primaryTileset, Tileset *secondaryTileset);
void flattenTo4bppImage(QImage * image);

static QList<QRgb> greyscalePalette({
    qRgb(0, 0, 0),
    qRgb(16, 16, 16),
    qRgb(32, 32, 32),
    qRgb(48, 48, 48),
    qRgb(64, 64, 64),
    qRgb(80, 80, 80),
    qRgb(96, 96, 96),
    qRgb(112, 112, 112),
    qRgb(128, 128, 128),
    qRgb(144, 144, 144),
    qRgb(160, 160, 160),
    qRgb(176, 176, 176),
    qRgb(192, 192, 192),
    qRgb(208, 208, 208),
    qRgb(224, 224, 224),
    qRgb(240, 240, 240),
});

#endif // IMAGEPROVIDERS_H
