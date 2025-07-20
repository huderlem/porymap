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
    uint16_t getSelectedMetatileId() const { return this->selectedMetatileId; }
    void updateSelectedMetatile();
    QPoint getMetatileIdCoordsOnWidget(uint16_t metatileId) const;

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
    uint16_t posToMetatileId(int x, int y, bool *ok = nullptr) const;
    uint16_t posToMetatileId(const QPoint &pos, bool *ok = nullptr) const;
    QPoint metatileIdToPos(uint16_t metatileId, bool *ok = nullptr) const;
    bool shouldAcceptEvent(QGraphicsSceneMouseEvent*);
    int numRows(int numMetatiles) const;
    int numRows() const;
    void drawGrid();
    void drawDivider();
    void drawFilters();
    void drawUnused();
    void drawCounts();
    int numPrimaryMetatilesRounded() const;

signals:
    void hoveredMetatileChanged(uint16_t);
    void hoveredMetatileCleared();
    void selectedMetatileChanged(uint16_t);
};

#endif // TILESETEDITORMETATILESELECTOR_H
