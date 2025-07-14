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
    MetatileSelector(int numMetatilesWide, Layout *layout): SelectablePixmapItem(Metatile::pixelWidth(), Metatile::pixelHeight()) {
        this->externalSelection = false;
        this->prefabSelection = false;
        this->numMetatilesWide = numMetatilesWide;
        this->layout = layout;
        this->selection = MetatileSelection{};
        this->cellPos = QPoint(-1, -1);
        setAcceptHoverEvents(true);
    }

    QPoint getSelectionDimensions() override;
    void draw() override;
    void refresh();

    bool select(uint16_t metatile);
    void selectFromMap(uint16_t metatileId, uint16_t collision, uint16_t elevation);
    MetatileSelection getMetatileSelection();
    void setPrefabSelection(MetatileSelection selection);
    void setExternalSelection(int, int, QList<uint16_t>, QList<QPair<uint16_t, uint16_t>>);
    QPoint getMetatileIdCoordsOnWidget(uint16_t);
    void setLayout(Layout *layout);
    bool isInternalSelection() const { return (!this->externalSelection && !this->prefabSelection); }

    Tileset *primaryTileset() const { return this->layout->tileset_primary; }
    Tileset *secondaryTileset() const { return this->layout->tileset_secondary; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
    void drawSelection() override;
private:
    QPixmap basePixmap;
    bool externalSelection;
    bool prefabSelection;
    int numMetatilesWide;
    Layout *layout;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<uint16_t> externalSelectedMetatiles;
    MetatileSelection selection;
    QPoint cellPos;

    void updateBasePixmap();
    void updateSelectedMetatiles();
    void updateExternalSelectedMetatiles();
    uint16_t getMetatileId(int x, int y) const;
    QPoint getMetatileIdCoords(uint16_t);
    bool positionIsValid(const QPoint &pos) const;
    bool selectionIsValid();
    void hoverChanged();
    int numPrimaryMetatilesRounded() const;

signals:
    void hoveredMetatileSelectionChanged(uint16_t);
    void hoveredMetatileSelectionCleared();
    void selectedMetatilesChanged();
};

#endif // METATILESELECTOR_H
