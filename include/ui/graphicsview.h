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

class Editor;

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView() : QGraphicsView() {}
    GraphicsView(QWidget *parent) : QGraphicsView(parent) {}

public:
//    GraphicsView_Object object;
    Editor *editor;
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void moveEvent(QMoveEvent *event);
};

//Q_DECLARE_METATYPE(GraphicsView)

#endif // GRAPHICSVIEW_H
