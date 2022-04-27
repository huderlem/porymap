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
};


/// Implements a command to commit tilemap paint actions
class EditTilemap : public QUndoCommand {
public:
    EditTilemap(RegionMap *map, QByteArray oldTilemap, QByteArray newTilemap, unsigned actionId, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return RMCommandId::ID_EditTilemap; }

private:
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
};


/// Add Entry
class AddEntry : public EditEntry {
public:
    AddEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
};

// ResizeTilemap

// ResizeMap

#endif // REGIONMAPEDITCOMMANDS_H
