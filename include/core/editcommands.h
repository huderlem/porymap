#pragma once
#ifndef EDITCOMMANDS_H
#define EDITCOMMANDS_H

#include "blockdata.h"
#include "mapconnection.h"

#include <QUndoCommand>
#include <QList>
#include <QPointer>

class Map;
class Layout;
class Blockdata;
class Event;
class DraggablePixmapItem;
class Editor;

enum CommandId {
    ID_PaintMetatile = 0,
    ID_BucketFillMetatile,
    ID_MagicFillMetatile,
    ID_ShiftMetatiles,
    ID_PaintCollision,
    ID_BucketFillCollision,
    ID_MagicFillCollision,
    ID_ResizeLayout,
    ID_PaintBorder,
    ID_ScriptEditLayout,
    ID_EventMove,
    ID_EventShift,
    ID_EventCreate,
    ID_EventDelete,
    ID_EventDuplicate,
    ID_EventPaste,
    ID_MapConnectionMove,
    ID_MapConnectionChangeDirection,
    ID_MapConnectionChangeMap,
    ID_MapConnectionAdd,
    ID_MapConnectionRemove,
};

#define IDMask_EventType_Object  (1 << 8)
#define IDMask_EventType_Warp    (1 << 9)
#define IDMask_EventType_BG      (1 << 10)
#define IDMask_EventType_Trigger (1 << 11)
#define IDMask_EventType_Heal    (1 << 12)

/// Implements a command to commit metatile paint actions
/// onto the map using the pencil tool.
class PaintMetatile : public QUndoCommand {
public:
    PaintMetatile(Layout *layout,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_PaintMetatile; }

private:
    Layout *layout;

    Blockdata newMetatiles;
    Blockdata oldMetatiles;

    unsigned actionId;
};



/// Implements a command to commit paint actions
/// on the metatile collision and elevation.
class PaintCollision : public PaintMetatile {
public:
    PaintCollision(Layout *layout,
        const Blockdata &oldCollision, const Blockdata &newCollision,
        unsigned actionId, QUndoCommand *parent = nullptr)
    : PaintMetatile(layout, oldCollision, newCollision, actionId, parent) {
        setText("Paint Collision");
    }

    int id() const override { return CommandId::ID_PaintCollision; }
};



/// Implements a command to commit paint actions on the map border.
class PaintBorder : public QUndoCommand {
public:
    PaintBorder(Layout *layout,
        const Blockdata &oldBorder, const Blockdata &newBorder,
        unsigned actionId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; };
    int id() const override { return CommandId::ID_PaintBorder; }

private:
    Layout *layout;

    Blockdata newBorder;
    Blockdata oldBorder;

    unsigned actionId;
};



/// Implements a command to commit flood fill metatile actions
/// with the bucket tool onto the map.
class BucketFillMetatile : public PaintMetatile {
public:
    BucketFillMetatile(Layout *layout,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr)
      : PaintMetatile(layout, oldMetatiles, newMetatiles, actionId, parent) {
        setText("Bucket Fill Metatiles");
    }

    int id() const override { return CommandId::ID_BucketFillMetatile; }
};



/// Implements a command to commit flood fill actions
/// on the metatile collision and elevation.
class BucketFillCollision : public PaintCollision {
public:
    BucketFillCollision(Layout *layout,
        const Blockdata &oldCollision, const Blockdata &newCollision,
        QUndoCommand *parent = nullptr)
      : PaintCollision(layout, oldCollision, newCollision, -1, parent) {
        setText("Flood Fill Collision");
    }

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_BucketFillCollision; }
};



/// Implements a command to commit magic fill metatile actions
/// with the bucket or paint tool onto the map.
class MagicFillMetatile : public PaintMetatile {
public:
    MagicFillMetatile(Layout *layout,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr)
      : PaintMetatile(layout, oldMetatiles, newMetatiles, actionId, parent) {
        setText("Magic Fill Metatiles");
    }

    int id() const override { return CommandId::ID_MagicFillMetatile; }
};



/// Implements a command to commit magic fill collision actions.
class MagicFillCollision : public PaintCollision {
public:
    MagicFillCollision(Layout *layout,
        const Blockdata &oldCollision, const Blockdata &newCollision,
        QUndoCommand *parent = nullptr)
    : PaintCollision(layout, oldCollision, newCollision, -1, parent) {
        setText("Magic Fill Collision");
    }

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_MagicFillCollision; }
};



/// Implements a command to commit metatile shift actions.
class ShiftMetatiles : public QUndoCommand {
public:
    ShiftMetatiles(Layout *layout,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_ShiftMetatiles; }

private:
    Layout *layout= nullptr;

    Blockdata newMetatiles;
    Blockdata oldMetatiles;

    unsigned actionId;
};



/// Implements a command to commit a map or border resize action.
class ResizeLayout : public QUndoCommand {
public:
    ResizeLayout(Layout *layout, QSize oldLayoutDimensions, QSize newLayoutDimensions,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        QSize oldBorderDimensions, QSize newBorderDimensions,
        const Blockdata &oldBorder, const Blockdata &newBorder,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_ResizeLayout; }

private:
    Layout *layout = nullptr;

    int oldLayoutWidth;
    int oldLayoutHeight;
    int newLayoutWidth;
    int newLayoutHeight;

    int oldBorderWidth;
    int oldBorderHeight;
    int newBorderWidth;
    int newBorderHeight;

    Blockdata newMetatiles;
    Blockdata oldMetatiles;

    Blockdata newBorder;
    Blockdata oldBorder;
};



/// Implements a command to commit a single- or multi-Event move action.
/// Actions are merged into one until the mouse is released.
class EventMove : public QUndoCommand {
public:
    EventMove(QList<Event *> events,
        int deltaX, int deltaY, unsigned actionId,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override;

private:
    QList<Event *> events;
    int deltaX;
    int deltaY;

    unsigned actionId;
};



/// Implements a command to commit Event shift actions.
class EventShift : public EventMove {
public:
    EventShift(QList<Event *> events,
        int deltaX, int deltaY, unsigned actionId,
        QUndoCommand *parent = nullptr);
    int id() const override;
private:
    QList<Event *> events;
};



/// Implements a command to commit Event create actions.
/// Works for a single Event only.
class EventCreate : public QUndoCommand {
public:
    EventCreate(Editor *editor, Map *map, Event *event,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override;

private:
    Map *map;
    Event *event;
    Editor *editor;
};



/// Implements a command to commit Event deletions.
/// Applies to every currently selected Event.
class EventDelete : public QUndoCommand {
public:
    EventDelete(Editor *editor, Map *map,
        QList<Event *> selectedEvents, Event *nextSelectedEvent,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override;

private:
    Editor *editor;
    Map *map;
    QList<Event *> selectedEvents; // allow multiple deletion of events
    Event *nextSelectedEvent;
};



/// Implements a command to commit Event duplications.
class EventDuplicate : public QUndoCommand {
public:
    EventDuplicate(Editor *editor, Map *map, QList<Event *> selectedEvents,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override;

protected:
    Map *map;
    QList<Event *> selectedEvents; // allow multiple deletion of events
    Editor *editor;
};



/// Implements a command to commit Event pastes from clipboard.
class EventPaste : public EventDuplicate {
public:
    EventPaste(Editor *editor, Map *map, QList<Event *> pastedEvents,
        QUndoCommand *parent = nullptr);

    int id() const override;
};



/// Implements a command to commit map edits from the scripting API.
/// The scripting api can edit map/border blocks and dimensions.
class ScriptEditLayout : public QUndoCommand {
public:
    ScriptEditLayout(Layout *layout,
        QSize oldLayoutDimensions, QSize newLayoutDimensions,
        const Blockdata &oldMetatiles, const Blockdata &newMetatiles,
        QSize oldBorderDimensions, QSize newBorderDimensions,
        const Blockdata &oldBorder, const Blockdata &newBorder,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_ScriptEditLayout; }

private:
    Layout *layout = nullptr;

    Blockdata newMetatiles;
    Blockdata oldMetatiles;

    Blockdata newBorder;
    Blockdata oldBorder;

    int oldLayoutWidth;
    int oldLayoutHeight;
    int newLayoutWidth;
    int newLayoutHeight;

    int oldBorderWidth;
    int oldBorderHeight;
    int newBorderWidth;
    int newBorderHeight;
};



/// Implements a command to commit Map Connectien move actions.
/// Actions are merged into one until the mouse is released when editing by click-and-drag,
/// or when the offset spin box loses focus when editing with the list UI.
class MapConnectionMove : public QUndoCommand {
public:
    MapConnectionMove(MapConnection *connection, int newOffset, unsigned actionId,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_MapConnectionMove; }

private:
    MapConnection *connection;
    int newOffset;
    int oldOffset;
    bool mirrored;
    unsigned actionId;
};



/// Implements a command to commit changes to a Map Connectien's 'direction' field.
class MapConnectionChangeDirection : public QUndoCommand {
public:
    MapConnectionChangeDirection(MapConnection *connection, QString newDirection,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return CommandId::ID_MapConnectionChangeDirection; }

private:
    QPointer<MapConnection> connection;
    QString newDirection;
    QString oldDirection;
    int oldOffset;
    int newOffset;
    bool mirrored;
};



/// Implements a command to commit changes to a Map Connectien's 'map' field.
class MapConnectionChangeMap : public QUndoCommand {
public:
    MapConnectionChangeMap(MapConnection *connection, QString newMapName,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return CommandId::ID_MapConnectionChangeMap; }

private:
    QPointer<MapConnection> connection;
    QString newMapName;
    QString oldMapName;
    int oldOffset;
    int newOffset;
    bool mirrored;
};



/// Implements a command to commit adding a Map Connection to a map.
class MapConnectionAdd : public QUndoCommand {
public:
    MapConnectionAdd(Map *map, MapConnection *connection,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return CommandId::ID_MapConnectionAdd; }

private:
    Map *map = nullptr;
    Map *mirrorMap = nullptr;
    QPointer<MapConnection> connection = nullptr;
    QPointer<MapConnection> mirror = nullptr;
};



/// Implements a command to commit removing a Map Connection from a map.
class MapConnectionRemove : public QUndoCommand {
public:
    MapConnectionRemove(Map *map, MapConnection *connection,
        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return CommandId::ID_MapConnectionRemove; }

private:
    Map *map = nullptr;
    Map *mirrorMap = nullptr;
    QPointer<MapConnection> connection = nullptr;
    QPointer<MapConnection> mirror = nullptr;
};


#endif // EDITCOMMANDS_H
