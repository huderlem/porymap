#include "heallocation.h"

HealLocation::HealLocation(QString map, int i, uint16_t x, uint16_t y)
{
    this->name = map;
    this->index = i;
    this->x = x;
    this->y = y;
}

HealLocation HealLocation::fromEvent(Event *event)
{
    HealLocation hl;
    hl.name  = event->get("loc_name");
    try {
        hl.index = event->get("index").toInt();
    }
    catch(...) {
        hl.index = 0;
    }
    hl.x     = event->getU16("x");
    hl.y     = event->getU16("y");
    return hl;
}

QDebug operator<<(QDebug debug, const HealLocation &hl)
{
    debug << "HealLocation_" + hl.name << "(" << hl.x << ',' << hl.y << ")";
    return debug;
}
