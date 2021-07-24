#ifndef DRAGGABLEPIXMAPITEM_H
#define DRAGGABLEPIXMAPITEM_H

#include <QString>
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemAnimation>

#include <QtWidgets>

#include "event.h"

class Editor;

class DraggablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {}
    
    DraggablePixmapItem(Event *event_, Editor *editor_) : QGraphicsPixmapItem(event_->pixmap) {
        event = event_;
        event->setPixmapItem(this);
        editor = editor_;
        updatePosition();
    }

    Editor *editor = nullptr;
    Event *event = nullptr;
    QGraphicsItemAnimation *pos_anim = nullptr;

    bool active;
    int last_x;
    int last_y;

    void updatePosition();
    void move(int dx, int dy);
    void moveTo(const QPoint &pos);
    void emitPositionChanged();
    void updatePixmap();
    void bind(QComboBox *combo, QString key);
    void bindToUserData(QComboBox *combo, QString key);

signals:
    void positionChanged(Event *event);
    void xChanged(int);
    void yChanged(int);
    void elevationChanged(int);
    void spriteChanged(QPixmap pixmap);
    void onPropertyChanged(QString key, QString value);

public slots:
    void set_x(const QString &text) {
        event->put("x", text);
        updatePosition();
    }
    void set_y(const QString &text) {
        event->put("y", text);
        updatePosition();
    }
    void set_elevation(const QString &text) {
        event->put("elevation", text);
        updatePosition();
    }
    void set_sprite(const QString &text) {
        event->put("sprite", text);
        updatePixmap();
    }
    void set_script(const QString &text) {
        event->put("script_label", text);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
};

#endif // DRAGGABLEPIXMAPITEM_H
