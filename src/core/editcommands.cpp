#include "editcommands.h"
#include "draggablepixmapitem.h"
#include "bordermetatilespixmapitem.h"
#include "editor.h"

#include <QDebug>

int getEventTypeMask(const QList<Event *> &events) {
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

int getConnectionDirectionMask(const QList<QString> &directions) {
    int mask = 0;
    for (auto direction : directions) {
        if (direction == "up") {
            mask |= IDMask_ConnectionDirection_Up;
        } else if (direction == "down") {
            mask |= IDMask_ConnectionDirection_Down;
        } else if (direction == "left") {
            mask |= IDMask_ConnectionDirection_Left;
        } else if (direction == "right") {
            mask |= IDMask_ConnectionDirection_Right;
        } else if (direction == "dive") {
            mask |= IDMask_ConnectionDirection_Dive;
        } else if (direction == "emerge") {
            mask |= IDMask_ConnectionDirection_Emerge;
        }
    }
    return mask;
}

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

ResizeLayout::ResizeLayout(Layout *layout, QSize oldLayoutDimensions, QMargins newLayoutMargins,
    const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
    QSize oldBorderDimensions, QSize newBorderDimensions,
    const Blockdata &oldBorder, const Blockdata &newBorder,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Resize Map");

    this->layout = layout;

    this->oldLayoutWidth = oldLayoutDimensions.width();
    this->oldLayoutHeight = oldLayoutDimensions.height();

    this->newLayoutMargins = newLayoutMargins;

    this->oldMetatiles = oldMetatiles;
    this->newMetatiles = newMetatiles;

    this->oldBorderWidth = oldBorderDimensions.width();
    this->oldBorderHeight = oldBorderDimensions.height();

    this->newBorderWidth = newBorderDimensions.width();
    this->newBorderHeight = newBorderDimensions.height();

    this->oldBorder = oldBorder;
    this->newBorder = newBorder;
}

void ResizeLayout::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    layout->border = newBorder;
    layout->setBorderDimensions(newBorderWidth, newBorderHeight, false, true);

    layout->width = oldLayoutWidth;
    layout->height = oldLayoutHeight;
    layout->adjustDimensions(this->newLayoutMargins);
    layout->blockdata = newMetatiles;

    layout->lastCommitBlocks.layoutDimensions = QSize(layout->getWidth(), layout->getHeight());
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

    emit layout->needsRedrawing();
}

void ResizeLayout::undo() {
    if (!layout) return;

    layout->border = oldBorder;
    layout->setBorderDimensions(oldBorderWidth, oldBorderHeight, false, true);

    layout->width = oldLayoutWidth + newLayoutMargins.left() + newLayoutMargins.right();
    layout->height = oldLayoutHeight + newLayoutMargins.top() + newLayoutMargins.bottom();
    layout->adjustDimensions(-this->newLayoutMargins);
    layout->blockdata = oldMetatiles;

    layout->lastCommitBlocks.layoutDimensions = QSize(layout->getWidth(), layout->getHeight());
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

    emit layout->needsRedrawing();

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
    editor->addEventPixmapItem(event);
    editor->selectMapEvent(event);
}

void EventCreate::undo() {
    map->removeEvent(event);
    editor->removeEventPixmapItem(event);
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
        editor->removeEventPixmapItem(event);
    }

    editor->selectedEvents.clear();
    if (nextSelectedEvent)
        editor->selectedEvents.append(nextSelectedEvent);
    editor->shouldReselectEvents();
}

void EventDelete::undo() {
    for (Event *event : selectedEvents) {
        map->addEvent(event);
        editor->addEventPixmapItem(event);
    }

    editor->selectedEvents = selectedEvents;
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
        editor->addEventPixmapItem(event);
    }

    editor->selectedEvents = selectedEvents;
    editor->shouldReselectEvents();
}

void EventDuplicate::undo() {
    for (Event *event : selectedEvents) {
        map->removeEvent(event);
        editor->removeEventPixmapItem(event);
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

ScriptEditLayout::ScriptEditLayout(Layout *layout,
        QSize oldLayoutDimensions, QSize newLayoutDimensions,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        QSize oldBorderDimensions, QSize newBorderDimensions,
        const Blockdata &oldBorder, const Blockdata &newBorder,
        QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Script Edit Layout");

    this->layout = layout;

    this->newMetatiles = newMetatiles;
    this->oldMetatiles = oldMetatiles;

    this->oldLayoutWidth = oldLayoutDimensions.width();
    this->oldLayoutHeight = oldLayoutDimensions.height();
    this->newLayoutWidth = newLayoutDimensions.width();
    this->newLayoutHeight = newLayoutDimensions.height();

    this->oldBorder = oldBorder;
    this->newBorder = newBorder;

    this->oldBorderWidth = oldBorderDimensions.width();
    this->oldBorderHeight = oldBorderDimensions.height();
    this->newBorderWidth = newBorderDimensions.width();
    this->newBorderHeight = newBorderDimensions.height();
}

void ScriptEditLayout::redo() {
    QUndoCommand::redo();

    if (!layout) return;

    if (newLayoutWidth != layout->getWidth() || newLayoutHeight != layout->getHeight()) {
        layout->blockdata = newMetatiles;
        layout->setDimensions(newLayoutWidth, newLayoutHeight, false);
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
    layout->lastCommitBlocks.layoutDimensions = QSize(newLayoutWidth, newLayoutHeight);
    layout->lastCommitBlocks.border = newBorder;
    layout->lastCommitBlocks.borderDimensions = QSize(newBorderWidth, newBorderHeight);

    renderBlocks(layout, true);
    layout->borderItem->draw();
}

void ScriptEditLayout::undo() {
    if (!layout) return;

    if (oldLayoutWidth != layout->getWidth() || oldLayoutHeight != layout->getHeight()) {
        layout->blockdata = oldMetatiles;
        layout->setDimensions(oldLayoutWidth, oldLayoutHeight, false);
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
    layout->lastCommitBlocks.layoutDimensions = QSize(oldLayoutWidth, oldLayoutHeight);
    layout->lastCommitBlocks.border = oldBorder;
    layout->lastCommitBlocks.borderDimensions = QSize(oldBorderWidth, oldBorderHeight);

    renderBlocks(layout, true);
    layout->borderItem->draw();

    QUndoCommand::undo();
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MapConnectionMove::MapConnectionMove(MapConnection *connection, int newOffset, unsigned actionId,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Move Map Connection");

    this->connection = connection;
    this->oldOffset = connection->offset();
    this->newOffset = newOffset;

    this->mirrored = porymapConfig.mirrorConnectingMaps;

    this->actionId = actionId;
}

void MapConnectionMove::redo() {
    QUndoCommand::redo();
    if (this->connection)
        this->connection->setOffset(this->newOffset, this->mirrored);
}

void MapConnectionMove::undo() {
    if (this->connection)
        this->connection->setOffset(this->oldOffset, this->mirrored);
    QUndoCommand::undo();
}

bool MapConnectionMove::mergeWith(const QUndoCommand *command) {
    if (this->id() != command->id())
        return false;

    const MapConnectionMove *other = static_cast<const MapConnectionMove *>(command);
    if (this->connection != other->connection)
        return false;
    if (this->actionId != other->actionId)
        return false;

    this->newOffset = other->newOffset;
    return true;
}

int MapConnectionMove::id() const {
    return CommandId::ID_MapConnectionMove | getConnectionDirectionMask({this->connection->direction()});
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MapConnectionChangeDirection::MapConnectionChangeDirection(MapConnection *connection, QString newDirection,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Change Map Connection Direction");

    this->connection = connection;

    this->oldDirection = connection->direction();
    this->newDirection = newDirection;

    this->oldOffset = connection->offset();

    // If the direction changes between vertical/horizontal then the old offset may not make sense, so we reset it.
    if (MapConnection::isHorizontal(this->oldDirection) != MapConnection::isHorizontal(this->newDirection)
     || MapConnection::isVertical(this->oldDirection) != MapConnection::isVertical(this->newDirection)) {
        this->newOffset = 0;
    } else {
        this->newOffset = oldOffset;
    }

    this->mirrored = porymapConfig.mirrorConnectingMaps;
}

void MapConnectionChangeDirection::redo() {
    QUndoCommand::redo();
    if (this->connection) {
        this->connection->setDirection(this->newDirection, this->mirrored);
        this->connection->setOffset(this->newOffset, this->mirrored);
    }
}

void MapConnectionChangeDirection::undo() {
    if (this->connection) {
        this->connection->setDirection(this->oldDirection, this->mirrored);
        this->connection->setOffset(this->oldOffset, this->mirrored);
    }
    QUndoCommand::undo();
}

int MapConnectionChangeDirection::id() const {
    return CommandId::ID_MapConnectionChangeDirection | getConnectionDirectionMask({this->oldDirection, this->newDirection});
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MapConnectionChangeMap::MapConnectionChangeMap(MapConnection *connection, QString newMapName,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Change Map Connection Map");

    this->connection = connection;

    this->oldMapName = connection->targetMapName();
    this->newMapName = newMapName;

    this->oldOffset = connection->offset();
    this->newOffset = 0; // The old offset may not make sense, so we reset it

    this->mirrored = porymapConfig.mirrorConnectingMaps;
}

void MapConnectionChangeMap::redo() {
    QUndoCommand::redo();
    if (this->connection) {
        this->connection->setTargetMapName(this->newMapName, this->mirrored);
        this->connection->setOffset(this->newOffset, this->mirrored);
    }
}

void MapConnectionChangeMap::undo() {
    if (this->connection) {
        this->connection->setTargetMapName(this->oldMapName, this->mirrored);
        this->connection->setOffset(this->oldOffset, this->mirrored);
    }
    QUndoCommand::undo();
}

int MapConnectionChangeMap::id() const {
    return CommandId::ID_MapConnectionChangeMap | getConnectionDirectionMask({this->connection->direction()});
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MapConnectionAdd::MapConnectionAdd(Map *map, MapConnection *connection,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Add Map Connection");

    this->map = map;
    this->connection = connection;

    // Set this now because it's needed to create a mirror below.
    // It would otherwise be set by Map::addConnection.
    this->connection->setParentMap(this->map, false);

    if (porymapConfig.mirrorConnectingMaps) {
        this->mirror = this->connection->createMirror();
        this->mirrorMap = this->connection->targetMap();
    }
}

void MapConnectionAdd::redo() {
    QUndoCommand::redo();

    this->map->addConnection(this->connection);
    if (this->mirrorMap)
        this->mirrorMap->addConnection(this->mirror);
}

void MapConnectionAdd::undo() {
    if (this->mirrorMap) {
        // We can't guarantee that the mirror we created earlier is still our connection's
        // mirror because there is no strict source->mirror pairing for map connections
        // (a different identical map connection can take its place during any mirrored change).
        if (!MapConnection::areMirrored(this->connection, this->mirror))
            this->mirror = this->connection->findMirror();

        this->mirrorMap->removeConnection(this->mirror);
    }
    this->map->removeConnection(this->connection);

    QUndoCommand::undo();
}

int MapConnectionAdd::id() const {
    return CommandId::ID_MapConnectionAdd | getConnectionDirectionMask({this->connection->direction()});
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

MapConnectionRemove::MapConnectionRemove(Map *map, MapConnection *connection,
    QUndoCommand *parent) : QUndoCommand(parent) {
    setText("Remove Map Connection");

    this->map = map;
    this->connection = connection;

    if (porymapConfig.mirrorConnectingMaps) {
        this->mirror = this->connection->findMirror();
        this->mirrorMap = this->connection->targetMap();
    }
}

void MapConnectionRemove::redo() {
    QUndoCommand::redo();

    if (this->mirrorMap) {
        // See comment in MapConnectionAdd::undo
        if (!MapConnection::areMirrored(this->connection, this->mirror))
            this->mirror = this->connection->findMirror();

        this->mirrorMap->removeConnection(this->mirror);
    }
    this->map->removeConnection(this->connection);
}

void MapConnectionRemove::undo() {
    this->map->addConnection(this->connection);
    if (this->mirrorMap)
        this->mirrorMap->addConnection(this->mirror);

    QUndoCommand::undo();
}

int MapConnectionRemove::id() const {
    return CommandId::ID_MapConnectionRemove | getConnectionDirectionMask({this->connection->direction()});
}
