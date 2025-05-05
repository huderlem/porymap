#ifndef TILESETEDITORMETATILESELECTOR_H
#define TILESETEDITORMETATILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class Layout;

class TilesetEditorMetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorMetatileSelector(Tileset *primaryTileset, Tileset *secondaryTileset, Layout *layout);
    Layout *layout = nullptr;

    void draw() override;
    void drawMetatile(uint16_t metatileId);
    void drawSelectedMetatile();

    bool select(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*);
    uint16_t getSelectedMetatileId();
    void updateSelectedMetatile();
    QPoint getMetatileIdCoordsOnWidget(uint16_t metatileId);
    QImage buildPrimaryMetatilesImage();
    QImage buildSecondaryMetatilesImage();

    QVector<uint16_t> usedMetatiles;
    bool selectorShowUnused = false;
    bool selectorShowCounts = false;
    bool showGrid = false;
    bool showDivider = false;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;

private:
    QImage baseImage;
    QPixmap basePixmap;
    Tileset *primaryTileset = nullptr;
    Tileset *secondaryTileset = nullptr;
    uint16_t selectedMetatileId;
    int numMetatilesWide;
    int numMetatilesHigh;
    void updateBasePixmap();
    uint16_t getMetatileId(int x, int y);
    QPoint getMetatileIdCoords(uint16_t);
    bool shouldAcceptEvent(QGraphicsSceneMouseEvent*);
    int numRows(int numMetatiles);
    int numRows();
    void drawGrid();
    void drawDivider();
    void drawFilters();
    void drawUnused();
    void drawCounts();
    QImage buildAllMetatilesImage();
    QImage buildImage(int metatileIdStart, int numMetatiles);
    int numPrimaryMetatilesRounded() const;

signals:
    void hoveredMetatileChanged(uint16_t);
    void hoveredMetatileCleared();
    void selectedMetatileChanged(uint16_t);
};

#endif // TILESETEDITORMETATILESELECTOR_H
