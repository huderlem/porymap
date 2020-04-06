#ifndef HEALLOCATION_H
#define HEALLOCATION_H

#include "event.h"
#include <QString>
#include <QDebug>

class HealLocation {

public:
    HealLocation()=default;
    HealLocation(QString, QString, int, uint16_t, uint16_t, QString = "", uint16_t = 0);
    friend QDebug operator<<(QDebug debug, const HealLocation &hl);

public:
    QString idName;
    QString mapName;
    int     index;
    uint16_t  x;
    uint16_t  y;
    QString respawnMap;
    uint16_t respawnNPC;
    static HealLocation fromEvent(Event*);
};

#endif // HEALLOCATION_H
