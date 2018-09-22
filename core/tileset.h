#ifndef TILESET_H
#define TILESET_H

#include "core/metatile.h"
#include "core/tile.h"
#include <QImage>

class Tileset
{
public:
    Tileset();
public:
    QString name;
    QString is_compressed;
    QString is_secondary;
    QString padding;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString callback_label;
    QString metatile_attrs_label;

    QList<QImage> *tiles = nullptr;
    QList<Metatile*> *metatiles = nullptr;
    QList<QList<QRgb>> *palettes = nullptr;

    static Tileset* getBlockTileset(int, Tileset*, Tileset*);
    static QImage getMetatileImage(int, Tileset*, Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static QImage getMetatileTile(int, Tileset*, Tileset*);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*);
};

#endif // TILESET_H
