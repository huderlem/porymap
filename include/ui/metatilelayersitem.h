#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include "selectablepixmapitem.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(Metatile *metatile, Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16, 6, 2) {
        this->metatile = metatile;
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->clearLastModifiedCoords();
        this->clearLastHoveredCoords();
        setAcceptHoverEvents(true);
    }
    void draw();
    void setTilesets(Tileset*, Tileset*);
    void setMetatile(Metatile*);
    void clearLastModifiedCoords();
    void clearLastHoveredCoords();
    bool showGrid;
private:
    Metatile* metatile;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QPoint prevChangedPos;
    QPoint prevHoveredPos;
    QPoint getBoundedPos(const QPointF &);
signals:
    void tileChanged(int, int);
    void selectedTilesChanged(QPoint, int, int);
    void hoveredTileChanged(uint16_t);
    void hoveredTileCleared();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
};

#endif // METATILELAYERSITEM_H
