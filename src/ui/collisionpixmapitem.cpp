#include "collisionpixmapitem.h"
#include "editcommands.h"

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
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    this->paint_tile_initial_x = this->straight_path_initial_x = x;
    this->paint_tile_initial_y = this->straight_path_initial_y = y;
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    this->lockedAxis = CollisionPixmapItem::Axis::None;
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::draw(bool ignoreCache) {
    if (map) {
        map->setCollisionItem(this);
        setPixmap(map->renderCollision(*this->opacity, ignoreCache));
    }
}

void CollisionPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        actionId_++;
    } else if (map) {
        Blockdata *oldCollision = map->layout->blockdata->copy();

        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;

        // Set straight paths on/off and snap to the dominant axis when on
        if (event->modifiers() & Qt::ControlModifier) {
            this->lockNondominantAxis(event);
            x = this->adjustCoord(x, MapPixmapItem::Axis::X);
            y = this->adjustCoord(y, MapPixmapItem::Axis::Y);
        } else {
            this->prevStraightPathState = false;
            this->lockedAxis = MapPixmapItem::Axis::None;
        }

        Block *block = map->getBlock(x, y);
        if (block) {
            block->collision = this->movementPermissionsSelector->getSelectedCollision();
            block->elevation = this->movementPermissionsSelector->getSelectedElevation();
            map->setBlock(x, y, *block, true);
        }

        Blockdata *newCollision = map->layout->blockdata->copy();
        if (newCollision->equals(oldCollision)) {
            delete newCollision;
            delete oldCollision;
        } else {
            map->editHistory.push(new PaintCollision(map, oldCollision, newCollision, actionId_));
        }
    }
}

void CollisionPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        this->actionId_++;
    } else if (map) {
        Blockdata *oldCollision = map->layout->blockdata->copy();

        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->floodFillCollisionElevation(x, y, collision, elevation);

        Blockdata *newCollision = map->layout->blockdata->copy();
        if (newCollision->equals(oldCollision)) {
            delete newCollision;
            delete oldCollision;
        } else {
            map->editHistory.push(new BucketFillCollision(map, oldCollision, newCollision));
        }
    }
}

void CollisionPixmapItem::magicFill(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        this->actionId_++;
    } else if (map) {
        Blockdata *oldCollision = map->layout->blockdata->copy();
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 16;
        int y = static_cast<int>(pos.y()) / 16;
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->magicFillCollisionElevation(x, y, collision, elevation);

        Blockdata *newCollision = map->layout->blockdata->copy();
        if (newCollision->equals(oldCollision)) {
            delete newCollision;
            delete oldCollision;
        } else {
            map->editHistory.push(new MagicFillCollision(map, oldCollision, newCollision));
        }
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
