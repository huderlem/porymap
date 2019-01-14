#ifndef REGIONMAPLAYOUTPIXMAPITEM_H
#define REGIONMAPLAYOUTPIXMAPITEM_H

#include "tilemaptileselector.h"
#include "regionmap.h"

class RegionMapLayoutPixmapItem : public SelectablePixmapItem {
    Q_OBJECT
public:
    RegionMapLayoutPixmapItem(RegionMap *rmap, TilemapTileSelector *ts) : SelectablePixmapItem(8, 8, 1, 1) {
            this->region_map = rmap;
            this->tile_selector = ts;
            setAcceptHoverEvents(true);
    }
    RegionMap *region_map;
    TilemapTileSelector *tile_selector;
    int selectedTile;
    int highlightedTile;
    void draw();
    void select(int, int);
    void select(int);
    void highlight(int, int, int);

private:
    void updateSelectedTile();

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
