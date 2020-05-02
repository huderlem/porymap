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
    virtual void render(QPainter *painter) {};
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
    virtual void render(QPainter *painter);
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
    virtual void render(QPainter *painter);
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
    virtual void render(QPainter *painter);
private:
    int x;
    int y;
    QImage image;
};

class Overlay
{
public:
    Overlay() {}
    ~Overlay() {
        this->clearItems();
    }
    QList<OverlayItem*> getItems();
    void clearItems();
    void addText(QString text, int x, int y, QString color = "#000000", int fontSize = 12);
    void addRect(int x, int y, int width, int height, QString color = "#000000", bool filled = false);
    void addImage(int x, int y, QString filepath);
private:
    QList<OverlayItem*> items;
};

#endif // OVERLAY_H
