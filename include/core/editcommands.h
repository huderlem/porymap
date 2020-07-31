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
    ID_PaintMetatile,       // - done
    ID_BucketFillMetatile,  // - done
    ID_MagicFillMetatile,   // - done
    ID_ShiftMetatiles,      // - done
    ID_ResizeMap,           // - done
    ID_PaintBorder,         // - done
    ID_EventMove,           // - done
    ID_EventShift,          // - done
    ID_EventCreate,         // - done
    ID_EventDelete,         // - done
    ID_EventSetData,        // - ?
    // Region map editor history commands
};



/// Implements a command to commit metatile paint actions
/// onto the map using the pencil tool.
class PaintMetatile : public QUndoCommand {
public:
    PaintMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~PaintMetatile();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_PaintMetatile; }

private:
    Map *map;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    unsigned eventId;
};



/// Implements a command to commit paint actions on the map border.
class PaintBorder : public QUndoCommand {
public:
    PaintBorder(Map *map,
        Blockdata *oldBorder, Blockdata *newBorder,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~PaintBorder();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override { return false; };
    int id() const override { return CommandId::ID_PaintBorder; }

private:
    Map *map;

    Blockdata *newBorder;
    Blockdata *oldBorder;

    unsigned eventId;
};



/// Implements a command to commit flood fill metatile actions
/// with the bucket tool onto the map.
class BucketFillMetatile : public PaintMetatile {
public:
    BucketFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~BucketFillMetatile();

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return CommandId::ID_BucketFillMetatile; }
};



/// Implements a command to commit magic fill metatile actions
/// with the bucket or paint tool onto the map.
class MagicFillMetatile : public PaintMetatile {
public:
    MagicFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~MagicFillMetatile();

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return CommandId::ID_BucketFillMetatile; }
};




/// Implements a command to commit metatile shift actions.
class ShiftMetatiles : public QUndoCommand {
public:
    ShiftMetatiles(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~ShiftMetatiles();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return CommandId::ID_ShiftMetatiles; }

private:
    Map *map;

    Blockdata *newMetatiles;
    Blockdata *oldMetatiles;

    unsigned eventId;
    bool mergeable = false;
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

    bool mergeWith(const QUndoCommand *command) override { return false; }
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

    bool mergeWith(const QUndoCommand *command) override { return false; }
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
    EventDelete(Editor *editor, Map *map, QList<Event *> selectedEvents,
        QUndoCommand *parent = nullptr);
    ~EventDelete();

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return CommandId::ID_EventDelete; }

private:
    Map *map;
    QList<Event *> selectedEvents; // allow multiple deletion of events
    Editor *editor;
};

#endif // EDITCOMMANDS_H
