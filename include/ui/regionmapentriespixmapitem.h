#ifndef REGIONMAPENTRIESPIXMAPITEM_H
#define REGIONMAPENTRIESPIXMAPITEM_H

#include "tilemaptileselector.h"
#include "regionmap.h"

class RegionMapEntriesPixmapItem : public SelectablePixmapItem {
    Q_OBJECT
public:
    RegionMapEntriesPixmapItem(RegionMap *rm, TilemapTileSelector *ts) : SelectablePixmapItem(8, 8, 1, 1) {
            this->region_map = rm;
            this->tile_selector = ts;
    }
    RegionMap *region_map;
    TilemapTileSelector *tile_selector;
    QString currentSection = QString();
    int selectedTile;
    int highlightedTile;
    int pressedX;
    int pressedY;
    void draw();
    void select(int, int);
    void select(int);
    bool draggingEntry = false;

private:
    void updateSelectedTile();

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, RegionMapEntriesPixmapItem *);
    void hoveredTileChanged(int);
    void hoveredTileCleared();
    void selectedTileChanged(QString);
    void entryPositionChanged(int, int);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // REGIONMAPENTRIESPIXMAPITEM_H
