#include "mapconnection.h"
#include "project.h"

QPointer<Project> MapConnection::project = nullptr;

const QMap<QString, QString> MapConnection::oppositeDirections = {
    {"up", "down"}, {"down", "up"},
    {"right", "left"}, {"left", "right"},
    {"dive", "emerge"}, {"emerge", "dive"}
};

MapConnection::MapConnection(const QString &targetMapName, const QString &direction, int offset) {
    m_parentMap = nullptr;
    m_targetMapName = targetMapName;
    m_direction = direction;
    m_offset = offset;
}

bool MapConnection::areMirrored(const MapConnection* a, const MapConnection* b) {
    if (!a || !b || !a->m_parentMap || !b->m_parentMap)
        return false;

    return a->parentMapName() == b->m_targetMapName
        && a->m_targetMapName == b->parentMapName()
        && a->m_offset == -b->m_offset
        && a->m_direction == oppositeDirection(b->m_direction);
}

MapConnection* MapConnection::findMirror() {
    auto map = targetMap();
    if (!map)
        return nullptr;

    // Find the matching connection in the connected map.
    // Note: There is no strict source -> mirror pairing, i.e. we are not guaranteed
    // to always get the same MapConnection if there are multiple identical copies.
    for (auto connection : map->getConnections()) {
        if (this != connection && areMirrored(this, connection))
            return connection;
    }
    return nullptr;
}

MapConnection * MapConnection::createMirror() {
    auto mirror = new MapConnection(parentMapName(), oppositeDirection(m_direction), -m_offset);
    mirror->setParentMap(targetMap(), false);
    return mirror;
}

void MapConnection::markMapEdited() {
    if (m_parentMap)
        m_parentMap->modify();
}

Map* MapConnection::getMap(const QString& mapName) const {
    return project ? project->getMap(mapName) : nullptr;
}

Map* MapConnection::targetMap() const {
    return getMap(m_targetMapName);
}

QPixmap MapConnection::getPixmap() {
    auto map = targetMap();
    if (!map)
        return QPixmap();

    return map->renderConnection(m_direction, m_parentMap ? m_parentMap->layout() : nullptr);
}

void MapConnection::setParentMap(Map* map, bool mirror) {
    if (map == m_parentMap)
        return;

    if (mirror) {
        auto connection = findMirror();
        if (connection)
            connection->setTargetMapName(map ? map->name() : QString(), false);
    }

    if (m_parentMap)
        m_parentMap->removeConnection(this);

    auto before = m_parentMap;
    m_parentMap = map;

    if (m_parentMap)
        m_parentMap->addConnection(this);

    emit parentMapChanged(before, m_parentMap);
}

QString MapConnection::parentMapName() const {
    return m_parentMap ? m_parentMap->name() : QString();
}

void MapConnection::setTargetMapName(const QString &targetMapName, bool mirror) {
    if (targetMapName == m_targetMapName)
        return;

    if (mirror) {
        auto connection = findMirror();
        if (connection)
            connection->setParentMap(getMap(targetMapName), false);
    }

    auto before = m_targetMapName;
    m_targetMapName = targetMapName;
    emit targetMapNameChanged(before, m_targetMapName);
    markMapEdited();
}

void MapConnection::setDirection(const QString &direction, bool mirror) {
    if (direction == m_direction)
        return;

    if (mirror) {
        auto connection = findMirror();
        if (connection)
            connection->setDirection(oppositeDirection(direction), false);
    }

    auto before = m_direction;
    m_direction = direction;
    emit directionChanged(before, m_direction);
    markMapEdited();
}

void MapConnection::setOffset(int offset, bool mirror) {
    if (offset == m_offset)
        return;

    if (mirror) {
        auto connection = findMirror();
        if (connection)
            connection->setOffset(-offset, false);
    }

    auto before = m_offset;
    m_offset = offset;
    emit offsetChanged(before, m_offset);
    markMapEdited();
}

const QStringList MapConnection::cardinalDirections = {
    "up", "down", "left", "right"
};

bool MapConnection::isCardinal(const QString &direction) {
    return cardinalDirections.contains(direction);
}

bool MapConnection::isHorizontal(const QString &direction) {
    return direction == "left" || direction == "right";
}

bool MapConnection::isVertical(const QString &direction) {
    return direction == "up" || direction == "down";
}

bool MapConnection::isDiving(const QString &direction) {
    return direction == "dive" || direction == "emerge";
}
