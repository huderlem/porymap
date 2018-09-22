#include "heallocation.h"

HealLocation::HealLocation(QString map, int i, uint16_t x, uint16_t y)
{
    this->name = map;
    this->index = i;
    this->x = x;
    this->y = y;
}

QDebug operator<<(QDebug debug, const HealLocation &hl)
{
    debug << "HealLocation_" + hl.name << "(" << hl.x << ',' << hl.y << ")";
    return debug;
}
