#ifndef REGIONMAPPIXMAPITEM_H
#define REGIONMAPPIXMAPITEM_H

#include "regionmap.h"
#include "tilemaptileselector.h"
#include <QGraphicsPixmapItem>

class RegionMapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

private:
    using QGraphicsPixmapItem::paint;

public:
    RegionMapPixmapItem(RegionMap *rmap, TilemapTileSelector *tile_selector) {
        this->region_map = rmap;
        this->tile_selector = tile_selector;
        setAcceptHoverEvents(true);
    }
    RegionMap *region_map;
    TilemapTileSelector *tile_selector;
    
    virtual void paint(QGraphicsSceneMouseEvent *);
    virtual void select(QGraphicsSceneMouseEvent *);
    virtual void draw();

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, RegionMapPixmapItem *);
    void hoveredRegionMapTileChanged(int x, int y);
    void hoveredRegionMapTileCleared();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
    void mousePressEvent(QGraphicsSceneMouseEvent *);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
};

#endif // REGIONMAPPIXMAPITEM_H
