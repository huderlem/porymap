#pragma once
#ifndef MAPLAYOUT_H
#define MAPLAYOUT_H

#include "blockdata.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>
#include <QString>

class Map;

class MapLayout {
public:
    MapLayout() {}

    static QString layoutConstantFromName(QString mapName);

    /// !TODO
    /* NEW */
    QList<Map *> maps;

    QString id;
    QString name;

    int width;
    int height;
    int border_width;
    int border_height;

    QString border_path;
    QString blockdata_path;

    QString tileset_primary_label;
    QString tileset_secondary_label;

    Tileset *tileset_primary = nullptr;
    Tileset *tileset_secondary = nullptr;

    Blockdata blockdata;

    QImage border_image;
    QPixmap border_pixmap;

    Blockdata border;
    Blockdata cached_blockdata;
    Blockdata cached_collision;
    Blockdata cached_border;
    struct {
        Blockdata blocks;
        QSize mapDimensions;
        Blockdata border;
        QSize borderDimensions;
    } lastCommitBlocks; // to track map changes

    int getWidth();
    int getHeight();
    int getBorderWidth();
    int getBorderHeight();
};

#endif // MAPLAYOUT_H
