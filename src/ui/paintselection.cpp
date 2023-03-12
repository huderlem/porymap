#include "paintselection.h"
#include "project.h"
#include "metatile.h"
#include "log.h"

bool MetatileSelection::paintNormal(int index, Block *block, Map *map, StampLayer stampLayer) {
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

int getLayerOffset(StampLayer stampLayer) {
    switch (stampLayer)
    {
    case StampLayer::STAMP_LAYER_BOTTOM:
        return 0;
    case StampLayer::STAMP_LAYER_TOP:
        return 4;
    default:
        return 0;
    }
}

bool StampSelection::paintNormal(int index, Block *block, Map *map, StampLayer stampLayer) {
    Tileset *primaryTileset = map->layout->tileset_primary;
    Tileset *secondaryTileset = map->layout->tileset_secondary;
    // 1. Build metatile by applying the stamp to the existing block.
    Metatile metatile = *(Tileset::getMetatile(block->metatileId, map->layout->tileset_primary, map->layout->tileset_secondary));
    int baseOffset = getLayerOffset(stampLayer);
    metatile.tiles[baseOffset] = Tile(0x5, false, false, 2);
    metatile.tiles[baseOffset + 1] = Tile(0x5, true, false, 2);
    metatile.tiles[baseOffset + 2] = Tile(0x15, false, false, 2);
    metatile.tiles[baseOffset + 3] = Tile(0x15, true, false, 2);

    // 2. Check if the metatile already exists in the map layout's primary or secondary tilesets.
    bool exists = false;
    uint16_t metatileId = 0;
    uint16_t i = 0;
    for (auto m : primaryTileset->metatiles) {
        if (*m == metatile) {
            exists = true;
            metatileId = i;
            break;
        }
        i++;
    }
    if (!exists) {
        i = Project::getNumMetatilesPrimary();
        for (auto m : secondaryTileset->metatiles) {
            if (*m == metatile) {
                exists = true;
                metatileId = i;
                break;
            }
            i++;
        }
    }

    // 3. If the metatile doesn't exist, append the new metatile to the secondary tileset.
    if (!exists) {
        int maxSecondaryMetatiles = Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary();
        if (secondaryTileset->metatiles.length() >= maxSecondaryMetatiles) {
            return false;
        }
        secondaryTileset->metatiles.append(new Metatile(metatile));
        metatileId = Project::getNumMetatilesPrimary() + secondaryTileset->metatiles.length() - 1;
    }

    // 4. Set the block's metatile id.
    block->metatileId = metatileId;
    return true;
}
