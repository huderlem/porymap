#ifndef TILEMAPTILESELECTOR_H
#define TILEMAPTILESELECTOR_H

#include "selectablepixmapitem.h"

class TilemapTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilemapTileSelector(QPixmap pixmap): SelectablePixmapItem(8, 8, 1, 1) {
        this->pixmap = pixmap;
        this->numTilesWide = pixmap.width() / 8;
        this->selectedTile = 0x00;
        setAcceptHoverEvents(true);
    }
    void draw();
    void select(unsigned tileId);
    unsigned getSelectedTile();

    int pixelWidth;
    int pixelHeight;

    unsigned selectedTile = 0;

    // TODO: which of these need to be made public?
    // call this tilemap? or is tilemap the binary file?
    QPixmap pixmap;// pointer?
    QImage currTile;// image of just the currently selected tile to draw onto graphicsview
    QImage tileImg(unsigned tileId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    int numTilesWide;
    int numTiles;
    void updateSelectedTile();
    unsigned getTileId(int x, int y);
    QPoint getTileIdCoords(unsigned);
    unsigned getValidTileId(unsigned);// TODO: implement this to prevent segfaults

signals:
    void hoveredTileChanged(unsigned);
    void hoveredTileCleared();
    void selectedTileChanged(unsigned);
};

#endif // TILEMAPTILESELECTOR_H
