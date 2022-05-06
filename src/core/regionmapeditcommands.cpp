#include "regionmapeditcommands.h"



EditTilemap::EditTilemap(RegionMap *map, QByteArray oldTilemap, QByteArray newTilemap, unsigned actionId, QUndoCommand *parent)
    : QUndoCommand(parent) {
    setText("Edit Tilemap");

    this->map = map;
    this->oldTilemap = oldTilemap;
    this->newTilemap = newTilemap;
    this->actionId = actionId;
}

void EditTilemap::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setTilemap(this->newTilemap);
}

void EditTilemap::undo() {
    if (!map) return;

    map->setTilemap(this->oldTilemap);

    QUndoCommand::undo();
}

bool EditTilemap::mergeWith(const QUndoCommand *command) {
    const EditTilemap *other = static_cast<const EditTilemap *>(command);

    if (this->map != other->map)
        return false;

    if (this->actionId != other->actionId)
        return false;

    newTilemap = other->newTilemap;

    return true;
}

///

EditLayout::EditLayout(RegionMap *map, QString layer, int index, QList<LayoutSquare> oldLayout, QList<LayoutSquare> newLayout, QUndoCommand *parent)
    : QUndoCommand(parent) {
    setText("Edit Layout");

    this->map = map;
    this->layer = layer;
    this->index = index;
    this->oldLayout = oldLayout;
    this->newLayout = newLayout;
}

void EditLayout::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setLayout(this->layer, this->newLayout);
}

void EditLayout::undo() {
    if (!map) return;

    map->setLayout(this->layer, this->oldLayout);

    QUndoCommand::undo();
}

bool EditLayout::mergeWith(const QUndoCommand *command) {
    const EditLayout *other = static_cast<const EditLayout *>(command);

    if (this->map != other->map)
        return false;

    if (this->text() != other->text())
        return false;

    if (this->index != other->index)
        return false;

    this->newLayout = other->newLayout;

    return true;
}

///

ResizeLayout::ResizeLayout(RegionMap *map, int oldWidth, int oldHeight, int newWidth, int newHeight,
        QMap<QString, QList<LayoutSquare>> oldLayouts, QMap<QString, QList<LayoutSquare>> newLayouts, QUndoCommand *parent)
    : QUndoCommand(parent) {
    setText("Change Layout Dimensions");

    this->map = map;
    this->oldWidth = oldWidth;
    this->oldHeight = oldHeight;
    this->newWidth = newWidth;
    this->newHeight = newHeight;
    this->oldLayouts = oldLayouts;
    this->newLayouts = newLayouts;
}

void ResizeLayout::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setLayoutDimensions(newWidth, newHeight, false);
    map->setAllLayouts(this->newLayouts);
}

void ResizeLayout::undo() {
    if (!map) return;

    map->setLayoutDimensions(oldWidth, oldHeight, false);
    map->setAllLayouts(this->oldLayouts);

    QUndoCommand::undo();
}

bool ResizeLayout::mergeWith(const QUndoCommand *command) {
    const ResizeLayout *other = static_cast<const ResizeLayout *>(command);

    if (this->map != other->map)
        return false;

    // always allow consecutive dimension changes to merge

    this->newWidth = other->newWidth;
    this->newHeight = other->newHeight;
    this->newLayouts = other->newLayouts;

    return true;
}

///

EditEntry::EditEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent)
    : QUndoCommand(parent) {
    setText("Change Entry for " + section);

    this->map = map;
    this->section = section;
    this->oldEntry = oldEntry;
    this->newEntry = newEntry;
}

void EditEntry::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setEntry(section, newEntry);
}

void EditEntry::undo() {
    if (!map) return;

    map->setEntry(section, oldEntry);

    QUndoCommand::undo();
}

bool EditEntry::mergeWith(const QUndoCommand *command) {
    const EditEntry *other = static_cast<const EditEntry *>(command);

    if (this->map != other->map)
        return false;

    if (this->section != other->section)
        return false;

    this->newEntry = other->newEntry;

    return true;
}

///

AddEntry::AddEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent)
    : EditEntry(map, section, oldEntry, newEntry, parent) {
    setText("Add Entry for " + section);
}

void AddEntry::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->setEntry(section, newEntry);
}

void AddEntry::undo() {
    if (!map) return;

    map->removeEntry(section);

    QUndoCommand::undo();
}

///

RemoveEntry::RemoveEntry(RegionMap *map, QString section, MapSectionEntry oldEntry, MapSectionEntry newEntry, QUndoCommand *parent)
    : EditEntry(map, section, oldEntry, newEntry, parent) {
    setText("Remove Entry for " + section);
}

void RemoveEntry::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->removeEntry(section);
}

void RemoveEntry::undo() {
    if (!map) return;

    map->setEntry(section, oldEntry);

    QUndoCommand::undo();
}

///

ResizeTilemap::ResizeTilemap(RegionMap *map, QByteArray oldTilemap, QByteArray newTilemap,
        int oldWidth, int oldHeight, int newWidth, int newHeight, QUndoCommand *parent) 
    : EditTilemap(map, oldTilemap, newTilemap, -1, parent) {
    setText("Resize Tilemap");

    this->oldWidth = oldWidth;
    this->oldHeight = oldHeight;
    this->newWidth = newWidth;
    this->newHeight = newHeight;
}

void ResizeTilemap::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->resizeTilemap(this->newWidth, this->newHeight, false);
    map->setTilemap(this->newTilemap);
    map->emitDisplay();
}

void ResizeTilemap::undo() {
    if (!map) return;

    map->resizeTilemap(this->oldWidth, this->oldHeight, false);
    map->setTilemap(this->oldTilemap);
    map->emitDisplay();

    QUndoCommand::undo();
}

///

ClearEntries::ClearEntries(RegionMap *map, tsl::ordered_map<QString, MapSectionEntry> entries, QUndoCommand *parent) {
    setText("Clear Entries");

    this->map = map;
    this->entries = entries;
}

void ClearEntries::redo() {
    QUndoCommand::redo();

    if (!map) return;

    map->clearEntries();
}

void ClearEntries::undo() {
    if (!map) return;

    map->setEntries(entries);

    QUndoCommand::undo();
}


