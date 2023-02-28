#ifndef STAMPSELECTOR_H
#define STAMPSELECTOR_H

#include "selectablepixmapitem.h"
#include "map.h"
#include "tileset.h"
#include "maplayout.h"
#include "paintselection.h"

class StampSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    StampSelector(int numStampsWide, Map *map): SelectablePixmapItem(16, 16) {
        this->numStampsWide = numStampsWide;
        this->map = map;
        this->primaryTileset = map->layout->tileset_primary;
        this->secondaryTileset = map->layout->tileset_secondary;
        this->selection = StampSelection{};
    }
    QPoint getSelectionDimensions();
    void draw();
    bool select(uint16_t stampId);
    void setTilesets(Tileset*, Tileset*);
    StampSelection getStampSelection();
    QPoint getStampIdCoordsOnWidget(uint16_t);
    void setMap(Map*);
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
private:
    int numStampsWide;
    Map *map;
    StampSelection selection;

    void updateSelectedStamps();
    uint16_t getStampId(int x, int y);
    QPoint getStampIdCoords(uint16_t);
    bool shouldAcceptEvent(QGraphicsSceneMouseEvent*);

signals:
    void selectedStampsChanged();
};

#endif // STAMPSELECTOR_H
