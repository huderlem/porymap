#ifndef TILEMAPTILESELECTOR_H
#define TILEMAPTILESELECTOR_H

#include "selectablepixmapitem.h"

class TilemapTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilemapTileSelector(QPixmap pixmap_): SelectablePixmapItem(8, 8, 1, 1) {
        this->tilemap = pixmap_;
        this->setPixmap(this->tilemap);
        this->numTilesWide = tilemap.width() / 8;
        this->selectedTile = 0x00;
        setAcceptHoverEvents(true);
    }
    void draw();
    void select(unsigned tileId);
    unsigned getSelectedTile();

    int pixelWidth;
    int pixelHeight;

    unsigned selectedTile = 0;

    QPixmap tilemap;
    QImage tileImg(unsigned tileId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    int numTilesWide;
    size_t numTiles;
    void updateSelectedTile();
    unsigned getTileId(int x, int y);
    QPoint getTileIdCoords(unsigned);

signals:
    void hoveredTileChanged(unsigned);
    void hoveredTileCleared();
    void selectedTileChanged(unsigned);
};

#endif // TILEMAPTILESELECTOR_H
