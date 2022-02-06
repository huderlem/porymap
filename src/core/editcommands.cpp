#include "editcommands.h"
#include "mappixmapitem.h"
#include "draggablepixmapitem.h"
#include "bordermetatilespixmapitem.h"
#include "editor.h"

#include <QDebug>

int getEventTypeMask(QList<Event *> events) {
    int eventTypeMask = 0;
    for (auto event : events) {
        QString groupType = event->get("event_group_type");
        if (groupType == EventGroup::Object) {
            eventTypeMask |= IDMask_EventType_Object;
        } else if (groupType == EventGroup::Warp) {
            eventTypeMask |= IDMask_EventType_Warp;
        } else if (groupType == EventGroup::Coord) {
            eventTypeMask |= IDMask_EventType_Trigger;
        } else if (groupType == EventGroup::Bg) {
            eventTypeMask |= IDMask_EventType_BG;
        } else if (groupType == EventGroup::Heal) {
            eventTypeMask |= IDMask_EventType_Heal;
        }
    }
    return eventTypeMask;
}

void renderMapBlocks(Map *map, bool ignoreCache = false) {
    map->mapItem->draw(ignoreCache);
    map->collisionItem->draw(ignoreCache);
}

PaintMetatile::PaintMetatile(Map *map,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Metatiles");

    this->map = map;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->actionId = actionId;
}

void PaintMetatile::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setBlockdata(newMetatiles);

    map->layout->lastCommitMapBlocks.blocks = map->layout->blockdata;

    renderMapBlocks(map);
}

void PaintMetatile::undo() {
    if (!map) return;

    map->setBlockdata(oldMetatiles);

    map->layout->lastCommitMapBlocks.blocks = map->layout->blockdata;

    renderMapBlocks(map);

    QUndoCommand::undo();
}

bool PaintMetatile::mergeWith(const QUndoCommand *command) {
    const PaintMetatile *other = static_cast<const PaintMetatile *>(command);

    if (map != other->map)
        return false;

    if (actionId != other->actionId)
        return false;

    newMetatiles = other->newMetatiles;

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

PaintBorder::PaintBorder(Map *map,
    const Blockdata &oldBorder, const Blockdata &newBorder,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Border");

    this->map = map;
    this->oldBorder = oldBorder;
    this->newBorder = newBorder;

    this->actionId = actionId;
}

void PaintBorder::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->layout->border = newBorder;

    map->borderItem->draw();
}

void PaintBorder::undo() {
    if (!map) return;

    map->layout->border = oldBorder;

    map->borderItem->draw();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ShiftMetatiles::ShiftMetatiles(Map *map,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Shift Metatiles");

    this->map = map;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->actionId = actionId;
}

void ShiftMetatiles::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setBlockdata(newMetatiles);

    map->layout->lastCommitMapBlocks.blocks = map->layout->blockdata;

    renderMapBlocks(map, true);
}

void ShiftMetatiles::undo() {
    if (!map) return;

    map->setBlockdata(oldMetatiles);

    map->layout->lastCommitMapBlocks.blocks = map->layout->blockdata;

    renderMapBlocks(map, true);

    QUndoCommand::undo();
}

bool ShiftMetatiles::mergeWith(const QUndoCommand *command) {
    const ShiftMetatiles *other = static_cast<const ShiftMetatiles *>(command);

    if (this->map != other->map)
        return false;

    if (actionId != other->actionId)
        return false;

    this->newMetatiles = other->newMetatiles;

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ResizeMap::ResizeMap(Map *map, QSize oldMapDimensions, QSize newMapDimensions,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    QSize oldBorderDimensions, QSize newBorderDimensions,
    const Blockdata &oldBorder, const Blockdata &newBorder,
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

void ResizeMap::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->layout->blockdata = newMetatiles;
    map->setDimensions(newMapWidth, newMapHeight, false);

    map->layout->border = newBorder;
    map->setBorderDimensions(newBorderWidth, newBorderHeight, false);

    map->layout->lastCommitMapBlocks.dimensions = QSize(map->getWidth(), map->getHeight());

    map->mapNeedsRedrawing();
}

void ResizeMap::undo() {
    if (!map) return;

    map->layout->blockdata = oldMetatiles;
    map->setDimensions(oldMapWidth, oldMapHeight, false);

    map->layout->border = oldBorder;
    map->setBorderDimensions(oldBorderWidth, oldBorderHeight, false);

    map->layout->lastCommitMapBlocks.dimensions = QSize(map->getWidth(), map->getHeight());

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

    // TODO: check that same events as well?

    this->deltaX += other->deltaX;
    this->deltaY += other->deltaY;

    return true;
}

int EventMove::id() const {
    return CommandId::ID_EventMove | getEventTypeMask(events);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventShift::EventShift(QList<Event *> events,
    int deltaX, int deltaY, unsigned actionId,
    QUndoCommand *parent) 
  : EventMove(events, deltaX, deltaY, actionId, parent) {
    this->events = events;
    setText("Shift Events");
}

int EventShift::id() const {
    return CommandId::ID_EventShift | getEventTypeMask(events);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventCreate::EventCreate(Editor *editor, Map *map, Event *event,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Create Event");

    this->editor = editor;

    this->map = map;
    this->event = event;
}

void EventCreate::redo() {
    QUndoCommand::redo();

    map->addEvent(event);

    editor->project->setEventPixmap(event);
    editor->addMapEvent(event);

    // select this event
    editor->selected_events->clear();
    editor->selectMapEvent(event->pixmapItem, false);
}

void EventCreate::undo() {
    map->removeEvent(event);

    if (editor->scene->items().contains(event->pixmapItem)) {
        editor->scene->removeItem(event->pixmapItem);
    }
    editor->selected_events->removeOne(event->pixmapItem);

    editor->shouldReselectEvents();

    QUndoCommand::undo();
}

int EventCreate::id() const {
    return CommandId::ID_EventCreate | getEventTypeMask(QList<Event*>({this->event}));
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventDelete::EventDelete(Editor *editor, Map *map,
    QList<Event *> selectedEvents, Event *nextSelectedEvent,
    QUndoCommand *parent) : QUndoCommand(parent) {
    if (selectedEvents.size() > 1) {
        setText("Delete Events");
    } else {
        setText("Delete Event");
    }

    this->editor = editor;
    this->map = map;

    this->selectedEvents = selectedEvents;
    this->nextSelectedEvent = nextSelectedEvent;
}

void EventDelete::redo() {
    QUndoCommand::redo();

    for (Event *event : selectedEvents) {
        map->removeEvent(event);

        if (editor->scene->items().contains(event->pixmapItem)) {
            editor->scene->removeItem(event->pixmapItem);
        }
        editor->selected_events->removeOne(event->pixmapItem);
    }

    editor->selected_events->clear();
    if (nextSelectedEvent)
        editor->selected_events->append(nextSelectedEvent->pixmapItem);
    editor->shouldReselectEvents();
}

void EventDelete::undo() {
    for (Event *event : selectedEvents) {
        map->addEvent(event);
        editor->project->setEventPixmap(event);
        editor->addMapEvent(event);
    }

    // select these events
    editor->selected_events->clear();
    for (Event *event : selectedEvents) {
        editor->selected_events->append(event->pixmapItem);
    }
    editor->shouldReselectEvents();

    QUndoCommand::undo();
}

int EventDelete::id() const {
    return CommandId::ID_EventDelete | getEventTypeMask(this->selectedEvents);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventDuplicate::EventDuplicate(Editor *editor, Map *map,
    QList<Event *> selectedEvents,
    QUndoCommand *parent) : QUndoCommand(parent) {
    if (selectedEvents.size() > 1) {
        setText("Duplicate Events");
    } else {
        setText("Duplicate Event");
    }

    this->editor = editor;

    this->map = map;
    this->selectedEvents = selectedEvents;
}

void EventDuplicate::redo() {
    QUndoCommand::redo();

    for (Event *event : selectedEvents) {
        map->addEvent(event);
        editor->project->setEventPixmap(event);
        editor->addMapEvent(event);
    }

    // select these events
    editor->selected_events->clear();
    for (Event *event : selectedEvents) {
        editor->selected_events->append(event->pixmapItem);
    }
    editor->shouldReselectEvents();
}

void EventDuplicate::undo() {
    for (Event *event : selectedEvents) {
        map->removeEvent(event);

        if (editor->scene->items().contains(event->pixmapItem)) {
            editor->scene->removeItem(event->pixmapItem);
        }
        editor->selected_events->removeOne(event->pixmapItem);
    }

    editor->shouldReselectEvents();

    QUndoCommand::undo();
}

int EventDuplicate::id() const {
    return CommandId::ID_EventDuplicate | getEventTypeMask(this->selectedEvents);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

EventPaste::EventPaste(Editor *editor, Map *map,
    QList<Event *> pastedEvents,
    QUndoCommand *parent) : EventDuplicate(editor, map, pastedEvents) {
    if (pastedEvents.size() > 1) {
        setText("Paste Events");
    } else {
        setText("Paste Event");
    }
}

int EventPaste::id() const {
    return CommandId::ID_EventPaste | getEventTypeMask(this->selectedEvents);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ScriptEditMap::ScriptEditMap(Map *map,
        QSize oldMapDimensions, QSize newMapDimensions,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Script Edit Map");

    this->map = map;

    this->newMetatiles = newMetatiles;
    this->oldMetatiles = oldMetatiles;

    this->oldMapWidth = oldMapDimensions.width();
    this->oldMapHeight = oldMapDimensions.height();
    this->newMapWidth = newMapDimensions.width();
    this->newMapHeight = newMapDimensions.height();
}

void ScriptEditMap::redo() {
    QUndoCommand::redo();

    if (!map) return;

    if (newMapWidth != map->getWidth() || newMapHeight != map->getHeight()) {
        map->layout->blockdata = newMetatiles;
        map->setDimensions(newMapWidth, newMapHeight, false);
    } else {
        map->setBlockdata(newMetatiles);
    }

    map->layout->lastCommitMapBlocks.blocks = newMetatiles;
    map->layout->lastCommitMapBlocks.dimensions = QSize(newMapWidth, newMapHeight);

    renderMapBlocks(map);
}

void ScriptEditMap::undo() {
    if (!map) return;

    if (oldMapWidth != map->getWidth() || oldMapHeight != map->getHeight()) {
        map->layout->blockdata = oldMetatiles;
        map->setDimensions(oldMapWidth, oldMapHeight, false);
    } else {
        map->setBlockdata(oldMetatiles);
    }

    map->layout->lastCommitMapBlocks.blocks = oldMetatiles;
    map->layout->lastCommitMapBlocks.dimensions = QSize(oldMapWidth, oldMapHeight);

    renderMapBlocks(map);

    QUndoCommand::undo();
}
