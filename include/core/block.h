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
    uint16_t metatileId:10;
    uint16_t collision:2;
    uint16_t elevation:4;
    uint16_t rawValue() const;
};

#endif // BLOCK_H
