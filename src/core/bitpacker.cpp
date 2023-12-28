#include "bitpacker.h"
#include <limits>

// Sometimes we can't explicitly define bitfields because we need to allow users to
// change the size and arrangement of its members. In those cases we use this
// convenience class to handle packing and unpacking each member.

BitPacker::BitPacker(uint32_t mask) {
    this->setMask(mask);
}

void BitPacker::setMask(uint32_t mask) {
    m_mask = mask;

    // Precalculate the number and positions of the mask bits
    m_setBits.clear();
    for (int i = 0; mask != 0; mask >>= 1, i++)
        if (mask & 1) m_setBits.append(1 << i);

    // For masks with only contiguous bits m_maxValue is equivalent to (m_mask >> n), where n is the number of trailing 0's in m_mask.
    m_maxValue = (m_setBits.length() >= 32) ? UINT_MAX : ((1 << m_setBits.length()) - 1);
}

// Given an arbitrary value to set for this bitfield member, returns a (potentially truncated) value that can later be packed losslessly.
uint32_t BitPacker::clamp(uint32_t value) const {
    return (m_maxValue == UINT_MAX) ? value : (value % (m_maxValue + 1));
}

// Given packed data, returns the extracted value for the bitfield member.
// For masks with only contiguous bits this is equivalent to ((data & m_mask) >> n), where n is the number of trailing 0's in m_mask.
uint32_t BitPacker::unpack(uint32_t data) const {
    uint32_t value = 0;
    data &= m_mask;
    for (int i = 0; i < m_setBits.length(); i++) {
        if (data & m_setBits.at(i))
            value |= (1 << i);
    }
    return value;
}

// Given a value for the bitfield member, returns the value to OR together with the other members.
// For masks with only contiguous bits this is equivalent to ((value << n) & m_mask), where n is the number of trailing 0's in m_mask.
uint32_t BitPacker::pack(uint32_t value) const {
    uint32_t data = 0;
    for (int i = 0; i < m_setBits.length(); i++) {
        if (value == 0) return data;
        if (value & 1) data |= m_setBits.at(i);
        value >>= 1;
    }
    return data;
}
