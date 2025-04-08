#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QJSValue>
#include "graphicsview.h"
#include "overlay.h"

class Editor;

class MapView : public QGraphicsView
{
    Q_OBJECT

public:
    MapView() : QGraphicsView() {}
    MapView(QWidget *parent) : QGraphicsView(parent) {}

    Editor *editor;

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
    Q_INVOKABLE qreal getHorizontalScale(int layer = 0);
    Q_INVOKABLE qreal getVerticalScale(int layer = 0);
    Q_INVOKABLE void setHorizontalScale(qreal scale, int layer);
    Q_INVOKABLE void setHorizontalScale(qreal scale);
    Q_INVOKABLE void setVerticalScale(qreal scale, int layer);
    Q_INVOKABLE void setVerticalScale(qreal scale);
    Q_INVOKABLE void setScale(qreal hScale, qreal vScale, int layer);
    Q_INVOKABLE void setScale(qreal hScale, qreal vScale);
    Q_INVOKABLE int getRotation(int layer = 0);
    Q_INVOKABLE void setRotation(int angle, int layer);
    Q_INVOKABLE void setRotation(int angle);
    Q_INVOKABLE void rotate(int degrees, int layer);
    Q_INVOKABLE void rotate(int degrees);
    Q_INVOKABLE void addText(QString text, int x, int y, QString color = "#000000", int fontSize = 12, int layer = 0);
    Q_INVOKABLE void addRect(int x, int y, int width, int height, QString borderColor = "#000000", QString fillColor = "transparent", int rounding = 0, int layer = 0);
    Q_INVOKABLE void addPath(QList<QList<int>> coords, QString borderColor = "#000000", QString fillColor = "transparent", int layer = 0);
    Q_INVOKABLE void addPath(QList<int> xCoords, QList<int> yCoords, QString borderColor = "#000000", QString fillColor = "transparent", int layer = 0);
    Q_INVOKABLE void addImage(int x, int y, QString filepath, int layer = 0, bool useCache = true);
    Q_INVOKABLE void createImage(int x, int y, QString filepath,
                                 int width = -1, int height = -1, int xOffset = 0, int yOffset = 0,
                                 qreal hScale = 1, qreal vScale = 1, int paletteId = -1, bool setTransparency = false,
                                 int layer = 0, bool useCache = true);
    Q_INVOKABLE void addTileImage(int x, int y, int tileId, bool xflip, bool yflip, int paletteId, bool setTransparency = false, int layer = 0);
    Q_INVOKABLE void addTileImage(int x, int y, QJSValue tileObj, bool setTransparency = false, int layer = 0);
    Q_INVOKABLE void addMetatileImage(int x, int y, int metatileId, bool setTransparency = false, int layer = 0);

protected:
    virtual void drawForeground(QPainter *painter, const QRectF &rect) override;
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void moveEvent(QMoveEvent *event) override;
private:
    QMap<int, Overlay*> overlayMap;

    void updateScene();
};

#endif // GRAPHICSVIEW_H
