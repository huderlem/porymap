#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include "selectablepixmapitem.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(Metatile *metatile, Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16, 6, 2) {
        this->metatile = metatile;
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->clearLastModifiedCoords();
    }
    void draw();
    void setTilesets(Tileset*, Tileset*);
    void setMetatile(Metatile*);
    void clearLastModifiedCoords();
private:
    Metatile* metatile;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QPoint prevChangedTile;
    void getBoundedCoords(QPointF, int*, int*);
signals:
    void tileChanged(int, int);
    void selectedTilesChanged(QPoint, int, int);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // METATILELAYERSITEM_H
