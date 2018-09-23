#ifndef MOVEMENTPERMISSIONSSELECTOR_H
#define MOVEMENTPERMISSIONSSELECTOR_H

#include "selectablepixmapitem.h"

class MovementPermissionsSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    MovementPermissionsSelector(): SelectablePixmapItem(32, 32, 1, 1) {
        setAcceptHoverEvents(true);
    }
    void draw();
    uint16_t getSelectedCollision();
    uint16_t getSelectedElevation();
    void select(uint16_t collision, uint16_t elevation);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    void setSelectedMovementPermissions(QPointF);

signals:
    void hoveredMovementPermissionChanged(uint16_t, uint16_t);
    void hoveredMovementPermissionCleared();
};

#endif // MOVEMENTPERMISSIONSSELECTOR_H
