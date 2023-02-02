#include "collisionpixmapitem.h"
#include "editcommands.h"
#include "metatile.h"

void CollisionPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    if (pos != this->previousPos) {
        this->previousPos = pos;
        emit this->hoveredMapMovementPermissionChanged(pos.x(), pos.y());
    }
    if (this->settings->betterCursors && this->paintingMode == LayoutPixmapItem::PaintMode::Metatiles) {
        setCursor(this->settings->mapCursor);
    }
}

void CollisionPixmapItem::hoverEnterEvent(QGraphicsSceneHoverEvent * event) {
    this->has_mouse = true;
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    emit this->hoveredMapMovementPermissionChanged(pos.x(), pos.y());
}

void CollisionPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit this->hoveredMapMovementPermissionCleared();
    if (this->settings->betterCursors && this->paintingMode == LayoutPixmapItem::PaintMode::Metatiles){
        unsetCursor();
    }
    this->has_mouse = false;
}

void CollisionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    this->paint_tile_initial_x = this->straight_path_initial_x = pos.x();
    this->paint_tile_initial_y = this->straight_path_initial_y = pos.y();
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    if (pos != this->previousPos) {
        this->previousPos = pos;
        emit this->hoveredMapMovementPermissionChanged(pos.x(), pos.y());
    }
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    this->lockedAxis = CollisionPixmapItem::Axis::None;
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::draw(bool ignoreCache) {
    if (this->layout) {
        // !TODO
        this->layout->setCollisionItem(this);
        setPixmap(this->layout->renderCollision(ignoreCache));
        setOpacity(*this->opacity);
    }
}

void CollisionPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        actionId_++;
    } else if (this->layout) {
        Blockdata oldCollision = this->layout->blockdata;

        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

        // Set straight paths on/off and snap to the dominant axis when on
        if (event->modifiers() & Qt::ControlModifier) {
            this->lockNondominantAxis(event);
            pos = this->adjustCoords(pos);
        } else {
            this->prevStraightPathState = false;
            this->lockedAxis = LayoutPixmapItem::Axis::None;
        }

        Block block;
        if (this->layout->getBlock(pos.x(), pos.y(), &block)) {
            block.collision = this->movementPermissionsSelector->getSelectedCollision();
            block.elevation = this->movementPermissionsSelector->getSelectedElevation();
            this->layout->setBlock(pos.x(), pos.y(), block, true);
        }

        if (this->layout->blockdata != oldCollision) {
            this->layout->editHistory.push(new PaintCollision(this->layout, oldCollision, this->layout->blockdata, actionId_));
        }
    }
}

void CollisionPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        this->actionId_++;
    } else if (this->layout) {
        Blockdata oldCollision = this->layout->blockdata;

        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        this->layout->floodFillCollisionElevation(pos.x(), pos.y(), collision, elevation);

        if (this->layout->blockdata != oldCollision) {
            this->layout->editHistory.push(new BucketFillCollision(this->layout, oldCollision, this->layout->blockdata));
        }
    }
}

void CollisionPixmapItem::magicFill(QGraphicsSceneMouseEvent *event) {
    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        this->actionId_++;
    } else if (this->layout) {
        Blockdata oldCollision = this->layout->blockdata;
        QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
        uint16_t collision = this->movementPermissionsSelector->getSelectedCollision();
        uint16_t elevation = this->movementPermissionsSelector->getSelectedElevation();
        this->layout->magicFillCollisionElevation(pos.x(), pos.y(), collision, elevation);

        if (this->layout->blockdata != oldCollision) {
            this->layout->editHistory.push(new MagicFillCollision(this->layout, oldCollision, this->layout->blockdata));
        }
    }
}

void CollisionPixmapItem::pick(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    Block block;
    if (this->layout->getBlock(pos.x(), pos.y(), &block)) {
        this->movementPermissionsSelector->select(block.collision, block.elevation);
    }
}

void CollisionPixmapItem::updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

    // Snap point to within map bounds.
    if (pos.x() < 0) pos.setX(0);
    if (pos.x() >= this->layout->getWidth()) pos.setX(this->layout->getWidth() - 1);
    if (pos.y() < 0) pos.setY(0);
    if (pos.y() >= this->layout->getHeight()) pos.setY(this->layout->getHeight() - 1);

    Block block;
    if (this->layout->getBlock(pos.x(), pos.y(), &block)) {
        this->movementPermissionsSelector->select(block.collision, block.elevation);
    }
}
