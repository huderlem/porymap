#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include "selectablepixmapitem.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(uint16_t metatileId,
                       Tileset *primaryTileset,
                       Tileset *secondaryTileset,
                       Qt::Orientation orientation = Qt::Vertical);

    void draw() override;
    void setTilesets(Tileset*, Tileset*);
    void setMetatileId(uint16_t);

    bool hasCursor() const { return this->cursorCellPos != QPoint(-1,-1); }
    Tile tileUnderCursor() const;

    QPoint tileIndexToPos(int index) const { return this->tilePositions.value(index); }
    int posToTileIndex(const QPoint &pos) const { return this->tilePositions.indexOf(pos); }
    int posToTileIndex(int x, int y) const { return posToTileIndex(QPoint(x, y)); }

    void setOrientation(Qt::Orientation orientation);

    bool showGrid;
private:
    uint16_t metatileId = 0;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    Qt::Orientation orientation;

    QPoint cursorCellPos = QPoint(-1,-1);

    QList<QPoint> tilePositions;

    QPoint getBoundedPos(const QPointF &);
    void updateSelection();
    bool setCursorCellPos(const QPoint &pos);
    Metatile* getMetatile() const { return Tileset::getMetatile(this->metatileId, this->primaryTileset, this->secondaryTileset); }
signals:
    void tileChanged(const QPoint &pos);
    void paletteChanged(const QPoint &pos);
    void selectedTilesChanged(const QPoint &pos, const QSize &dimensions);
    void hoveredTileChanged(const Tile &tile);
    void hoveredTileCleared();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
};

#endif // METATILELAYERSITEM_H
