#ifndef CONNECTIONPIXMAPITEM_H
#define CONNECTIONPIXMAPITEM_H

#include "mapconnection.h"
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QPointer>

class ConnectionPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ConnectionPixmapItem(MapConnection* connection, int originX, int originY);
    ConnectionPixmapItem(MapConnection* connection, QPoint origin);

    const QPointer<MapConnection> connection;

    void setOrigin(int x, int y);
    void setOrigin(QPoint pos);

    void setEditable(bool editable);
    bool getEditable();

    void setSelected(bool selected);

    void updatePos();
    void render(bool ignoreCache = false);

private:
    QPixmap basePixmap;
    qreal originX;
    qreal originY;
    bool selected = false;
    unsigned actionId = 0;

    static const int mWidth = 16;
    static const int mHeight = 16;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;

signals:
    void connectionItemDoubleClicked(MapConnection*);
    void selectionChanged(bool selected);
};

#endif // CONNECTIONPIXMAPITEM_H
