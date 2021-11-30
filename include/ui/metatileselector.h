#ifndef METATILESELECTOR_H
#define METATILESELECTOR_H

#include <QPair>
#include "selectablepixmapitem.h"
#include "map.h"
#include "tileset.h"
#include "maplayout.h"

class MetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileSelector(int numMetatilesWide, Map *map): SelectablePixmapItem(16, 16) {
        this->externalSelection = false;
        this->numMetatilesWide = numMetatilesWide;
        this->map = map;
        this->primaryTileset = map->layout->tileset_primary;
        this->secondaryTileset = map->layout->tileset_secondary;
        this->selectedMetatiles = new QList<uint16_t>();
        this->selectedCollisions = new QList<QPair<uint16_t, uint16_t>>();
        this->externalSelectedMetatiles = new QList<uint16_t>();
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    bool select(uint16_t metatile);
    bool selectFromMap(uint16_t metatileId, uint16_t collision, uint16_t elevation);
    void setTilesets(Tileset*, Tileset*);
    QList<uint16_t>* getSelectedMetatiles();
    QList<QPair<uint16_t, uint16_t>>* getSelectedCollisions();
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
    int numMetatilesWide;
    Map *map;
    QList<uint16_t> *selectedMetatiles;
    QList<QPair<uint16_t, uint16_t>> *selectedCollisions;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<uint16_t> *externalSelectedMetatiles;

    void updateSelectedMetatiles();
    void updateExternalSelectedMetatiles();
    uint16_t getMetatileId(int x, int y);
    QPoint getMetatileIdCoords(uint16_t);
    bool shouldAcceptEvent(QGraphicsSceneMouseEvent*);
    bool selectionIsValid();

signals:
    void hoveredMetatileSelectionChanged(uint16_t);
    void hoveredMetatileSelectionCleared();
    void selectedMetatilesChanged();
};

#endif // METATILESELECTOR_H
