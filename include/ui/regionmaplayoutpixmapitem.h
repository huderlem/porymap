#ifndef REGIONMAPLAYOUTPIXMAPITEM_H
#define REGIONMAPLAYOUTPIXMAPITEM_H

#include "tilemaptileselector.h"
//#include "regionmappixmapitem.h"
#include "regionmapeditor.h"

class RegionMapLayoutPixmapItem : public SelectablePixmapItem {
    Q_OBJECT
public:
    RegionMapLayoutPixmapItem(RegionMap *rmap, TilemapTileSelector *ts) : SelectablePixmapItem(8, 8, 1, 1) {
            //
            this->region_map = rmap;
            this->tile_selector = ts;
            setAcceptHoverEvents(true);
    }
    RegionMap *region_map;// inherited from RegionMapPixmapItem?
    TilemapTileSelector *tile_selector;
    int selectedTile;// index in map_squares
    void draw();
    void select(int, int);
    void setDefaultSelection();

private:
    void updateSelectedTile();

// can I implement these if they are virtual?
signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, RegionMapLayoutPixmapItem *);
    void hoveredTileChanged(int);
    void hoveredTileCleared();
    void selectedTileChanged(int);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // REGIONMAPLAYOUTPIXMAPITEM_H
