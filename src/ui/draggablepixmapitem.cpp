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
    QList<Event*> objects;
    objects.append(event);
    event->pixmap = QPixmap();
    editor->project->loadEventPixmaps(objects);
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
    if (!this->editor->map_ruler->isAnchored() && this->editor->map_ruler->isMousePressed(mouse)) {
        this->editor->map_ruler->setAnchor(mouse->scenePos(), mouse->screenPos());
    } else if (this->editor->map_ruler->isAnchored()) {
        if (mouse->buttons() & Qt::LeftButton)
            this->editor->map_ruler->locked = !this->editor->map_ruler->locked;
        if (this->editor->map_ruler->isMousePressed(mouse))
            this->editor->map_ruler->endAnchor();
    }
}

void DraggablePixmapItem::move(int x, int y) {
    event->setX(event->x() + x);
    event->setY(event->y() + y);
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        QPoint pos = Metatile::coordFromPixmapCoord(mouse->scenePos());
        emit this->editor->map_item->hoveredMapMetatileChanged(mouse->scenePos(), mouse->screenPos());
        if (this->editor->map_ruler->isAnchored()) {
            this->editor->map_ruler->setEndPos(mouse->scenePos(), mouse->screenPos());
        } else if (pos.x() != last_x || pos.y() != last_y) {
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
    if (this->event->get("event_type") == EventType::Warp) {
        QString destMap = this->event->get("destination_map_name");
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(this->event->get("destination_map_name"), this->event->get("destination_warp"));
        }
    }
    else if (this->event->get("event_type") == EventType::SecretBase) {
        QString baseId = this->event->get("secret_base_id");
        QString destMap = editor->project->mapConstantsToMapNames->value("MAP_" + baseId.left(baseId.lastIndexOf("_")));
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(destMap, "0");
        }
    }
}
