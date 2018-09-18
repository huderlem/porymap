#ifndef TILESET_H
#define TILESET_H

#include "tile.h"
#include <QImage>

class Metatile;

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

    static int num_tiles_primary;
    static int num_tiles_total;
    static int num_metatiles_primary;
    static int num_metatiles_total;
    static int num_pals_primary;
    static int num_pals_total;
};

class Metatile
{
public:
    Metatile();
public:
    QList<Tile> *tiles = nullptr;
    int attr;

    static QImage getMetatileImage(int, Tileset*, Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static QImage getMetatileTile(int, Tileset*, Tileset*);
    static Tileset* getBlockTileset(int, Tileset*, Tileset*);
    static int getBlockIndex(int);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*);
};

#endif // TILESET_H
