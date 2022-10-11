#ifndef OVERLAY_H
#define OVERLAY_H

#include <QList>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QStaticText>

class OverlayItem {
public:
    OverlayItem() {}
    virtual ~OverlayItem() {};
    virtual void render(QPainter *, int, int) {};
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
    virtual void render(QPainter *painter, int x, int y);
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
        this->prevX = 0;
        this->prevY = 0;
        this->path = path;
        this->borderColor = borderColor;
        this->fillColor = fillColor;
    }
    ~OverlayPath() {}
    virtual void render(QPainter *painter, int x, int y);
private:
    int prevX;
    int prevY;
    QPainterPath path;
    QColor borderColor;
    QColor fillColor;
};

class OverlayImage : public OverlayItem {
public:
    OverlayImage(int x, int y, QImage image) {
        this->x = x;
        this->y = y;
        this->image = image;
    }
    ~OverlayImage() {}
    virtual void render(QPainter *painter, int x, int y);
private:
    int x;
    int y;
    QImage image;
};

class Overlay
{
public:
    Overlay() {
        this->x = 0;
        this->y = 0;
        this->hidden = false;
        this->opacity = 1.0;
        this->clippingRect = nullptr;
    }
    ~Overlay() {
        this->clearItems();
    }
    bool getHidden();
    void setHidden(bool hidden);
    int getOpacity();
    void setOpacity(int opacity);
    int getX();
    int getY();
    void setX(int x);
    void setY(int y);
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
    bool addPath(QList<int> x, QList<int> y, QString borderColorStr, QString fillColorStr);
private:
    QColor getColor(QString colorStr);
    QList<OverlayItem*> items;
    int x;
    int y;
    bool hidden;
    qreal opacity;
    QRectF *clippingRect;
};

#endif // OVERLAY_H
