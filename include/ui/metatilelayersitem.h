#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include "selectablepixmapitem.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(Metatile *metatile, Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16, 2, 2) {
        this->metatile = metatile;
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
    }
    void draw();
    void setTilesets(Tileset*, Tileset*);
    void setMetatile(Metatile*);
private:
    Metatile* metatile;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
signals:
    void tileChanged(int, int);
    void selectedTilesChanged(QPoint, int, int);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // METATILELAYERSITEM_H
