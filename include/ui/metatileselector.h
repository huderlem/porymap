#ifndef METATILESELECTOR_H
#define METATILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class MetatileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    MetatileSelector(int numMetatilesWide, Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16) {
        this->externalSelection = false;
        this->numMetatilesWide = numMetatilesWide;
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->selectedMetatiles = new QList<uint16_t>();
        this->externalSelectedMetatiles = new QList<uint16_t>();
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    void select(uint16_t metatile);
    void setTilesets(Tileset*, Tileset*);
    QList<uint16_t>* getSelectedMetatiles();
    void setExternalSelection(int, int, QList<uint16_t>*);
    QPoint getMetatileIdCoordsOnWidget(uint16_t);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
private:
    bool externalSelection;
    int numMetatilesWide;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QList<uint16_t> *selectedMetatiles;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<uint16_t> *externalSelectedMetatiles;

    void updateSelectedMetatiles();
    uint16_t getMetatileId(int x, int y);
    QPoint getMetatileIdCoords(uint16_t);

signals:
    void hoveredMetatileSelectionChanged(uint16_t);
    void hoveredMetatileSelectionCleared();
    void selectedMetatilesChanged();
};

#endif // METATILESELECTOR_H
