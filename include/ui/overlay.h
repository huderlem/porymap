#ifndef OVERLAY_H
#define OVERLAY_H

#include <QList>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QStaticText>
#include <QPainterPath>

#ifdef QT_QML_LIB

class OverlayItem {
public:
    OverlayItem() {}
    virtual ~OverlayItem() {};
    virtual void render(QPainter *) {};
};

class OverlayText : public OverlayItem {
public:
    OverlayText(const QString inputText, int x, int y, QColor color, int fontSize) :
        text(QStaticText(inputText))
    {
        this->x = x;
        this->y = y;
        this->color = color;
        this->fontSize = fontSize;
    }
    ~OverlayText() {}
    virtual void render(QPainter *painter);
private:
    const QStaticText text;
    int x;
    int y;
    QColor color;
    int fontSize;
};

class OverlayPath : public OverlayItem {
public:
    OverlayPath(QPainterPath path, QColor borderColor, QColor fillColor) {
        this->path = path;
        this->borderColor = borderColor;
        this->fillColor = fillColor;
    }
    ~OverlayPath() {}
    virtual void render(QPainter *painter);
private:
    QPainterPath path;
    QColor borderColor;
    QColor fillColor;
};

class OverlayPixmap : public OverlayItem {
public:
    OverlayPixmap(int x, int y, QPixmap pixmap) {
        this->x = x;
        this->y = y;
        this->pixmap = pixmap;
    }
    ~OverlayPixmap() {}
    virtual void render(QPainter *painter);
private:
    int x;
    int y;
    QPixmap pixmap;
};

class Overlay
{
public:
    Overlay(QRect fromRect) : image(fromRect.size(), QImage::Format_ARGB32) {
        this->x = 0;
        this->y = 0;
        this->angle = 0;
        this->hScale = 1.0;
        this->vScale = 1.0;
        this->hidden = false;
        this->opacity = 1.0;
        this->clippingRect = nullptr;
        this->image.setOffset(fromRect.topLeft());
        this->image.fill(QColor(0, 0, 0, 0));
        this->valid = false;
    }
    ~Overlay() {
        this->clearItems();
    }
    void invalidate() { this->valid = false; }
    bool getHidden();
    void setHidden(bool hidden);
    int getOpacity();
    void setOpacity(int opacity);
    int getX();
    int getY();
    void setX(int x);
    void setY(int y);
    qreal getHScale();
    qreal getVScale();
    void setHScale(qreal scale);
    void setVScale(qreal scale);
    void setScale(qreal hScale, qreal vScale);
    int getRotation();
    void setRotation(int angle);
    void rotate(int degrees);
    void setClippingRect(QRectF rect);
    void clearClippingRect();
    void setPosition(int x, int y);
    void move(int deltaX, int deltaY);
    void renderItems(QPainter *painter);
    QList<OverlayItem*> getItems();
    void clearItems();
    void addText(const QString text, int x, int y, QString colorStr, int fontSize);
    bool addRect(int x, int y, int width, int height, QString borderColorStr, QString fillColorStr, int rounding);
    bool addImage(int x, int y, QString filepath, bool useCache = true, int width = -1, int height = -1, int xOffset = 0, int yOffset = 0, qreal hScale = 1, qreal vScale = 1, QList<QRgb> palette = QList<QRgb>(), bool setTransparency = false);
    bool addImage(int x, int y, QImage image);
    bool addPath(QList<int> xCoords, QList<int> yCoords, QString borderColorStr, QString fillColorStr);
private:
    void clampAngle();
    QColor getColor(QString colorStr);
    QList<OverlayItem*> items;
    int x;
    int y;
    int angle;
    qreal hScale;
    qreal vScale;
    bool hidden;
    qreal opacity;
    QRectF *clippingRect;
    QImage image;
    bool valid;
};

#else

class Overlay
{
public:
    Overlay() {}
    ~Overlay() {}

    void renderItems(QPainter *) {}
};

#endif // QT_QML_LIB

#endif // OVERLAY_H
