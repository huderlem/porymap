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
    SelectablePixmapItem::select(collision, elevation, 0, 0);
}

void MovementPermissionsSelector::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->setSelectedMovementPermissions(event->pos());
}

void MovementPermissionsSelector::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->setSelectedMovementPermissions(event->pos());
}

void MovementPermissionsSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->setSelectedMovementPermissions(event->pos());
}

void MovementPermissionsSelector::setSelectedMovementPermissions(QPointF pos) {
    int x = static_cast<int>(pos.x()) / 32;
    int y = static_cast<int>(pos.y()) / 32;
    int width = this->pixmap().width() / 32;
    int height = this->pixmap().height() / 32;

    // Snap to a valid position inside the selection area.
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;
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
