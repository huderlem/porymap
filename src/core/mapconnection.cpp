#include <QMap>
#include "mapconnection.h"

const QMap<QString, QString> oppositeDirections = {
    {"up", "down"}, {"down", "up"},
    {"right", "left"}, {"left", "right"},
    {"dive", "emerge"}, {"emerge", "dive"}
};

MapConnection::MapConnection(const QString &direction, const QString &hostMapName, const QString &targetMapName, int offset) {
    m_direction = direction;
    m_hostMapName = hostMapName;
    m_targetMapName = targetMapName;
    m_offset = offset;
}

void MapConnection::setDirection(const QString &direction) {
    if (direction == m_direction)
        return;

    auto before = m_direction;
    m_direction = direction;
    emit directionChanged(before, m_direction);
}

void MapConnection::setHostMapName(const QString &hostMapName) {
    if (hostMapName == m_hostMapName)
        return;

    auto before = m_hostMapName;
    m_hostMapName = hostMapName;
    emit hostMapNameChanged(before, m_hostMapName);

}

void MapConnection::setTargetMapName(const QString &targetMapName) {
    if (targetMapName == m_targetMapName)
        return;

    auto before = m_targetMapName;
    m_targetMapName = targetMapName;
    emit targetMapNameChanged(before, m_targetMapName);
}

void MapConnection::setOffset(int offset) {
    if (offset == m_offset)
        return;

    auto before = m_offset;
    m_offset = offset;
    emit offsetChanged(before, m_offset);
}

MapConnection * MapConnection::createMirror() {
    return new MapConnection(oppositeDirections.value(m_direction, m_direction), m_targetMapName, m_hostMapName, -m_offset);
}

bool MapConnection::isMirror(const MapConnection* other) {
    if (!other)
        return false;

    return m_hostMapName == other->m_targetMapName
        && m_targetMapName == other->m_hostMapName
        && m_offset == -other->m_offset
        && m_direction == oppositeDirections.value(other->m_direction, other->m_direction);
}

const QStringList MapConnection::cardinalDirections = {
    "up", "down", "left", "right"
};

bool MapConnection::isHorizontal(const QString &direction) {
    return direction == "left" || direction == "right";
}

bool MapConnection::isVertical(const QString &direction) {
    return direction == "up" || direction == "down";
}

bool MapConnection::isCardinal(const QString &direction) {
    return cardinalDirections.contains(direction);
}
