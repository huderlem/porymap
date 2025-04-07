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
    void spriteChanged(const QPixmap &pixmap);
    void doubleClicked(Event *event);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override { emit doubleClicked(this->event); }
};

#endif // DRAGGABLEPIXMAPITEM_H
