#include "editcommands.h"
#include "draggablepixmapitem.h"
#include "bordermetatilespixmapitem.h"
#include "editor.h"

#include <QDebug>

int getEventTypeMask(QList<Event *> events) {
    int eventTypeMask = 0;
    for (auto event : events) {
        Event::Group groupType = event->getEventGroup();
        if (groupType == Event::Group::Object) {
            eventTypeMask |= IDMask_EventType_Object;
        } else if (groupType == Event::Group::Warp) {
            eventTypeMask |= IDMask_EventType_Warp;
        } else if (groupType == Event::Group::Coord) {
            eventTypeMask |= IDMask_EventType_Trigger;
        } else if (groupType == Event::Group::Bg) {
            eventTypeMask |= IDMask_EventType_BG;
        } else if (groupType == Event::Group::Heal) {
            eventTypeMask |= IDMask_EventType_Heal;
        }
    }
    return eventTypeMask;
}

/// !TODO:
void renderBlocks(Layout *layout, bool ignoreCache = false) {
    layout->layoutItem->draw(ignoreCache);
    layout->collisionItem->draw(ignoreCache);
}

PaintMetatile::PaintMetatile(Layout *layout,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Metatiles");

    this->layout = layout;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->actionId = actionId;
}

void PaintMetatile::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    layout->setBlockdata(newMetatiles, true);

    layout->lastCommitBlocks.blocks = layout->blockdata;

    renderBlocks(layout);
}

void PaintMetatile::undo() {
    if (!layout) return;

    layout->setBlockdata(oldMetatiles, true);

    layout->lastCommitBlocks.blocks = layout->blockdata;

    renderBlocks(layout);

    QUndoCommand::undo();
}

bool PaintMetatile::mergeWith(const QUndoCommand *command) {
    const PaintMetatile *other = static_cast<const PaintMetatile *>(command);

    if (layout != other->layout)
        return false;

    if (actionId != other->actionId)
        return false;

    newMetatiles = other->newMetatiles;

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

PaintBorder::PaintBorder(Layout *layout,
    const Blockdata &oldBorder, const Blockdata &newBorder,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Paint Border");

    this->layout = layout;
    this->oldBorder = oldBorder;
    this->newBorder = newBorder;

    this->actionId = actionId;
}

void PaintBorder::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    layout->setBorderBlockData(newBorder, true);

    layout->lastCommitBlocks.border = layout->border;

    layout->borderItem->draw();
}

void PaintBorder::undo() {
    if (!layout) return;

    layout->setBorderBlockData(oldBorder, true);

    layout->lastCommitBlocks.border = layout->border;

    layout->borderItem->draw();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ShiftMetatiles::ShiftMetatiles(Layout *layout,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    unsigned actionId, QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Shift Metatiles");

    this->layout = layout;
    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->actionId = actionId;
}

void ShiftMetatiles::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    layout->setBlockdata(newMetatiles, true);

    layout->lastCommitBlocks.blocks = layout->blockdata;

    renderBlocks(layout, true);
}

void ShiftMetatiles::undo() {
    if (!layout) return;

    layout->setBlockdata(oldMetatiles, true);

    layout->lastCommitBlocks.blocks = layout->blockdata;

    renderBlocks(layout, true);

    QUndoCommand::undo();
}

bool ShiftMetatiles::mergeWith(const QUndoCommand *command) {
    const ShiftMetatiles *other = static_cast<const ShiftMetatiles *>(command);

    if (this->layout != other->layout)
        return false;

    if (actionId != other->actionId)
        return false;

    this->newMetatiles = other->newMetatiles;

    return true;
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ResizeMap::ResizeMap(Layout *layout, QSize oldMapDimensions, QSize newMapDimensions,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    QSize oldBorderDimensions, QSize newBorderDimensions,
    const Blockdata &oldBorder, const Blockdata &newBorder,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Resize Map");

    this->layout = layout;

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

    if (!layout) return;

    layout->blockdata = newMetatiles;
    layout->setDimensions(newMapWidth, newMapHeight, false, true);

    layout->border = newBorder;
    layout->setBorderDimensions(newBorderWidth, newBorderHeight, false, true);

    layout->lastCommitBlocks.mapDimensions = QSize(layout->getWidth(), layout->getHeight());
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

    layout->needsRedrawing();
}

void ResizeMap::undo() {
    if (!layout) return;

    layout->blockdata = oldMetatiles;
    layout->setDimensions(oldMapWidth, oldMapHeight, false, true);

    layout->border = oldBorder;
    layout->setBorderDimensions(oldBorderWidth, oldBorderHeight, false, true);

    layout->lastCommitBlocks.mapDimensions = QSize(layout->getWidth(), layout->getHeight());
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

    layout->needsRedrawing();

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
        event->getPixmapItem()->move(deltaX, deltaY);
    }
}

void EventMove::undo() {
    for (Event *event : events) {
        event->getPixmapItem()->move(-deltaX, -deltaY);
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
    editor->selectMapEvent(event->getPixmapItem(), false);
}

void EventCreate::undo() {
    map->removeEvent(event);

    if (editor->scene->items().contains(event->getPixmapItem())) {
        editor->scene->removeItem(event->getPixmapItem());
    }
    editor->selected_events->removeOne(event->getPixmapItem());

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

        if (editor->scene->items().contains(event->getPixmapItem())) {
            editor->scene->removeItem(event->getPixmapItem());
        }
        editor->selected_events->removeOne(event->getPixmapItem());
    }

    editor->selected_events->clear();
    if (nextSelectedEvent)
        editor->selected_events->append(nextSelectedEvent->getPixmapItem());
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
        editor->selected_events->append(event->getPixmapItem());
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
        editor->selected_events->append(event->getPixmapItem());
    }
    editor->shouldReselectEvents();
}

void EventDuplicate::undo() {
    for (Event *event : selectedEvents) {
        map->removeEvent(event);

        if (editor->scene->items().contains(event->getPixmapItem())) {
            editor->scene->removeItem(event->getPixmapItem());
        }
        editor->selected_events->removeOne(event->getPixmapItem());
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
    QUndoCommand *parent) : EventDuplicate(editor, map, pastedEvents, parent) {
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

ScriptEditMap::ScriptEditMap(Layout *layout,
        QSize oldMapDimensions, QSize newMapDimensions,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        QSize oldBorderDimensions, QSize newBorderDimensions,
        const Blockdata &oldBorder, const Blockdata &newBorder,
        QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Script Edit Map");

    this->layout = layout;

    this->newMetatiles = newMetatiles;
    this->oldMetatiles = oldMetatiles;

    this->oldMapWidth = oldMapDimensions.width();
    this->oldMapHeight = oldMapDimensions.height();
    this->newMapWidth = newMapDimensions.width();
    this->newMapHeight = newMapDimensions.height();

    this->oldBorder = oldBorder;
    this->newBorder = newBorder;

    this->oldBorderWidth = oldBorderDimensions.width();
    this->oldBorderHeight = oldBorderDimensions.height();
    this->newBorderWidth = newBorderDimensions.width();
    this->newBorderHeight = newBorderDimensions.height();
}

void ScriptEditMap::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    if (newMapWidth != layout->getWidth() || newMapHeight != layout->getHeight()) {
        layout->blockdata = newMetatiles;
        layout->setDimensions(newMapWidth, newMapHeight, false);
    } else {
        layout->setBlockdata(newMetatiles);
    }

    if (newBorderWidth != layout->getBorderWidth() || newBorderHeight != layout->getBorderHeight()) {
        layout->border = newBorder;
        layout->setBorderDimensions(newBorderWidth, newBorderHeight, false);
    } else {
        layout->setBorderBlockData(newBorder);
    }

    layout->lastCommitBlocks.blocks = newMetatiles;
    layout->lastCommitBlocks.mapDimensions = QSize(newMapWidth, newMapHeight);
    layout->lastCommitBlocks.border = newBorder;
    layout->lastCommitBlocks.borderDimensions = QSize(newBorderWidth, newBorderHeight);

    // !TODO
    renderBlocks(layout);
    layout->borderItem->draw();
}

void ScriptEditMap::undo() {
    if (!layout) return;

    if (oldMapWidth != layout->getWidth() || oldMapHeight != layout->getHeight()) {
        layout->blockdata = oldMetatiles;
        layout->setDimensions(oldMapWidth, oldMapHeight, false);
    } else {
        layout->setBlockdata(oldMetatiles);
    }

    if (oldBorderWidth != layout->getBorderWidth() || oldBorderHeight != layout->getBorderHeight()) {
        layout->border = oldBorder;
        layout->setBorderDimensions(oldBorderWidth, oldBorderHeight, false);
    } else {
        layout->setBorderBlockData(oldBorder);
    }

    layout->lastCommitBlocks.blocks = oldMetatiles;
    layout->lastCommitBlocks.mapDimensions = QSize(oldMapWidth, oldMapHeight);
    layout->lastCommitBlocks.border = oldBorder;
    layout->lastCommitBlocks.borderDimensions = QSize(oldBorderWidth, oldBorderHeight);

    // !TODO
    renderBlocks(layout);
    layout->borderItem->draw();

    QUndoCommand::undo();
}
