#include "block.h"

Block::Block() : tile(0), collision(0), elevation(0) { }

Block::Block(uint16_t tile, uint16_t collision, uint16_t elevation) :
    tile(tile),
    collision(collision),
    elevation(elevation)
{  }

Block::Block(uint16_t word) :
    tile(word & 0x3ff),
    collision((word >> 10) & 0x3),
    elevation((word >> 12) & 0xf)
{  }

Block::Block(const Block &other) :
    tile(other.tile),
    collision(other.collision),
    elevation(other.elevation)
{  }

Block &Block::operator=(const Block &other) {
    tile = other.tile;
    collision = other.collision;
    elevation = other.elevation;
    return *this;
}

uint16_t Block::rawValue() const {
    return static_cast<uint16_t>(
                (tile & 0x3ff) +
                ((collision & 0x3) << 10) +
                ((elevation & 0xf) << 12));
}

bool Block::operator ==(Block other) const {
    return (tile == other.tile) && (collision == other.collision) && (elevation == other.elevation);
}

bool Block::operator !=(Block other) const {
    return !(operator ==(other));
}
