#include "blockdata.h"
#include <QDebug>

Blockdata::Blockdata(QObject *parent) : QObject(parent)
{
    blocks = new QList<Block>;
}

void Blockdata::addBlock(uint16_t word) {
    Block block(word);
    blocks->append(block);
}

void Blockdata::addBlock(Block block) {
    blocks->append(block);
}

QByteArray Blockdata::serialize() {
    QByteArray data;
    for (int i = 0; i < blocks->length(); i++) {
        Block block = blocks->value(i);
        uint16_t word = block.rawValue();
        data.append(static_cast<char>(word & 0xff));
        data.append(static_cast<char>((word >> 8) & 0xff));
    }
    return data;
}

void Blockdata::copyFrom(Blockdata* other) {
    blocks->clear();
    for (int i = 0; i < other->blocks->length(); i++) {
        addBlock(other->blocks->value(i));
    }
}

Blockdata* Blockdata::copy() {
    Blockdata* blockdata = new Blockdata;
    blockdata->copyFrom(this);
    return blockdata;
}

bool Blockdata::equals(Blockdata *other) {
    if (!other) {
        return false;
    }
    if (blocks->length() != other->blocks->length()) {
        return false;
    }
    for (int i = 0; i < blocks->length(); i++) {
        if (blocks->value(i) != other->blocks->value(i)) {
            return false;
        }
    }
    return true;
}
