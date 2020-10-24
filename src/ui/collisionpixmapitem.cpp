#include "collisionpixmapitem.h"
#include "editcommands.h"
#include "metatile.h"

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
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    this->paint_tile_initial_x = this->straight_path_initial_x = pos.x();
    this->paint_tile_initial_y = this->straight_path_initial_y = pos.y();
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

        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

        // Set straight paths on/off and snap to the dominant axis when on
        if (event->modifiers() & Qt::ControlModifier) {
            this->lockNondominantAxis(event);
            pos = this->adjustCoords(pos);
        } else {
            this->prevStraightPathState = false;
            this->lockedAxis = MapPixmapItem::Axis::None;
        }

        Block *block = map->getBlock(pos.x(), pos.y());
        if (block) {
            block->collision = this->movementPermissionsSelector->getSelectedCollision();
            block->elevation = this->movementPermissionsSelector->getSelectedElevation();
            map->setBlock(pos.x(), pos.y(), *block, true);
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

        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->floodFillCollisionElevation(pos.x(), pos.y(), collision, elevation);

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
        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        map->magicFillCollisionElevation(pos.x(), pos.y(), collision, elevation);

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
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    Block *block = map->getBlock(pos.x(), pos.y());
    if (block) {
        this->movementPermissionsSelector->select(block->collision, block->elevation);
    }
}

void CollisionPixmapItem::updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

    // Snap point to within map bounds.
    if (pos.x() < 0) pos.setX(0);
    if (pos.x() >= map->getWidth()) pos.setX(map->getWidth() - 1);
    if (pos.y() < 0) pos.setY(0);
    if (pos.y() >= map->getHeight()) pos.setY(map->getHeight() - 1);

    Block *block = map->getBlock(pos.x(), pos.y());
    this->movementPermissionsSelector->select(block->collision, block->elevation);
}
