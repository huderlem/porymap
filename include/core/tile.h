#pragma once
#ifndef TILE_H
#define TILE_H

#include <QObject>
#include <QSize>

class Tile
{
public:
    Tile();
    Tile(uint16_t tileId, uint16_t xflip, uint16_t yflip, uint16_t palette);
    Tile(uint16_t raw);

public:
    uint16_t tileId:10;
    uint16_t xflip:1;
    uint16_t yflip:1;
    uint16_t palette:4;
    uint16_t rawValue() const;

    Qt::Orientations orientation() const;
    void flip(QImage *image) const;

    static int getIndexInTileset(int);

    static const uint16_t maxValue;

    static constexpr int pixelWidth() { return 8; }
    static constexpr int pixelHeight() { return 8; }
    static constexpr QSize pixelSize() { return QSize(Tile::pixelWidth(), Tile::pixelHeight()); }
    static constexpr int numPixels() { return Tile::pixelWidth() * Tile::pixelHeight(); }
    static constexpr int sizeInBytes() { return sizeof(uint16_t); }
};

inline bool operator==(const Tile &a, const Tile &b) {
    return a.tileId  == b.tileId &&
           a.xflip   == b.xflip &&
           a.yflip   == b.yflip &&
           a.palette == b.palette;
}

inline bool operator!=(const Tile &a, const Tile &b) {
    return !(operator==(a, b));
}

#endif // TILE_H
