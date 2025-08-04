#ifndef TILESETEDITORMETATILESELECTOR_H
#define TILESETEDITORMETATILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class Layout;

class TilesetEditorMetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorMetatileSelector(int numMetatilesWide, Tileset *primaryTileset, Tileset *secondaryTileset, Layout *layout);
    Layout *layout = nullptr;

    void draw() override;
    void drawMetatile(uint16_t metatileId);
    void drawSelectedMetatile();

    bool select(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*);
    uint16_t getSelectedMetatileId() const { return this->selectedMetatileId; }
    QPoint getMetatileIdCoordsOnWidget(uint16_t metatileId) const;

    void setSwapMode(bool enabeled);
    void addToSwapSelection(uint16_t metatileId);
    void removeFromSwapSelection(uint16_t metatileId);
    void clearSwapSelection();

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
    const int numMetatilesWide;
    QImage baseImage;
    QPixmap basePixmap;
    Tileset *primaryTileset = nullptr;
    Tileset *secondaryTileset = nullptr;
    uint16_t selectedMetatileId = 0;
    QPoint prevCellPos = QPoint(-1,-1);

    QList<uint16_t> swapMetatileIds;
    uint16_t lastHoveredMetatileId = 0;
    bool inSwapMode = false;

    void updateBasePixmap();
    uint16_t posToMetatileId(int x, int y, bool *ok = nullptr) const;
    uint16_t posToMetatileId(const QPoint &pos, bool *ok = nullptr) const;
    QPoint metatileIdToPos(uint16_t metatileId, bool *ok = nullptr) const;
    bool isValidMetatileId(uint16_t metatileId) const;
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
    void swapRequested(uint16_t, uint16_t);
};

#endif // TILESETEDITORMETATILESELECTOR_H
