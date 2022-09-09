#pragma once
#ifndef HEALLOCATION_H
#define HEALLOCATION_H

#include <QString>
#include <QDebug>

class Event;

class HealLocation {

public:
    HealLocation()=default;
    HealLocation(QString, QString, int, int16_t, int16_t, QString = "", uint8_t = 0);
    friend QDebug operator<<(QDebug debug, const HealLocation &hl);

public:
    QString idName;
    QString mapName;
    int     index;
    int16_t  x;
    int16_t  y;
    QString respawnMap;
    uint8_t respawnNPC;
    static HealLocation fromEvent(Event *);
};

#endif // HEALLOCATION_H
