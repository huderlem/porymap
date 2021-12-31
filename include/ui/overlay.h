#ifndef OVERLAY_H
#define OVERLAY_H

#include <QList>
#include <QString>
#include <QColor>
#include <QPainter>

class OverlayItem {
public:
    OverlayItem() {}
    virtual ~OverlayItem() {};
    virtual void render(QPainter *, int, int) {};
};

class OverlayText : public OverlayItem {
public:
    OverlayText(QString text, int x, int y, QColor color, int fontSize) {
        this->text = text;
        this->x = x;
        this->y = y;
        this->color = color;
        this->fontSize = fontSize;
    }
    ~OverlayText() {}
    virtual void render(QPainter *painter, int x, int y);
private:
    QString text;
    int x;
    int y;
    QColor color;
    int fontSize;
};

class OverlayRect : public OverlayItem {
public:
    OverlayRect(int x, int y, int width, int height, QColor color, bool filled) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->color = color;
        this->filled = filled;
    }
    ~OverlayRect() {}
    virtual void render(QPainter *painter, int x, int y);
private:
    int x;
    int y;
    int width;
    int height;
    QColor color;
    bool filled;
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
    void setPosition(int x, int y);
    void move(int deltaX, int deltaY);
    void renderItems(QPainter *painter);
    QList<OverlayItem*> getItems();
    void clearItems();
    void addText(QString text, int x, int y, QString color = "#000000", int fontSize = 12);
    void addRect(int x, int y, int width, int height, QString color = "#000000", bool filled = false);
    bool addImage(int x, int y, QString filepath, bool useCache = true, int width = -1, int height = -1, unsigned offset = 0, bool xflip = false, bool yflip = false, QList<QRgb> palette = QList<QRgb>(), bool setTransparency = false);
    bool addImage(int x, int y, QImage image);
private:
    QList<OverlayItem*> items;
    int x;
    int y;
    bool hidden;
    qreal opacity;
};

#endif // OVERLAY_H
