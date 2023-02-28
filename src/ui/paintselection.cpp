#include "paintselection.h"
#include "log.h"

bool MetatileSelection::paintNormal(int index, Block *block) {
    MetatileSelectionItem item = this->metatileItems.at(index);
    if (!item.enabled)
        return false;

    block->metatileId = item.metatileId;
    if (this->hasCollision && this->collisionItems.length() == this->metatileItems.length()) {
        CollisionSelectionItem collisionItem = this->collisionItems.at(index);
        block->collision = collisionItem.collision;
        block->elevation = collisionItem.elevation;
    }
    return true;
}

bool StampSelection::paintNormal(int index, Block *block) {
    block->metatileId = 1;
    return true;
}
