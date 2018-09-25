#ifndef IMAGEPROVIDERS_H
#define IMAGEPROVIDERS_H

#include "block.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>

QImage getCollisionMetatileImage(Block);
QImage getCollisionMetatileImage(int, int);
QImage getMetatileImage(int, Tileset*, Tileset*);
QImage getTileImage(int, Tileset*, Tileset*);

#endif // IMAGEPROVIDERS_H
