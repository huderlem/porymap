#ifndef CONNECTIONPIXMAPITEM_H
#define CONNECTIONPIXMAPITEM_H

#include "mapconnection.h"
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QPointer>
#include <QKeyEvent>

class ConnectionPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ConnectionPixmapItem(MapConnection* connection);

    const QPointer<MapConnection> connection;

    void setEditable(bool editable);
    bool getEditable();

    void setSelected(bool selected);

    void render(bool ignoreCache = false);

signals:
    void positionChanged(qreal x, qreal y);

private:
    QPixmap basePixmap;
    qreal originX;
    qreal originY;
    bool selected = false;
    unsigned actionId = 0;

    static const int mWidth = 16;
    static const int mHeight = 16;

    void updatePos();
    void updateOrigin();
    void refresh();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void focusInEvent(QFocusEvent*) override;

signals:
    void connectionItemDoubleClicked(MapConnection*);
    void selectionChanged(bool selected);
    void deleteRequested(MapConnection*);
};

#endif // CONNECTIONPIXMAPITEM_H
