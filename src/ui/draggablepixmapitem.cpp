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
        setZValue(event->y() + 1);
    } else {
        setZValue(event->y());
    }
}

void DraggablePixmapItem::emitPositionChanged() {
    emit xChanged(event->x());
    emit yChanged(event->y());
    emit elevationChanged(event->elevation());
}

void DraggablePixmapItem::updatePixmap() {
    editor->project->setEventPixmap(event, true);
    this->updatePosition();
    editor->redrawObject(this);
    emit spriteChanged(event->pixmap);
}

void DraggablePixmapItem::bind(QComboBox *combo, QString key) {
    connect(combo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
            this, [this, key](QString value){
        this->event->put(key, value);
    });
    connect(this, &DraggablePixmapItem::onPropertyChanged,
            this, [combo, key](QString key2, QString value){
        if (key2 == key) {
            combo->addItem(value);
            combo->setCurrentText(value);
        }
    });
}

void DraggablePixmapItem::bindToUserData(QComboBox *combo, QString key) {
    connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this, combo, key](int index) {
        this->event->put(key, combo->itemData(index).toString());
    });
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
    event->setX(event->x() + dx);
    event->setY(event->y() + dy);
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
    QString eventType = this->event->get("event_type");
    if (eventType == EventType::Warp) {
        QString destMap = this->event->get("destination_map_name");
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(destMap, this->event->get("destination_warp"), EventGroup::Warp);
        }
    }
    else if (eventType == EventType::CloneObject) {
        QString destMap = this->event->get("target_map");
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(destMap, this->event->get("target_local_id"), EventGroup::Object);
        }
    }
    else if (eventType == EventType::SecretBase) {
        QString baseId = this->event->get("secret_base_id");
        QString destMap = editor->project->mapConstantsToMapNames.value("MAP_" + baseId.left(baseId.lastIndexOf("_")));
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(destMap, "0", EventGroup::Warp);
        }
    }
}
