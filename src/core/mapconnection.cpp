#include "mapconnection.h"
#include "project.h"

QPointer<Project> MapConnection::project = nullptr;

const QMap<QString, QString> MapConnection::oppositeDirections = {
    {"up", "down"}, {"down", "up"},
    {"right", "left"}, {"left", "right"},
    {"dive", "emerge"}, {"emerge", "dive"}
};

MapConnection::MapConnection(const QString &direction, const QString &hostMapName, const QString &targetMapName, int offset) {
    m_direction = direction;
    m_hostMapName = hostMapName;
    m_targetMapName = targetMapName;
    m_offset = offset;
    m_ignoreMirror = false;
}

bool MapConnection::isMirror(const MapConnection* other) {
    if (!other)
        return false;

    return m_hostMapName == other->m_targetMapName
        && m_targetMapName == other->m_hostMapName
        && m_offset == -other->m_offset
        && m_direction == oppositeDirection(other->m_direction);
}

MapConnection* MapConnection::findMirror() {
    if (!porymapConfig.mirrorConnectingMaps || !project || m_ignoreMirror)
        return nullptr;

    Map *connectedMap = project->getMap(m_targetMapName);
    if (!connectedMap)
        return nullptr;

    // Find the matching connection in the connected map.
    // Note: There is no strict source -> mirror pairing, i.e. we are not guaranteed
    // to always get the same MapConnection if there are multiple identical copies.
    for (auto connection : connectedMap->connections) {
        if (this != connection && this->isMirror(connection))
            return connection;
    }
    return nullptr;
}

MapConnection * MapConnection::createMirror() {
    if (!porymapConfig.mirrorConnectingMaps)
        return nullptr;
    return new MapConnection(oppositeDirection(m_direction), m_targetMapName, m_hostMapName, -m_offset);
}

void MapConnection::markMapEdited() {
    if (project) {
        Map * map = project->getMap(m_hostMapName);
        if (map) map->modify();
    }
}

QPixmap MapConnection::getPixmap() {
    if (!project)
        return QPixmap();

    Map *hostMap = project->getMap(m_hostMapName);
    Map *targetMap = project->getMap(m_targetMapName);
    if (!hostMap || !targetMap)
        return QPixmap();

    return targetMap->renderConnection(m_direction, hostMap->layout);
}

void MapConnection::setDirection(const QString &direction) {
    if (direction == m_direction)
        return;

    mirrorDirection(direction);
    auto before = m_direction;
    m_direction = direction;
    emit directionChanged(before, m_direction);
    markMapEdited();
}

void MapConnection::mirrorDirection(const QString &direction) {
    MapConnection * mirror = findMirror();
    if (!mirror)
        return;
    mirror->m_ignoreMirror = true;
    mirror->setDirection(oppositeDirection(direction));
    mirror->m_ignoreMirror = false;
}

void MapConnection::setHostMapName(const QString &hostMapName) {
    if (hostMapName == m_hostMapName)
        return;

    mirrorHostMapName(hostMapName);
    auto before = m_hostMapName;
    m_hostMapName = hostMapName;

    if (project) {
        // TODO: This is probably unexpected design, esp because creating a MapConnection doesn't insert to host map
        // Reassign connection to new host map
        Map * oldMap = project->getMap(before);
        Map * newMap = project->getMap(m_hostMapName);
        if (oldMap) oldMap->removeConnection(this);
        if (newMap) newMap->addConnection(this);
    }
    emit hostMapNameChanged(before, m_hostMapName);
}

void MapConnection::mirrorHostMapName(const QString &hostMapName) {
    MapConnection * mirror = findMirror();
    if (!mirror)
        return;
    mirror->m_ignoreMirror = true;
    mirror->setTargetMapName(hostMapName);
    mirror->m_ignoreMirror = false;
}

void MapConnection::setTargetMapName(const QString &targetMapName) {
    if (targetMapName == m_targetMapName)
        return;

    mirrorTargetMapName(targetMapName);
    auto before = m_targetMapName;
    m_targetMapName = targetMapName;
    emit targetMapNameChanged(before, m_targetMapName);
    markMapEdited();
}

void MapConnection::mirrorTargetMapName(const QString &targetMapName) {
    MapConnection * mirror = findMirror();
    if (!mirror)
        return;
    mirror->m_ignoreMirror = true;
    mirror->setHostMapName(targetMapName);
    mirror->m_ignoreMirror = false;
}

void MapConnection::setOffset(int offset) {
    if (offset == m_offset)
        return;

    mirrorOffset(offset);
    auto before = m_offset;
    m_offset = offset;
    emit offsetChanged(before, m_offset);
    markMapEdited();
}

void MapConnection::mirrorOffset(int offset) {
    MapConnection * mirror = findMirror();
    if (!mirror)
        return;
    mirror->m_ignoreMirror = true;
    mirror->setOffset(-offset);
    mirror->m_ignoreMirror = false;
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
