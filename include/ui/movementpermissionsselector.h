#ifndef MOVEMENTPERMISSIONSSELECTOR_H
#define MOVEMENTPERMISSIONSSELECTOR_H

#include "selectablepixmapitem.h"

class MovementPermissionsSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    MovementPermissionsSelector(QPixmap basePixmap)
    : SelectablePixmapItem(MovementPermissionsSelector::CellWidth, MovementPermissionsSelector::CellHeight, 1, 1) {
        this->basePixmap = basePixmap;
        setAcceptHoverEvents(true);
    }
    void draw();
    uint16_t getSelectedCollision();
    uint16_t getSelectedElevation();
    void select(uint16_t collision, uint16_t elevation);
    void setBasePixmap(QPixmap pixmap);

    static const int CellWidth;
    static const int CellHeight;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    void setSelectedMovementPermissions(QPointF);
    QPixmap basePixmap;

signals:
    void hoveredMovementPermissionChanged(uint16_t, uint16_t);
    void hoveredMovementPermissionCleared();
};

#endif // MOVEMENTPERMISSIONSSELECTOR_H
