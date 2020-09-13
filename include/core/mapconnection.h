#pragma once
#ifndef MAPCONNECTION_H
#define MAPCONNECTION_H

#include <QString>
#include <QHash>

class MapConnection {
public:
    QString direction;
    QString offset;
    QString map_name;
};

inline bool operator==(const MapConnection &c1, const MapConnection &c2) {
    return c1.map_name == c2.map_name;
}

inline uint qHash(const MapConnection &key) {
    return qHash(key.map_name);
}

#endif // MAPCONNECTION_H
