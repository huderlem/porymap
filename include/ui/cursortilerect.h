#ifndef CURSORTILERECT_H
#define CURSORTILERECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRgb>

class CursorTileRect : public QGraphicsItem
{
public:
    CursorTileRect(bool *enabled, QRgb color);
    QRectF boundingRect() const override
    {
        int width = this->width;
        int height = this->height;
        if (this->singleTileMode) {
            width = 16;
            height = 16;
        } else if (!this->rightClickSelectionAnchored && this->smartPathMode && this->selectionHeight == 3 && this->selectionWidth == 3) {
            width = 32;
            height = 32;
        }
        qreal penWidth = 4;
        return QRectF(-penWidth,
                      -penWidth,
                      width + penWidth * 2,
                      height + penWidth * 2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        if (!(*enabled)) return;
        int width = this->width;
        int height = this->height;
        if (this->singleTileMode) {
            width = 16;
            height = 16;
        } else if (this->smartPathInEffect()) {
            width = 32;
            height = 32;
        }

        painter->setPen(this->color);
        painter->drawRect(x() - 1, y() - 1, width + 2, height + 2);
        painter->setPen(QColor(0, 0, 0));
        painter->drawRect(x() - 2, y() - 2, width + 4, height + 4);
        painter->drawRect(x(), y(), width, height);
    }
    void initAnchor(int coordX, int coordY);
    void stopAnchor();
    void initRightClickSelectionAnchor(int coordX, int coordY);
    void stopRightClickSelectionAnchor();
    void setSmartPathMode(bool enable);
    bool getSmartPathMode() { return this->smartPathMode; }
    void setStraightPathMode(bool enable);
    bool getStraightPathMode() { return this->straightPathMode; }
    void setSingleTileMode();
    void stopSingleTileMode();
    void updateLocation(int x, int y);
    void updateSelectionSize(int width, int height);
    void setActive(bool active);
    bool getActive();
    bool *enabled;
private:
    bool active;
    int width;
    int height;
    bool anchored;
    bool rightClickSelectionAnchored;
    bool smartPathMode;
    bool straightPathMode;
    bool singleTileMode;
    int anchorCoordX;
    int anchorCoordY;
    int selectionWidth;
    int selectionHeight;
    QRgb color;
    bool smartPathInEffect();
};


#endif // CURSORTILERECT_H
