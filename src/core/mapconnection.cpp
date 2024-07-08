#include "mapconnection.h"
#include "map.h"

void MapConnection::setDirection(const QString & direction) {
    if (direction == m_direction)
        return;
    auto before = m_direction;
    m_direction = direction;
    emit directionChanged(before, m_direction);
}

void MapConnection::setOffset(int offset) {
    if (offset == m_offset)
        return;
    auto before = m_offset;
    m_offset = offset;
    emit offsetChanged(before, m_offset);
}

void MapConnection::setMapName(const QString &mapName) {
    if (mapName == m_mapName)
        return;
    auto before = m_mapName;
    m_mapName = mapName;
    emit mapNameChanged(before, m_mapName);
}
/*
static QString MapConnection::oppositeDirection(const QString &direction) {
    static const QMap<QString, QString> oppositeDirections = {
        {"up", "down"}, {"down", "up"},
        {"right", "left"}, {"left", "right"},
        {"dive", "emerge"}, {"emerge", "dive"}
    };
    return oppositeDirections.value(direction);
}

static MapConnection* MapConnection::getMirror(const MapConnection*, const Map*) {

}

static MapConnection* MapConnection::newMirror(const MapConnection*) {

}*/
