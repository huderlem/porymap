#ifndef TILESET_H
#define TILESET_H

#include "metatile.h"
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

    QList<QImage> *tiles = NULL;
    QList<Metatile*> *metatiles = NULL;
    QList<QList<QRgb>> *palettes = NULL;
};

#endif // TILESET_H
