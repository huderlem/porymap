#include "block.h"

Block::Block() :
    m_metatileId(0),
    m_collision(0),
    m_elevation(0) 
{  }

Block::Block(uint16_t metatileId, uint16_t collision, uint16_t elevation) :
    m_metatileId(metatileId),
    m_collision(collision),
    m_elevation(elevation)
{  }

Block::Block(uint16_t word) :
    m_metatileId(word & 0x3ff),
    m_collision((word >> 10) & 0x3),
    m_elevation((word >> 12) & 0xf)
{  }

Block::Block(const Block &other) :
    m_metatileId(other.m_metatileId),
    m_collision(other.m_collision),
    m_elevation(other.m_elevation)
{  }

Block &Block::operator=(const Block &other) {
    m_metatileId = other.m_metatileId;
    m_collision = other.m_collision;
    m_elevation = other.m_elevation;
    return *this;
}

uint16_t Block::rawValue() const {
    return static_cast<uint16_t>(
                (m_metatileId & 0x3ff) +
                ((m_collision & 0x3) << 10) +
                ((m_elevation & 0xf) << 12));
}

bool Block::operator ==(Block other) const {
    return (m_metatileId == other.m_metatileId) && (m_collision == other.m_collision) && (m_elevation == other.m_elevation);
}

bool Block::operator !=(Block other) const {
    return !(operator ==(other));
}
