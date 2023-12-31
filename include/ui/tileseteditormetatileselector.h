#ifndef TILESETEDITORMETATILESELECTOR_H
#define TILESETEDITORMETATILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"
#include "map.h"

class TilesetEditorMetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorMetatileSelector(Tileset *primaryTileset, Tileset *secondaryTileset, Map *map);
    Map *map = nullptr;
    void draw();
    bool select(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*, bool draw = true);
    uint16_t getSelectedMetatileId();
    void updateSelectedMetatile();
    QPoint getMetatileIdCoordsOnWidget(uint16_t metatileId);
    QImage buildPrimaryMetatilesImage();
    QImage buildSecondaryMetatilesImage();

    QVector<uint16_t> usedMetatiles;
    bool selectorShowUnused = false;
    bool selectorShowCounts = false;
    bool showGrid;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    Tileset *primaryTileset = nullptr;
    Tileset *secondaryTileset = nullptr;
    uint16_t selectedMetatile;
    int numMetatilesWide;
    int numMetatilesHigh;
    uint16_t getMetatileId(int x, int y);
    QPoint getMetatileIdCoords(uint16_t);
    bool shouldAcceptEvent(QGraphicsSceneMouseEvent*);
    int numRows(int numMetatiles);
    int numRows();
    void drawGrid();
    void drawFilters();
    void drawUnused();
    void drawCounts();
    QImage buildAllMetatilesImage();
    QImage buildImage(int metatileIdStart, int numMetatiles);

signals:
    void hoveredMetatileChanged(uint16_t);
    void hoveredMetatileCleared();
    void selectedMetatileChanged(uint16_t);
};

#endif // TILESETEDITORMETATILESELECTOR_H
