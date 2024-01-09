#include "heallocation.h"
#include "config.h"
#include "events.h"
#include "map.h"

HealLocation::HealLocation(QString id, QString map,
                           int i, int16_t x, int16_t y,
                           QString respawnMap, uint8_t respawnNPC) {
    this->idName = id;
    this->mapName = map;
    this->index = i;
    this->x = x;
    this->y = y;
    this->respawnMap = respawnMap;
    this->respawnNPC = respawnNPC;
}

HealLocation HealLocation::fromEvent(Event *fromEvent) {
    HealLocationEvent *event = dynamic_cast<HealLocationEvent *>(fromEvent);

    HealLocation healLocation;
    healLocation.idName  = event->getIdName();
    healLocation.mapName = event->getLocationName();
    healLocation.index   = event->getIndex();
    healLocation.x       = event->getX();
    healLocation.y       = event->getY();
    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        healLocation.respawnNPC = event->getRespawnNPC();
        healLocation.respawnMap = Map::mapConstantFromName(event->getRespawnMap(), false);
    }
    return healLocation;
}

QDebug operator<<(QDebug debug, const HealLocation &healLocation) {
    debug << "HealLocation_" + healLocation.mapName << "(" << healLocation.x << ',' << healLocation.y << ")";
    return debug;
}
