#ifndef EVENTPIXMAPITEM_H
#define EVENTPIXMAPITEM_H

#include <QString>
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemAnimation>

#include <QtWidgets>

#include "events.h"

class Project;

class EventPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    explicit EventPixmapItem(Event *event);

    void render(Project *project);

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

    Event * getEvent() const { return m_event; }

    void move(int dx, int dy);
    void moveTo(int x, int y);
    void moveTo(const QPoint &pos);

private:
    QPixmap m_basePixmap;
    Event *const m_event = nullptr;
    QPoint m_lastPos;
    bool m_active = false;
    bool m_selected = false;
    bool m_releaseSelectionQueued = false;

    void updatePixelPosition();

signals:
    void xChanged(int x);
    void yChanged(int y);
    void posChanged(int x, int y);
    void rendered(const QPixmap &pixmap);
    void selected(Event *event, bool toggle);
    void dragged(Event *event, const QPoint &oldPosition, const QPoint &newPosition);
    void released(Event *event, const QPoint &position);
    void doubleClicked(Event *event);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override { emit doubleClicked(m_event); }
};

#endif // EVENTPIXMAPITEM_H
