#ifndef CONNECTIONPIXMAPITEM_H
#define CONNECTIONPIXMAPITEM_H

#include "mapconnection.h"
#include <QGraphicsPixmapItem>
#include <QPainter>

class ConnectionPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ConnectionPixmapItem(QPixmap pixmap, MapConnection* connection, int x, int y, int baseMapWidth, int baseMapHeight)
    : QGraphicsPixmapItem(pixmap),
      connection(connection)
    {
        this->basePixmap = pixmap;
        setFlag(ItemIsMovable);
        setFlag(ItemSendsGeometryChanges);
        this->initialX = x;
        this->initialY = y;
        this->initialOffset = connection->offset;
        this->baseMapWidth = baseMapWidth;
        this->baseMapHeight = baseMapHeight;
    }
    QPixmap basePixmap;
    MapConnection* const connection;
    int initialX;
    int initialY;
    int initialOffset;
    int baseMapWidth;
    int baseMapHeight;
    void render(qreal opacity = 1);
    void setEditable(bool editable);
    bool getEditable();
    void updateHighlight(bool selected);

private:
    bool highlighted = false;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);

signals:
    void connectionItemSelected(ConnectionPixmapItem* connectionItem);
    void connectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void connectionMoved(MapConnection*);
    void highlightChanged(bool highlighted);
};

#endif // CONNECTIONPIXMAPITEM_H
