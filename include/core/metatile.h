#pragma once
#ifndef METATILE_H
#define METATILE_H

#include "tile.h"
#include <QImage>
#include <QPoint>
#include <QString>

class Metatile {
public:
    Metatile();
    Metatile(const Metatile& other) = default;
    Metatile& operator=(const Metatile& other) = default;

public:
    QList<Tile> tiles;
    uint16_t behavior; // 8 bits RSE, 9 bits FRLG
    uint8_t layerType;
    uint8_t encounterType; // FRLG only
    uint8_t terrainType; // FRLG only
    QString label;

    static int getBlockIndex(int);
    static QPoint coordFromPixmapCoord(const QPointF& pixelCoord);
};

#endif // METATILE_H
