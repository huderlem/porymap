#include "mapconnection.h"
#include "map.h"

const QStringList MapConnection::cardinalDirections = {
    "up", "down", "left", "right"
};

MapConnection MapConnection::mirror(const MapConnection &source, const QString &mapName) {
    static const QMap<QString, QString> oppositeDirections = {
        {"up", "down"}, {"down", "up"},
        {"right", "left"}, {"left", "right"},
        {"dive", "emerge"}, {"emerge", "dive"}
    };

    // TODO: Allowing editing unknown directions is a can of worms.
    //       Specifically a self-connection with an empty direction and an offset of 0 can be identified as its own mirror
    MapConnection mirror;
    mirror.direction = oppositeDirections.value(source.direction/*, source.direction*/);
    mirror.map_name = mapName;
    mirror.offset = -source.offset;

    return mirror;
}

bool MapConnection::isHorizontal(const QString &direction) {
    return direction == "left" || direction == "right";
}

bool MapConnection::isVertical(const QString &direction) {
    return direction == "up" || direction == "down";
}

bool MapConnection::isCardinal(const QString &direction) {
    return cardinalDirections.contains(direction);
}
