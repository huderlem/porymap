#include "draggablepixmapitem.h"
#include "editor.h"
#include "editcommands.h"
#include "mapruler.h"
#include "metatile.h"

static unsigned currentActionId = 0;


void DraggablePixmapItem::updatePosition() {
    int x = event->getPixelX();
    int y = event->getPixelY();
    setX(x);
    setY(y);
    if (editor->selected_events && editor->selected_events->contains(this)) {
        setZValue(event->getY() + 1);
    } else {
        setZValue(event->getY());
    }
    editor->updateWarpEventWarning(event);
}

void DraggablePixmapItem::emitPositionChanged() {
    emit xChanged(event->getX());
    emit yChanged(event->getY());
    emit elevationChanged(event->getElevation());
}

void DraggablePixmapItem::updatePixmap() {
    editor->project->setEventPixmap(event, true);
    this->updatePosition();
    editor->redrawObject(this);
    emit spriteChanged(event->getPixmap());
}

void DraggablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouse) {
    active = true;
    QPoint pos = Metatile::coordFromPixmapCoord(mouse->scenePos());
    last_x = pos.x();
    last_y = pos.y();
    this->editor->selectMapEvent(this, mouse->modifiers() & Qt::ControlModifier);
    this->editor->selectingEvent = true;
}

void DraggablePixmapItem::move(int dx, int dy) {
    event->setX(event->getX() + dx);
    event->setY(event->getY() + dy);
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::moveTo(const QPoint &pos) {
    event->setX(pos.x());
    event->setY(pos.y());
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        QPoint pos = Metatile::coordFromPixmapCoord(mouse->scenePos());
        if (pos.x() != last_x || pos.y() != last_y) {
            emit this->editor->map_item->hoveredMapMetatileChanged(pos);
        	QList <Event *> selectedEvents;
            if (editor->selected_events->contains(this)) {
                for (DraggablePixmapItem *item : *editor->selected_events) {
                    selectedEvents.append(item->event);
                }
            } else {
                selectedEvents.append(this->event);
            }
            editor->map->editHistory.push(new EventMove(selectedEvents, pos.x() - last_x, pos.y() - last_y, currentActionId));
            last_x = pos.x();
            last_y = pos.y();
        }
    }
}

void DraggablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    active = false;
    currentActionId++;
}

void DraggablePixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    Event::Type eventType = this->event->getEventType();
    if (eventType == Event::Type::Warp) {
        WarpEvent *warp = dynamic_cast<WarpEvent *>(this->event);
        QString destMap = warp->getDestinationMap();
        int warpId = ParseUtil::gameStringToInt(warp->getDestinationWarpID());
        emit editor->warpEventDoubleClicked(destMap, warpId, Event::Group::Warp);
    }
    else if (eventType == Event::Type::CloneObject) {
        CloneObjectEvent *clone = dynamic_cast<CloneObjectEvent *>(this->event);
        emit editor->warpEventDoubleClicked(clone->getTargetMap(), clone->getTargetID(), Event::Group::Object);
    }
    else if (eventType == Event::Type::SecretBase) {
        const QString mapPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
        SecretBaseEvent *base = dynamic_cast<SecretBaseEvent *>(this->event);
        QString baseId = base->getBaseID();
        QString destMap = editor->project->mapConstantsToMapNames.value(mapPrefix + baseId.left(baseId.lastIndexOf("_")));
        emit editor->warpEventDoubleClicked(destMap, 0, Event::Group::Warp);
    }
}
