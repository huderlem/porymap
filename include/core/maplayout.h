#ifndef MAPLAYOUT_H
#define MAPLAYOUT_H

#include "blockdata.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>
#include <QString>

class MapLayout {
public:
    MapLayout() {}
    static QString layoutConstantFromName(QString mapName);
    QString id;
    QString name;
    QString width;
    QString height;
    QString border_width;
    QString border_height;
    QString border_path;
    QString blockdata_path;
    QString tileset_primary_label;
    QString tileset_secondary_label;
    Tileset *tileset_primary = nullptr;
    Tileset *tileset_secondary = nullptr;
    Blockdata* blockdata = nullptr;
    QImage border_image;
    QPixmap border_pixmap;
    Blockdata *border = nullptr;
    Blockdata *cached_blockdata = nullptr;
    Blockdata *cached_collision = nullptr;
    Blockdata *cached_border = nullptr;
    bool has_unsaved_changes = false;
};

#endif // MAPLAYOUT_H
