#include "collisionpixmapitem.h"

void CollisionPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    int x = static_cast<int>(event->pos().x()) / 16;
    int y = static_cast<int>(event->pos().y()) / 16;
    emit this->hoveredMapMovementPermissionChanged(x, y);
    if (this->settings->betterCursors && this->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        setCursor(this->settings->mapCursor);
    }
}
void CollisionPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit this->hoveredMapMovementPermissionCleared();
    if (this->settings->betterCursors && this->paintingMode == MapPixmapItem::PaintMode::Metatiles){
        unsetCursor();
    }
}
void CollisionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
void CollisionPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
void CollisionPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::draw(bool ignoreCache) {
    if (map) {
        setPixmap(map->renderCollision(*this->opacity, ignoreCache));
    }
}

void CollisionPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;
        Block *block = map->getBlock(x, y);
        if (block) {
            block->collision = this->movementPermissionsSelector->getSelectedCollision();
            block->elevation = this->movementPermissionsSelector->getSelectedElevation();
            map->setBlock(x, y, *block, true);
        }
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            map->commit();
        }
        draw();
    }
}

void CollisionPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->floodFillCollisionElevation(x, y, collision, elevation);
        draw();
    }
}

void CollisionPixmapItem::magicFill(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->magicFillCollisionElevation(x, y, collision, elevation);
        draw();
    }
}

void CollisionPixmapItem::pick(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    Block *block = map->getBlock(x, y);
    if (block) {
        this->movementPermissionsSelector->select(block->collision, block->elevation);
    }
}

void CollisionPixmapItem::updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;

    // Snap point to within map bounds.
    if (x < 0) x = 0;
    if (x >= map->getWidth()) x = map->getWidth() - 1;
    if (y < 0) y = 0;
    if (y >= map->getHeight()) y = map->getHeight() - 1;

    Block *block = map->getBlock(x, y);
    this->movementPermissionsSelector->select(block->collision, block->elevation);
}
