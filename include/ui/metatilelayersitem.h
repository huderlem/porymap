#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include "selectablepixmapitem.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(Metatile *metatile,
                       Tileset *primaryTileset,
                       Tileset *secondaryTileset,
                       Qt::Orientation orientation = Qt::Horizontal);
    void draw();
    void setTilesets(Tileset*, Tileset*);
    void setMetatile(Metatile*);
    void clearLastModifiedCoords();
    void clearLastHoveredCoords();

    QPoint tileIndexToPos(int index) const { return this->tilePositions.value(index); }
    int posToTileIndex(const QPoint &pos) const { return this->tilePositions.indexOf(pos); }
    int posToTileIndex(int x, int y) const { return posToTileIndex(QPoint(x, y)); }

    void setOrientation(Qt::Orientation orientation);

    bool showGrid;
private:
    Metatile* metatile;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    Qt::Orientation orientation;
    QPoint prevChangedPos;
    QPoint prevHoveredPos;

    QList<QPoint> tilePositions;

    QPoint getBoundedPos(const QPointF &);
    void updateSelection();
    void hover(const QPoint &pos);
signals:
    void tileChanged(const QPoint &pos);
    void paletteChanged(const QPoint &pos);
    void selectedTilesChanged(const QPoint &pos, const QSize &dimensions);
    void hoveredTileChanged(const Tile &tile);
    void hoveredTileCleared();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
};

#endif // METATILELAYERSITEM_H
