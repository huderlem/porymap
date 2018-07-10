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

    QList<QImage> *tiles = NULL;
    QList<Metatile*> *metatiles = NULL;
    QList<QList<QRgb>> *palettes = NULL;
};

class Metatile
{
public:
    Metatile();
public:
    QList<Tile> *tiles = NULL;
    int attr;

    static QImage getMetatileImage(int, Tileset*, Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static QImage getMetatileTile(int, Tileset*, Tileset*);
    static Tileset* getBlockTileset(int, Tileset*, Tileset*);
    static int getBlockIndex(int);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*);
};

#endif // TILESET_H
