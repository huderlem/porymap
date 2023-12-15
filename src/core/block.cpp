#include "block.h"
#include "bitpacker.h"
#include "config.h"

// Upper limit for metatile ID, collision, and elevation masks. Used externally.
const uint16_t Block::maxValue = 0xFFFF;

static BitPacker bitsMetatileId = BitPacker(0x3FF);
static BitPacker bitsCollision = BitPacker(0xC00);
static BitPacker bitsElevation = BitPacker(0xF000);

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

Block::Block(uint16_t data) :
    m_metatileId(bitsMetatileId.unpack(data)),
    m_collision(bitsCollision.unpack(data)),
    m_elevation(bitsElevation.unpack(data))
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
    return bitsMetatileId.pack(m_metatileId)
          | bitsCollision.pack(m_collision)
          | bitsElevation.pack(m_elevation);
}

// TODO: After parsing, recalc config (or parsed!) values that depend on max collision/elevation
/*      - newMapMetatileId
        - newMapElevation
        - newMapCollision
        - newMapBorderMetatileIds
        - collisionSheetWidth
        - collisionSheetHeight
        - NUM_METATILES_IN_PRIMARY
        - event elevations
        - metatile labels?

*/
// TODO: Settings editor -- disable UI & restore after refresh
void Block::setLayout() {
    bitsMetatileId.setMask(projectConfig.getBlockMetatileIdMask());
    bitsCollision.setMask(projectConfig.getBlockCollisionMask());
    bitsElevation.setMask(projectConfig.getBlockElevationMask());

    // Some settings may need to be reevaluated based on the layout
    /*uint16_t metatileId = projectConfig.getNewMapMetatileId();
    if (bitsMetatileId.clamp(metatileId) != metatileId)
        projectConfig.setNewMapMetatileId(bitsMetatileId.clamp(metatileId));
    uint16_t metatileId = projectConfig.getNewMapMetatileId();
    if (bitsMetatileId.clamp(metatileId) != metatileId)
        projectConfig.setNewMapMetatileId(bitsMetatileId.clamp(metatileId));*/
    
}

bool Block::operator ==(Block other) const {
    return (m_metatileId == other.m_metatileId)
        && (m_collision == other.m_collision)
        && (m_elevation == other.m_elevation);
}

bool Block::operator !=(Block other) const {
    return !(operator ==(other));
}

void Block::setMetatileId(uint16_t metatileId) {
    m_metatileId = bitsMetatileId.clamp(metatileId);
}

void Block::setCollision(uint16_t collision) {
    m_collision = bitsCollision.clamp(collision);
}

void Block::setElevation(uint16_t elevation) {
    m_elevation = bitsElevation.clamp(elevation);
}

uint16_t Block::getMaxMetatileId() {
    return bitsMetatileId.maxValue();
}

uint16_t Block::getMaxCollision() {
    return bitsCollision.maxValue();
}

uint16_t Block::getMaxElevation() {
    return bitsElevation.maxValue();
}
