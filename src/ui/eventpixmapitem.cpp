#include "eventpixmapitem.h"
#include "editor.h"
#include "editcommands.h"
#include "mapruler.h"
#include "metatile.h"

void EventPixmapItem::move(int dx, int dy) {
    event->setX(event->getX() + dx);
    event->setY(event->getY() + dy);
    updatePosition();
    emitPositionChanged();
}

void EventPixmapItem::moveTo(const QPoint &pos) {
    event->setX(pos.x());
    event->setY(pos.y());
    updatePosition();
    emitPositionChanged();
}

void EventPixmapItem::updatePosition() {
    int x = this->event->getPixelX();
    int y = this->event->getPixelY();
    setX(x);
    setY(y);
    editor->updateWarpEventWarning(event);
}

void EventPixmapItem::emitPositionChanged() {
    emit xChanged(event->getX());
    emit yChanged(event->getY());
}

void EventPixmapItem::updatePixmap() {
    editor->redrawEventPixmapItem(this);
    emit spriteChanged(event->getPixmap());
}

void EventPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (this->active)
        return;
    this->active = true;
    this->lastPos = Metatile::coordFromPixmapCoord(mouseEvent->scenePos());

    bool selectionToggle = mouseEvent->modifiers() & Qt::ControlModifier;
    if (selectionToggle || !this->editor->selectedEvents.contains(this->event)) {
        // User is either toggling this selection on/off as part of a group selection,
        // or they're newly selecting just this item.
        emit selected(this->event, selectionToggle);
    } else {
        // This item is already selected and the user isn't toggling the selection, so there are 4 possibilities:
        // 1. This is the only selected event, and the selection is pointless.
        // 2. This is the only selected event, and they want to drag the item around.
        // 3. There's a group selection, and they want to start a new selection with just this item.
        // 4. There's a group selection, and they want to drag the group around.
        // 'selectMapEvent' will immediately clear the rest of the selection, which supports #1-3 but prevents #4.
        // To support #4 we set the flag below, and we only call 'selectMapEvent' on mouse release if no move occurred.
        this->releaseSelectionQueued = true;
    }
    mouseEvent->accept();
}

void EventPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (!this->active)
        return;

    QPoint pos = Metatile::coordFromPixmapCoord(mouseEvent->scenePos());
    if (pos == this->lastPos)
        return;

    this->releaseSelectionQueued = false;
    emit dragged(this->event, this->lastPos, pos);
    this->lastPos = pos;
}

void EventPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (!this->active)
        return;
    this->active = false;
    if (this->releaseSelectionQueued) {
        this->releaseSelectionQueued = false;
        if (Metatile::coordFromPixmapCoord(mouseEvent->scenePos()) == this->lastPos)
            emit selected(this->event, false);
    }
    emit released(this->event, this->lastPos);
}
