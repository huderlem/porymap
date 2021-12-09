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
    Overlay * getOverlay(int layer);
    void clearOverlays();
    void setOverlaysHidden(bool hidden);
    void setOverlaysX(int x);
    void setOverlaysY(int y);
    void setOverlaysPosition(int x, int y);
    void moveOverlays(int deltaX, int deltaY);

public:
//    GraphicsView_Object object;
    Editor *editor;
    QMap<int, Overlay*> overlayMap;
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void drawForeground(QPainter *painter, const QRectF &rect);
    void moveEvent(QMoveEvent *event);
};

//Q_DECLARE_METATYPE(GraphicsView)

#endif // GRAPHICSVIEW_H
