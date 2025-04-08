#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>

class NoScrollGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    NoScrollGraphicsView(QWidget *parent = nullptr) : QGraphicsView(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) {
        event->ignore();
    }
};

class ClickableGraphicsView : public NoScrollGraphicsView
{
    Q_OBJECT
public:
    ClickableGraphicsView(QWidget *parent = nullptr) : NoScrollGraphicsView(parent) {}

public:
    void mouseReleaseEvent(QMouseEvent *event) override {
        QGraphicsView::mouseReleaseEvent(event);
        emit this->clicked(event);
    }

signals:
    void clicked(QMouseEvent *event);
};

#endif // GRAPHICSVIEW_H
