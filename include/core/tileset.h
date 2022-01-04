#pragma once
#ifndef TILESET_H
#define TILESET_H

#include "metatile.h"
#include "tile.h"
#include <QImage>

class Tileset
{
public:
    Tileset() = default;
    Tileset(const Tileset &other);
    Tileset &operator=(const Tileset &other);

public:
    QString name;
    QString is_compressed;
    QString is_secondary;
    QString padding;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString metatiles_path;
    QString callback_label;
    QString metatile_attrs_label;
    QString metatile_attrs_path;
    QString tilesImagePath;
    QImage tilesImage;
    QStringList palettePaths;

    QList<QImage> tiles;
    QList<Metatile*> metatiles;
    QList<QList<QRgb>> palettes;
    QList<QList<QRgb>> palettePreviews;

    static Tileset* getMetatileTileset(int, Tileset*, Tileset*);
    static Tileset* getTileTileset(int, Tileset*, Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*, bool useTruePalettes = false);
    static QList<QRgb> getPalette(int, Tileset*, Tileset*, bool useTruePalettes = false);
    static bool metatileIsValid(uint16_t metatileId, Tileset *, Tileset *);

    bool appendToHeaders(QString headerFile, QString friendlyName);
    bool appendToGraphics(QString graphicsFile, QString friendlyName, bool primary);
    bool appendToMetatiles(QString metatileFile, QString friendlyName, bool primary);
};

#endif // TILESET_H
