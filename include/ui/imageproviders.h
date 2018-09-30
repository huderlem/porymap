#ifndef IMAGEPROVIDERS_H
#define IMAGEPROVIDERS_H

#include "block.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>

QImage getCollisionMetatileImage(Block);
QImage getCollisionMetatileImage(int, int);
QImage getMetatileImage(uint16_t, Tileset*, Tileset*);
QImage getTileImage(uint16_t, Tileset*, Tileset*);
QImage getColoredTileImage(uint16_t, Tileset*, Tileset*, QList<QRgb>);

#endif // IMAGEPROVIDERS_H
