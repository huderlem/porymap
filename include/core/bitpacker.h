#ifndef BITPACKER_H
#define BITPACKER_H

#include <QList>

class BitPacker
{
public:
    BitPacker() = default;
    BitPacker(uint32_t mask);

public:
    void setMask(uint32_t mask);
    uint32_t mask() const { return m_mask; }
    uint32_t maxValue() const { return m_maxValue; }

    uint32_t unpack(uint32_t data) const;
    uint32_t pack(uint32_t value) const;
    uint32_t clamp(uint32_t value) const;

private:
    uint32_t m_mask = 0;
    uint32_t m_maxValue = 0;
    QList<uint32_t> m_setBits;
};

#endif // BITPACKER_H
