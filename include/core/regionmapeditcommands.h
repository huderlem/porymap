#pragma once
#ifndef REGIONMAPEDITCOMMANDS_H
#define REGIONMAPEDITCOMMANDS_H

#include "regionmap.h"

#include <QUndoCommand>
#include <QList>

class RegionMap;

enum RMCommandId {
    ID_EditTilemap = 0,
    ID_EditLayout,
    ID_ResizeLayout,
    ID_EditEntry,
    ID_RemoveEntry,
    ID_AddEntry,
    ID_ResizeTilemap,
    ID_ClearEntries,
};


/// Implements a command to commit tilemap paint actions
class EditTilemap : public QUndoCommand {
public:
    EditTilemap(RegionMap *map, QByteArray oldTilemap, QByteArray newTilemap, unsigned actionId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return RMCommandId::ID_EditTilemap; }

protected:
    RegionMap *map;

    QByteArray oldTilemap;
    QByteArray newTilemap;

    unsigned actionId;
};


/// Edit region map section layout
class EditLayout : public QUndoCommand {
public:
    EditLayout(RegionMap *map, QString layer, int index, QList<LayoutSquare> oldLayout, QList<LayoutSquare> newLayout, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return RMCommandId::ID_EditLayout; }

private:
    RegionMap *map;

    int index;
    QString layer;
    QList<LayoutSquare> oldLayout;
    QList<LayoutSquare> newLayout;
};


/// Edit Layout Dimensions
class ResizeLayout : public QUndoCommand {
public:
    ResizeLayout(RegionMap *map, int oldWidth, int oldHeight, int newWidth, int newHeight,
        QMap<QString, QList<LayoutSquare>> oldLayouts, QMap<QString, QList<LayoutSquare>> newLayouts, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return RMCommandId::ID_ResizeLayout; }

private:
    RegionMap *map;

    int oldWidth;
    int oldHeight;
    int newWidth;
    int newHeight;
    QMap<QString, QList<LayoutSquare>> oldLayouts;
    QMap<QString, QList<LayoutSquare>> newLayouts;
};


/// Edit Entry Value
class EditEntry : public QUndoCommand {
public:
    EditEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return RMCommandId::ID_EditEntry; }

protected:
    RegionMap *map;

    QString section;
    MapSectionEntry oldEntry;
    MapSectionEntry newEntry;
};


/// Remove Entry
class RemoveEntry : public EditEntry {
public:
    RemoveEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return RMCommandId::ID_RemoveEntry; }
};


/// Add Entry
class AddEntry : public EditEntry {
public:
    AddEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return RMCommandId::ID_AddEntry; }
};


/// ResizeTilemap
class ResizeTilemap : public EditTilemap {
public:
    ResizeTilemap(RegionMap *map, QByteArray oldTilemap, QByteArray newTilemap,
        int oldWidth, int oldHeight, int newWidth, int newHeight, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    int id() const override { return RMCommandId::ID_ResizeTilemap; }

private:
    int oldWidth;
    int oldHeight;
    int newWidth;
    int newHeight;
};


/// ClearEntries
class ClearEntries : public QUndoCommand {
public:
    ClearEntries(RegionMap *map, tsl::ordered_map<QString, MapSectionEntry>, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override { return false; }
    int id() const override { return RMCommandId::ID_ClearEntries; }

private:
    RegionMap *map;
    tsl::ordered_map<QString, MapSectionEntry> entries;
};

#endif // REGIONMAPEDITCOMMANDS_H
