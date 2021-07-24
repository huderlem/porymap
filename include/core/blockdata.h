#pragma once
#ifndef BLOCKDATA_H
#define BLOCKDATA_H

#include "block.h"

#include <QByteArray>
#include <QVector>

class Blockdata : public QVector<Block>
{
public:
    QByteArray serialize() const;
};

#endif // BLOCKDATA_H
