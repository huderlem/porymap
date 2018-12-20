#ifndef HEALLOCATION_H
#define HEALLOCATION_H

#include "event.h"
#include <QString>
#include <QDebug>

class HealLocation {

public:
    HealLocation()=default;
    HealLocation(QString, int, uint16_t, uint16_t);
    friend QDebug operator<<(QDebug debug, const HealLocation &hl);

public:
    QString name;
    int     index;
    uint16_t  x;
    uint16_t  y;
    static HealLocation fromEvent(Event*);
};

#endif // HEALLOCATION_H
