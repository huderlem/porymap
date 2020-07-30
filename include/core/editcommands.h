#ifndef EDITCOMMANDS_H
#define EDITCOMMANDS_H

#include <QUndoCommand>
#include <QList>

class MapPixmapItem;
class Map;
class Blockdata;
class Event;
class DraggablePixmapItem;

enum CommandId {
    ID_PaintMetatile,       // - done
    ID_BucketFillMetatile,  // - done
    ID_MagicFillMetatile,   // - done
    ID_ShiftMetatiles,      // - done
    ID_ResizeMap,           // - done
    ID_PaintBorder,         // - done
    ID_EventMove,           // - done
    ID_EventShift,          // - 
    ID_EventCreate,         // - 
    ID_EventDelete,         // - 
    ID_EventSetData,        // - ?
    // Tileset editor history commands
    // Region map editor history commands
};



/// TODO
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



/// 
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



///
class BucketFillMetatile : public PaintMetatile {
public:
    BucketFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~BucketFillMetatile();

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return CommandId::ID_BucketFillMetatile; }
};



///
class MagicFillMetatile : public PaintMetatile {
public:
    MagicFillMetatile(Map *map,
        Blockdata *oldMetatiles, Blockdata *newMetatiles,
        unsigned eventId, QUndoCommand *parent = nullptr);
    ~MagicFillMetatile();

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return CommandId::ID_BucketFillMetatile; }
};




///
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



///
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



///
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

#endif // EDITCOMMANDS_H
