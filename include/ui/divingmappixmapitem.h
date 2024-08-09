#ifndef DIVINGMAPPIXMAPITEM_H
#define DIVINGMAPPIXMAPITEM_H

#include "mapconnection.h"

#include <QGraphicsPixmapItem>
#include <QPointer>

class DivingMapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DivingMapPixmapItem(MapConnection* connection)
    : QGraphicsPixmapItem(getBasePixmap(connection))
    {
        m_connection = connection;
        setZValue(2);

        // Update pixmap if the connected map is swapped.
        connect(m_connection, &MapConnection::targetMapNameChanged, this, &DivingMapPixmapItem::updatePixmap);
    }
    MapConnection* connection() const { return m_connection; }

private:
    QPointer<MapConnection> m_connection;

    static QPixmap getBasePixmap(MapConnection* connection) {
        // If the map is connected to itself then rendering is pointless.
        if (!connection || connection->targetMapName() == connection->parentMapName())
            return QPixmap();
        return connection->getPixmap();
    }
    void updatePixmap() { setPixmap(getBasePixmap(m_connection)); }
};

#endif // DIVINGMAPPIXMAPITEM_H
