#pragma once
#ifndef BLOCK_H
#define BLOCK_H

#include <QObject>

class Block
{
public:
    Block();
    Block(uint16_t);
    Block(uint16_t metatileId, uint16_t collision, uint16_t elevation);
    Block(const Block &);
    Block &operator=(const Block &);
    bool operator ==(Block) const;
    bool operator !=(Block) const;
    void setMetatileId(uint16_t metatileId);
    void setCollision(uint16_t collision);
    void setElevation(uint16_t elevation);
    uint16_t metatileId() const { return m_metatileId; }
    uint16_t collision() const { return m_collision; }
    uint16_t elevation() const { return m_elevation; }
    uint16_t rawValue() const;
    static void setLayout();
    static uint16_t getMaxMetatileId();
    static uint16_t getMaxCollision();
    static uint16_t getMaxElevation();

private:
    uint16_t m_metatileId; // 10
    uint16_t m_collision;  // 2
    uint16_t m_elevation;  // 4
};

#endif // BLOCK_H
