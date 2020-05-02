#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include "overlay.h"
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
    Overlay overlay;
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void drawForeground(QPainter *painter, const QRectF &rect);
};

//Q_DECLARE_METATYPE(GraphicsView)

#endif // GRAPHICSVIEW_H
