#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "graphicsview.h"
#include "overlay.h"

class MapView : public GraphicsView
{
public:
    MapView() : GraphicsView() {}
    MapView(QWidget *parent) : GraphicsView(parent) {}
    Overlay * getOverlay(int layer);
    void clearOverlays();
    void setOverlaysHidden(bool hidden);
    void setOverlaysX(int x);
    void setOverlaysY(int y);
    void setOverlaysPosition(int x, int y);
    void moveOverlays(int deltaX, int deltaY);
    void setOverlaysOpacity(int opacity);

public:
    QMap<int, Overlay*> overlayMap;
protected:
    void drawForeground(QPainter *painter, const QRectF &rect);
};

#endif // GRAPHICSVIEW_H
