#ifndef EDITCOMMANDS_H
#define EDITCOMMANDS_H

#include <QUndoCommand>
#include <QList>

class MapPixmapItem;
class Map;
class Blockdata;
class Event;
class DraggablePixmapItem;
class Editor;

enum CommandId {
    ID_PaintMetatile,
    ID_BucketFillMetatile,
    ID_MagicFillMetatile,
    ID_ShiftMetatiles,
    ID_PaintCollision,
    ID_BucketFillCollision,
    ID_MagicFillCollision,
    ID_ResizeMap,
    ID_PaintBorder,
    ID_EventMove,
    ID_EventShift,
    ID_EventCreate,
    ID_EventDelete,
    ID_EventDuplicate,
    ID_EventSetData,
    ID_ScriptEditMap,
};



/// Implements a command to commit metatile paint actions
/// onto the map using the pencil tool.
class PaintMetatile : public QUndoCommand {
public:
    PaintMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr);
    virtual ~PaintMetatile();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_PaintMetatile; }

private:
    Map *map;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    unsigned actionId;
};



/// Implements a command to commit paint actions
/// on the metatile collision and elevation.
class PaintCollision : public PaintMetatile {
public:
    PaintCollision(Map *map,
        Blockdata *oldCollision, Blockdata *newCollision,
        unsigned actionId, QUndoCommand *parent = nullptr)
    : PaintMetatile(map, oldCollision, newCollision, actionId, parent) {
        setText("Paint Collision");
    }

    int id() const override { return CommandId::ID_PaintCollision; }
};



/// Implements a command to commit paint actions on the map border.
class PaintBorder : public QUndoCommand {
public:
    PaintBorder(Map *map,
        Blockdata *oldBorder, Blockdata *newBorder,
        unsigned actionId, QUndoCommand *parent = nullptr);
    ~PaintBorder();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; };
    int id() const override { return CommandId::ID_PaintBorder; }

private:
    Map *map;

    Blockdata *newBorder;
    Blockdata *oldBorder;

    unsigned actionId;
};



/// Implements a command to commit flood fill metatile actions
/// with the bucket tool onto the map.
class BucketFillMetatile : public PaintMetatile {
public:
    BucketFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr)
      : PaintMetatile(map, oldMetatiles, newMetatiles, actionId, parent) {
        setText("Bucket Fill Metatiles");
    }

    int id() const override { return CommandId::ID_BucketFillMetatile; }
};



/// Implements a command to commit flood fill actions
/// on the metatile collision and elevation.
class BucketFillCollision : public PaintCollision {
public:
    BucketFillCollision(Map *map,
        Blockdata *oldCollision, Blockdata *newCollision,
        QUndoCommand *parent = nullptr)
      : PaintCollision(map, oldCollision, newCollision, -1, parent) {
        setText("Flood Fill Collision");
    }

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_BucketFillCollision; }
};



/// Implements a command to commit magic fill metatile actions
/// with the bucket or paint tool onto the map.
class MagicFillMetatile : public PaintMetatile {
public:
    MagicFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr)
      : PaintMetatile(map, oldMetatiles, newMetatiles, actionId, parent) {
        setText("Magic Fill Metatiles");
    }

    int id() const override { return CommandId::ID_MagicFillMetatile; }
};



/// Implements a command to commit magic fill collision actions.
class MagicFillCollision : public PaintCollision {
public:
    MagicFillCollision(Map *map,
        Blockdata *oldCollision, Blockdata *newCollision,
        QUndoCommand *parent = nullptr)
    : PaintCollision(map, oldCollision, newCollision, -1, parent) {
        setText("Magic Fill Collision");
    }

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_MagicFillCollision; }
};



/// Implements a command to commit metatile shift actions.
class ShiftMetatiles : public QUndoCommand {
public:
    ShiftMetatiles(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned actionId, QUndoCommand *parent = nullptr);
    ~ShiftMetatiles();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_ShiftMetatiles; }

private:
    Map *map;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    unsigned actionId;
};



/// Implements a command to commit a map or border resize action.
class ResizeMap : public QUndoCommand {
public:
    ResizeMap(Map *map, QSize oldMapDimensions, QSize newMapDimensions,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        QSize oldBorderDimensions, QSize newBorderDimensions,
        Blockdata *oldBorder, Blockdata *newBorder,
        QUndoCommand *parent = nullptr);
    ~ResizeMap();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_ResizeMap; }

private:
    Map *map;

    int oldMapWidth;
    int oldMapHeight;
    int newMapWidth;
    int newMapHeight;

    int oldBorderWidth;
    int oldBorderHeight;
    int newBorderWidth;
    int newBorderHeight;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    Blockdata *newBorder;
    Blockdata *oldBorder;
};



/// Implements a command to commit a single- or multi-Event move action.
/// Actions are merged into one until the mouse is released.
class EventMove : public QUndoCommand {
public:
    EventMove(QList<Event *> events,
        int deltaX, int deltaY, unsigned actionId,
        QUndoCommand *parent = nullptr);
    ~EventMove();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_EventMove; }

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
    ~EventShift();

    int id() const override { return CommandId::ID_EventShift; }
};



/// Implements a command to commit Event create actions.
/// Works for a single Event only.
class EventCreate : public QUndoCommand {
public:
    EventCreate(Editor *editor, Map *map, Event *event,
        QUndoCommand *parent = nullptr);
    ~EventCreate();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_EventCreate; }

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
    ~EventDelete();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_EventDelete; }

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
    ~EventDuplicate();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; }
    int id() const override { return CommandId::ID_EventDuplicate; }

private:
    Map *map;
    QList<Event *> selectedEvents; // allow multiple deletion of events
    Editor *editor;
};



/// Implements a command to commit map edits from the scripting API.
/// The scripting api can edit metatiles and map dimensions.
class ScriptEditMap : public QUndoCommand {
public:
    ScriptEditMap(Map *map,
        QSize oldMapDimensions, QSize newMapDimensions,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        QUndoCommand *parent = nullptr);
    ~ScriptEditMap();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *) override { return false; };
    int id() const override { return CommandId::ID_ScriptEditMap; }

private:
    Map *map;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    int oldMapWidth;
    int oldMapHeight;
    int newMapWidth;
    int newMapHeight;
};

#endif // EDITCOMMANDS_H
