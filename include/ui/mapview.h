#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QJSValue>
#include "graphicsview.h"
#include "overlay.h"

class MapView : public GraphicsView
{
    Q_OBJECT

public:
    MapView() : GraphicsView() {}
    MapView(QWidget *parent) : GraphicsView(parent) {}

    Overlay * getOverlay(int layer);
    void clearOverlayMap();

    // Overlay scripting API
    Q_INVOKABLE void clear(int layer);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void hide(int layer);
    Q_INVOKABLE void hide();
    Q_INVOKABLE void show(int layer);
    Q_INVOKABLE void show();
    Q_INVOKABLE bool getVisibility(int layer = 0);
    Q_INVOKABLE void setVisibility(bool visible, int layer);
    Q_INVOKABLE void setVisibility(bool visible);
    Q_INVOKABLE int getX(int layer = 0);
    Q_INVOKABLE int getY(int layer = 0);
    Q_INVOKABLE void setX(int x, int layer);
    Q_INVOKABLE void setX(int x);
    Q_INVOKABLE void setY(int y, int layer);
    Q_INVOKABLE void setY(int y);
    Q_INVOKABLE void setClippingRect(int x, int y, int width, int height, int layer);
    Q_INVOKABLE void setClippingRect(int x, int y, int width, int height);
    Q_INVOKABLE void clearClippingRect(int layer);
    Q_INVOKABLE void clearClippingRect();
    Q_INVOKABLE QJSValue getPosition(int layer = 0);
    Q_INVOKABLE void setPosition(int x, int y, int layer);
    Q_INVOKABLE void setPosition(int x, int y);
    Q_INVOKABLE void move(int deltaX, int deltaY, int layer);
    Q_INVOKABLE void move(int deltaX, int deltaY);
    Q_INVOKABLE int getOpacity(int layer = 0);
    Q_INVOKABLE void setOpacity(int opacity, int layer);
    Q_INVOKABLE void setOpacity(int opacity);
    Q_INVOKABLE void addText(QString text, int x, int y, QString color = "#000000", int fontSize = 12, int layer = 0);
    Q_INVOKABLE void addRect(int x, int y, int width, int height, QString color = "#000000", int layer = 0);
    Q_INVOKABLE void addFilledRect(int x, int y, int width, int height, QString color = "#000000", int layer = 0);
    Q_INVOKABLE void addImage(int x, int y, QString filepath, int layer = 0, bool useCache = true);
    Q_INVOKABLE void createImage(int x, int y, QString filepath,
                                 int width = -1, int height = -1, int xOffset = 0, int yOffset = 0,
                                 qreal hScale = 1, qreal vScale = 1, int paletteId = -1, bool setTransparency = false,
                                 int layer = 0, bool useCache = true);
    Q_INVOKABLE void addTileImage(int x, int y, int tileId, bool xflip, bool yflip, int paletteId, bool setTransparency = false, int layer = 0);
    Q_INVOKABLE void addTileImage(int x, int y, QJSValue tileObj, bool setTransparency = false, int layer = 0);
    Q_INVOKABLE void addMetatileImage(int x, int y, int metatileId, bool setTransparency = false, int layer = 0);

private:
    QMap<int, Overlay*> overlayMap;
protected:
    void drawForeground(QPainter *painter, const QRectF &rect);
};

#endif // GRAPHICSVIEW_H
