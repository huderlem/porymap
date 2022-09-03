#include "movementpermissionsselector.h"
#include <QPainter>

void MovementPermissionsSelector::draw() {
    QPixmap pixmap(":/images/collisions.png");
    this->setPixmap(pixmap.scaled(64, 512));
    this->drawSelection();
}

uint16_t MovementPermissionsSelector::getSelectedCollision() {
    return static_cast<uint16_t>(this->selectionInitialX);
}

uint16_t MovementPermissionsSelector::getSelectedElevation() {
    return static_cast<uint16_t>(this->selectionInitialY);
}

void MovementPermissionsSelector::select(uint16_t collision, uint16_t elevation) {
    SelectablePixmapItem::select(collision != 0, elevation, 0, 0);
}

void MovementPermissionsSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t collision = static_cast<uint16_t>(pos.x());
    uint16_t elevation = static_cast<uint16_t>(pos.y());
    emit this->hoveredMovementPermissionChanged(collision, elevation);
}

void MovementPermissionsSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit this->hoveredMovementPermissionCleared();
}
