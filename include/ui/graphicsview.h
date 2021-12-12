#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>

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
