#include "eventpixmapitem.h"
#include "project.h"
#include "editcommands.h"
#include "mapruler.h"
#include "metatile.h"

EventPixmapItem::EventPixmapItem(Event *event)
  : QGraphicsPixmapItem(event->getPixmap()),
    m_basePixmap(pixmap()),
    m_event(event)
{
    m_event->setPixmapItem(this);
    updatePixelPosition();
}

void EventPixmapItem::render(Project *project) {
    if (!m_event)
        return;

    m_basePixmap = m_event->loadPixmap(project);

    // If the base pixmap changes, the event's pixel position may change.
    updatePixelPosition();

    QPixmap pixmap = m_basePixmap;
    if (m_selected) {
        // Draw the selection rectangle
        QPainter painter(&pixmap);
        painter.setPen(Qt::magenta);
        painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    }
    setPixmap(pixmap);
    emit rendered(m_basePixmap);
}

void EventPixmapItem::move(int dx, int dy) {
    moveTo(m_event->getX() + dx,
           m_event->getY() + dy);
}

void EventPixmapItem::moveTo(const QPoint &pos) {
    moveTo(pos.x(), pos.y());
}

void EventPixmapItem::moveTo(int x, int y) {
    bool changed = false;
    if (m_event->getX() != x) {
        m_event->setX(x);
        emit xChanged(x);
        changed = true;
    }
    if (m_event->getY() != y) {
        m_event->setY(y);
        emit yChanged(y);
        changed = true;
    }
    if (changed) {
        updatePixelPosition();
        emit posChanged(x, y);
    }
}

void EventPixmapItem::updatePixelPosition() {
    setPos(m_event->getPixelX(), m_event->getPixelY());
}

void EventPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (m_active)
        return;
    m_active = true;
    m_lastPos = Metatile::coordFromPixmapCoord(mouseEvent->scenePos());

    bool selectionToggle = mouseEvent->modifiers() & Qt::ControlModifier;
    if (selectionToggle || !m_selected) {
        // User is either toggling this selection on/off as part of a group selection,
        // or they're newly selecting just this item.
        m_selected = (selectionToggle) ? !m_selected : true;
        emit selected(m_event, selectionToggle);
    } else {
        // This item is already selected and the user isn't toggling the selection, so there are 4 possibilities:
        // 1. This is the only selected event, and the selection is pointless.
        // 2. This is the only selected event, and they want to drag the item around.
        // 3. There's a group selection, and they want to start a new selection with just this item.
        // 4. There's a group selection, and they want to drag the group around.
        // 'selectMapEvent' will immediately clear the rest of the selection, which supports #1-3 but prevents #4.
        // To support #4 we set the flag below, and we only call 'selectMapEvent' on mouse release if no move occurred.
        m_releaseSelectionQueued = true;
    }
    mouseEvent->accept();
}

void EventPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (!m_active || !m_selected)
        return;

    QPoint pos = Metatile::coordFromPixmapCoord(mouseEvent->scenePos());
    if (pos == m_lastPos)
        return;

    m_releaseSelectionQueued = false;
    emit dragged(m_event, m_lastPos, pos);
    m_lastPos = pos;
}

void EventPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    if (!m_active)
        return;
    m_active = false;
    if (m_releaseSelectionQueued) {
        m_releaseSelectionQueued = false;
        if (Metatile::coordFromPixmapCoord(mouseEvent->scenePos()) == m_lastPos)
            emit selected(m_event, false);
    }
    emit released(m_event, m_lastPos);
}
