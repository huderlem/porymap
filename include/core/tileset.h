#pragma once
#ifndef TILESET_H
#define TILESET_H

#include "metatile.h"
#include "tile.h"
#include <QImage>
#include <QHash>

struct MetatileLabelPair {
    QString owned;
    QString shared;
};

class Tileset
{
public:
    Tileset() = default;
    Tileset(const Tileset &other);
    Tileset &operator=(const Tileset &other);
    ~Tileset();

public:
    QString name;
    bool is_secondary;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString metatiles_path;
    QString metatile_attrs_label;
    QString metatile_attrs_path;
    QString tilesImagePath;
    QStringList palettePaths;

    QHash<int, QString> metatileLabels;
    QList<QList<QRgb>> palettes;
    QList<QList<QRgb>> palettePreviews;

    static QString stripPrefix(const QString &fullName);
    static Tileset* getMetatileTileset(int, Tileset*, Tileset*);
    static Tileset* getTileTileset(int, Tileset*, Tileset*);
    static const Tileset* getTileTileset(int, const Tileset*, const Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static Tileset* getMetatileLabelTileset(int, Tileset*, Tileset*);
    static QString getMetatileLabel(int, Tileset *, Tileset *);
    static QString getOwnedMetatileLabel(int, Tileset *, Tileset *);
    static MetatileLabelPair getMetatileLabelPair(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset);
    static bool setMetatileLabel(int, QString, Tileset *, Tileset *);
    QString getMetatileLabelPrefix();
    static QString getMetatileLabelPrefix(const QString &name);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*, bool useTruePalettes = false);
    static QList<QRgb> getPalette(int, Tileset*, Tileset*, bool useTruePalettes = false);
    static bool metatileIsValid(uint16_t metatileId, Tileset *, Tileset *);
    static QHash<int, QString> getHeaderMemberMap(bool usingAsm);
    static QString getExpectedDir(QString tilesetName, bool isSecondary);
    QString getExpectedDir();

    bool load();
    bool loadMetatiles();
    bool loadMetatileAttributes();
    bool loadTilesImage(QImage *importedImage = nullptr);
    bool loadPalettes();

    bool save();
    bool saveMetatileAttributes();
    bool saveMetatiles();
    bool saveTilesImage();
    bool savePalettes();

    bool appendToHeaders(const QString &filepath, const QString &friendlyName, bool usingAsm);
    bool appendToGraphics(const QString &filepath, const QString &friendlyName, bool usingAsm);
    bool appendToMetatiles(const QString &filepath, const QString &friendlyName, bool usingAsm);

    void setTilesImage(const QImage &image);

    void setMetatiles(const QList<Metatile*> &metatiles);
    void addMetatile(Metatile* metatile);

    const QList<Metatile*> &metatiles() const { return m_metatiles; }
    const Metatile* metatileAt(unsigned int i) const { return m_metatiles.at(i); }

    void clearMetatiles();
    void resizeMetatiles(int newNumMetatiles);
    int numMetatiles() const { return m_metatiles.length(); }
    int maxMetatiles() const;

    uint16_t firstMetatileId() const;
    uint16_t lastMetatileId() const;
    bool contains(uint16_t metatileId) const { return metatileId >= firstMetatileId() && metatileId <= lastMetatileId(); }

    int numTiles() const { return m_tiles.length(); }
    int maxTiles() const;

    QImage tileImage(uint16_t tileId) const { return m_tiles.value(Tile::getIndexInTileset(tileId)); }

    QSet<int> getUnusedColorIds(int paletteId, Tileset *pairedTileset, const QSet<int> &searchColors = {}) const;

    static constexpr int maxPalettes() { return 16; }
    static constexpr int numColorsPerPalette() { return 16; }

private:
    QList<Metatile*> m_metatiles;

    QList<QImage> m_tiles;
    QImage m_tilesImage;
    bool m_hasUnsavedTilesImage = false;
};

#endif // TILESET_H
