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
    QString is_secondary;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString metatiles_path;
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
    static QHash<int, QString> getHeaderMemberMap(bool usingAsm);
    static QString getExpectedDir(QString tilesetName, bool isSecondary);
    QString getExpectedDir();
    bool appendToHeaders(QString root, QString friendlyName, bool usingAsm);
    bool appendToGraphics(QString root, QString friendlyName, bool usingAsm);
    bool appendToMetatiles(QString root, QString friendlyName, bool usingAsm);
};

#endif // TILESET_H
