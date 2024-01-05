#ifndef METATILESELECTOR_H
#define METATILESELECTOR_H

#include <QPair>
#include "selectablepixmapitem.h"
#include "map.h"
#include "tileset.h"
#include "maplayout.h"

struct MetatileSelectionItem
{
    bool enabled;
    uint16_t metatileId;
};

struct CollisionSelectionItem
{
    bool enabled;
    uint16_t collision;
    uint16_t elevation;
};

struct MetatileSelection
{
    QPoint dimensions;
    bool hasCollision;
    QList<MetatileSelectionItem> metatileItems;
    QList<CollisionSelectionItem> collisionItems;
};

class MetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileSelector(int numMetatilesWide, Map *map): SelectablePixmapItem(16, 16) {
        this->externalSelection = false;
        this->prefabSelection = false;
        this->numMetatilesWide = numMetatilesWide;
        this->map = map;
        this->primaryTileset = map->layout->tileset_primary;
        this->secondaryTileset = map->layout->tileset_secondary;
        this->selection = MetatileSelection{};
        this->cellPos = QPoint(-1, -1);
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    bool select(uint16_t metatile);
    bool selectFromMap(uint16_t metatileId, uint16_t collision, uint16_t elevation);
    void setTilesets(Tileset*, Tileset*);
    MetatileSelection getMetatileSelection();
    void setPrefabSelection(MetatileSelection selection);
    void setExternalSelection(int, int, QList<uint16_t>, QList<QPair<uint16_t, uint16_t>>);
    QPoint getMetatileIdCoordsOnWidget(uint16_t);
    void setMap(Map*);
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
private:
    bool externalSelection;
    bool prefabSelection;
    int numMetatilesWide;
    Map *map;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<uint16_t> externalSelectedMetatiles;
    MetatileSelection selection;
    QPoint cellPos;

    void updateSelectedMetatiles();
    void updateExternalSelectedMetatiles();
    uint16_t getMetatileId(int x, int y) const;
    QPoint getMetatileIdCoords(uint16_t);
    bool positionIsValid(const QPoint &pos) const;
    bool selectionIsValid();
    void hoverChanged();

signals:
    void hoveredMetatileSelectionChanged(uint16_t);
    void hoveredMetatileSelectionCleared();
    void selectedMetatilesChanged();
};

#endif // METATILESELECTOR_H
