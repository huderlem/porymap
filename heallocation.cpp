#include "heallocation.h"

//HealLocation::HealLocation() {}

HealLocation::HealLocation(QString map, int i, size_t x0, size_t y0) {

    name  = map;
    index = i;
    x     = x0;
    y     = y0;

}

QDebug operator<<(QDebug debug, const HealLocation &hl) {

    debug << "HealLocation_" + hl.name << "(" << hl.x << ',' << hl.y << ")";
    return debug;
    
}
