#pragma once
#ifndef PAINTSELECTION_H
#define PAINTSELECTION_H

#include "block.h"
#include "map.h"

#include <QList>
#include <QPoint>

struct MetatileSelectionItem
{
    bool enabled;
    uint16_t metatileId;
};

struct CollisionSelectionItem
{
    bool enabled;
    uint16_t collision;
    uint16_t elevation;
};

class PaintSelection
{
public:
    QPoint dimensions;
    virtual bool paintNormal(int, Block*, Map*, int layer) = 0;
};

class MetatileSelection: public PaintSelection
{
public:
    MetatileSelection() {}
    void clone(MetatileSelection other) {
        this->dimensions = other.dimensions;
        this->hasCollision = other.hasCollision;
        this->metatileItems = other.metatileItems;
        this->collisionItems = other.collisionItems;
    }
    bool paintNormal(int index, Block *block, Map*, int layer) override;
    bool hasCollision = false;
    QList<MetatileSelectionItem> metatileItems;
    QList<CollisionSelectionItem> collisionItems;
};

class StampSelection: public PaintSelection
{
public:
    StampSelection() {}
    bool paintNormal(int index, Block *block, Map*, int layer) override;
    QList<uint16_t> stampIds;
};

#endif // PAINTSELECTION_H
