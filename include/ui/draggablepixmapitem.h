#ifndef DRAGGABLEPIXMAPITEM_H
#define DRAGGABLEPIXMAPITEM_H

#include <QString>
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemAnimation>

#include <QtWidgets>

#include "events.h"

class Editor;

class DraggablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {}
    
    DraggablePixmapItem(Event *event, Editor *editor) : QGraphicsPixmapItem(event->getPixmap()) {
        this->event = event;
        event->setPixmapItem(this);
        this->editor = editor;
        updatePosition();
    }

    Event *event = nullptr;

    void updatePosition();
    void move(int dx, int dy);
    void moveTo(const QPoint &pos);
    void emitPositionChanged();
    void updatePixmap();

private:
    Editor *editor = nullptr;
    QPoint lastPos;
    bool active = false;
    bool releaseSelectionQueued = false;

signals:
    void positionChanged(Event *event);
    void xChanged(int);
    void yChanged(int);
    void elevationChanged(int);
    void spriteChanged(QPixmap pixmap);
    void onPropertyChanged(QString key, QString value);

public slots:
    void set_x(int x) {
        event->setX(x);
        updatePosition();
    }
    void set_y(int y) {
        event->setY(y);
        updatePosition();
    }
    void set_elevation(int z) {
        event->setElevation(z);
        updatePosition();
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
};

#endif // DRAGGABLEPIXMAPITEM_H
