#include "editcommands.h"
#include "mappixmapitem.h"
#include "draggablepixmapitem.h"
#include "bordermetatilespixmapitem.h"
#include "editor.h"

#include <QDebug>



PaintMetatile::PaintMetatile(Map *map,
    Blockdata *oldMetatiles, Blockdata *newMetatiles,
    unsigned eventId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Metatiles");

    this->map = map;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->eventId = eventId;
}

PaintMetatile::~PaintMetatile() {
    if (newMetatiles) delete newMetatiles;
    if (oldMetatiles) delete oldMetatiles;
}

void PaintMetatile::redo() {
    QUndoCommand::redo();

    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(newMetatiles);
    }

    map->mapItem->draw();
}

void PaintMetatile::undo() {
    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(oldMetatiles);
    }

    map->mapItem->draw();

    QUndoCommand::undo();
}

bool PaintMetatile::mergeWith(const QUndoCommand *command) {
    // does an up merge
    const PaintMetatile *other = static_cast<const PaintMetatile *>(command);

    if (this->map != other->map)
        return false;

    if (eventId != other->eventId)
        return false;

    this->newMetatiles->copyFrom(other->newMetatiles);

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

PaintBorder::PaintBorder(Map *map,
    Blockdata *oldBorder, Blockdata *newBorder,
    unsigned eventId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Border");

    this->map = map;
    this->oldBorder = oldBorder;
    this->newBorder = newBorder;

    this->eventId = eventId;
}

PaintBorder::~PaintBorder() {
    if (newBorder) delete newBorder;
    if (oldBorder) delete oldBorder;
}

void PaintBorder::redo() {
    QUndoCommand::redo();

    if (!map) return;

    if (map->layout->border) {
        map->layout->border->copyFrom(newBorder);
    }

    map->borderItem->draw();
}

void PaintBorder::undo() {
    if (!map) return;

    if (map->layout->border) {
        map->layout->border->copyFrom(oldBorder);
    }

    map->borderItem->draw();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

BucketFillMetatile::BucketFillMetatile(Map *map,
    Blockdata *oldMetatiles, Blockdata *newMetatiles,
    unsigned eventId, QUndoCommand *parent)
        : PaintMetatile(map, oldMetatiles, newMetatiles, eventId, parent) {
    setText("Bucket Fill Metatiles");
}

BucketFillMetatile::~BucketFillMetatile() {
    PaintMetatile::~PaintMetatile();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MagicFillMetatile::MagicFillMetatile(Map *map,
    Blockdata *oldMetatiles, Blockdata *newMetatiles,
    unsigned eventId, QUndoCommand *parent)
        : PaintMetatile(map, oldMetatiles, newMetatiles, eventId, parent) {
    setText("Magic Fill Metatiles");
}

MagicFillMetatile::~MagicFillMetatile() {
    PaintMetatile::~PaintMetatile();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ShiftMetatiles::ShiftMetatiles(Map *map,
    Blockdata *oldMetatiles, Blockdata *newMetatiles,
    unsigned eventId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Shift Metatiles");

    this->map = map;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->eventId = eventId;
}

ShiftMetatiles::~ShiftMetatiles() {
    if (newMetatiles) delete newMetatiles;
    if (oldMetatiles) delete oldMetatiles;
}

void ShiftMetatiles::redo() {
    QUndoCommand::redo();

    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(newMetatiles);
    }

    map->mapItem->draw(true);
}

void ShiftMetatiles::undo() {
    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(oldMetatiles);
    }

    map->mapItem->draw(true);

    QUndoCommand::undo();
}

bool ShiftMetatiles::mergeWith(const QUndoCommand *command) {
    const ShiftMetatiles *other = static_cast<const ShiftMetatiles *>(command);

    if (this->map != other->map)
        return false;

    if (eventId != other->eventId)
        return false;

    this->newMetatiles->copyFrom(other->newMetatiles);

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ResizeMap::ResizeMap(Map *map, QSize oldMapDimensions, QSize newMapDimensions,
    Blockdata *oldMetatiles, Blockdata *newMetatiles,
    QSize oldBorderDimensions, QSize newBorderDimensions,
    Blockdata *oldBorder, Blockdata *newBorder,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Resize Map");

    this->map = map;

    this->oldMapWidth = oldMapDimensions.width();
    this->oldMapHeight = oldMapDimensions.height();

    this->newMapWidth = newMapDimensions.width();
    this->newMapHeight = newMapDimensions.height();

    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->oldBorderWidth = oldBorderDimensions.width();
    this->oldBorderHeight = oldBorderDimensions.height();

    this->newBorderWidth = newBorderDimensions.width();
    this->newBorderHeight = newBorderDimensions.height();

    this->oldBorder = oldBorder;
    this->newBorder = newBorder;
}

ResizeMap::~ResizeMap() {
    if (newMetatiles) delete newMetatiles;
    if (oldMetatiles) delete oldMetatiles;
}

void ResizeMap::redo() {
    QUndoCommand::redo();

    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(newMetatiles);
        map->setDimensions(newMapWidth, newMapHeight, false);
    }

    if (map->layout->border) {
        map->layout->border->copyFrom(newBorder);
        map->setBorderDimensions(newBorderWidth, newBorderHeight, false);
    }

    map->mapNeedsRedrawing();
}

void ResizeMap::undo() {
    if (!map) return;

    if (map->layout->blockdata) {
        map->layout->blockdata->copyFrom(oldMetatiles);
        map->setDimensions(oldMapWidth, oldMapHeight, false);
    }

    if (map->layout->border) {
        map->layout->border->copyFrom(oldBorder);
        map->setBorderDimensions(oldBorderWidth, oldBorderHeight, false);
    }

    map->mapNeedsRedrawing();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventMove::EventMove(QList<Event *> events,
    int deltaX, int deltaY, unsigned actionId,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Move Event");

    this->events = events;

    this->deltaX = deltaX;
    this->deltaY = deltaY;

    this->actionId = actionId;
}

EventMove::~EventMove() {}

void EventMove::redo() {
    QUndoCommand::redo();

    for (Event *event : events) {
        event->pixmapItem->move(deltaX, deltaY);
    }
}

void EventMove::undo() {
    for (Event *event : events) {
        event->pixmapItem->move(-deltaX, -deltaY);
    }

    QUndoCommand::undo();
}

bool EventMove::mergeWith(const QUndoCommand *command) {
    const EventMove *other = static_cast<const EventMove *>(command);

    if (actionId != other->actionId)
        return false;

    this->deltaX += other->deltaX;
    this->deltaY += other->deltaY;

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventShift::EventShift(QList<Event *> events,
    int deltaX, int deltaY, unsigned actionId,
    QUndoCommand *parent) 
  : EventMove(events, deltaX, deltaY, actionId, parent) {
    setText("Shift Events");
}

EventShift::~EventShift() {}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventCreate::EventCreate(Editor *editor, Map *map, Event *event,
    QUndoCommand *parent) : QUndoCommand(parent) {
    //
    setText("Create Event");

    this->editor = editor;

    this->map = map;
    this->event = event;
}

EventCreate::~EventCreate() {}

void EventCreate::redo() {
    QUndoCommand::redo();

    map->addEvent(event);
    
    editor->project->loadEventPixmaps(map->getAllEvents());
    editor->addMapEvent(event);

    map->objectsChanged();
}

void EventCreate::undo() {
    //
    map->removeEvent(event);

    if (editor->scene->items().contains(event->pixmapItem)) {
        editor->scene->removeItem(event->pixmapItem);
    }
    editor->selected_events->removeOne(event->pixmapItem);

    map->objectsChanged();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventDelete::EventDelete(Editor *editor, Map *map, Event *event,
    QUndoCommand *parent) : QUndoCommand(parent) {
    //
    setText("Delete Event");

    this->editor = editor;

    this->map = map;
    this->event = event;
}

EventDelete::~EventDelete() {}

void EventDelete::redo() {
    QUndoCommand::redo();

    map->removeEvent(event);

    if (editor->scene->items().contains(event->pixmapItem)) {
        editor->scene->removeItem(event->pixmapItem);
    }
    editor->selected_events->removeOne(event->pixmapItem);

    map->objectsChanged();
}

void EventDelete::undo() {
    //
    map->addEvent(event);
    
    editor->project->loadEventPixmaps(map->getAllEvents());
    editor->addMapEvent(event);

    map->objectsChanged();

    QUndoCommand::undo();
}
