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
    int index;
    QString name;
    QString label;
    QString width;
    QString height;
    QString border_label;
    QString border_path;
    QString blockdata_label;
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
public:
    static QString getNameFromLabel(QString label) {
        // ASSUMPTION: strip off "_Layout" from layout label. Directories in 'data/layouts/' must be well-formed.
        return label.replace(label.lastIndexOf("_Layout"), label.length(), "");
    }
};

#endif // MAPLAYOUT_H
