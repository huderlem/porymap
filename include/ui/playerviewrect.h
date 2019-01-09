#ifndef PLAYERVIEWRECT_H
#define PLAYERVIEWRECT_H

#include <QGraphicsItem>
#include <QPainter>

class PlayerViewRect : public QGraphicsItem
{
public:
    PlayerViewRect(bool *enabled);
    QRectF boundingRect() const override
    {
        qreal penWidth = 4;
        return QRectF(-penWidth,
                      -penWidth,
                      30 * 8 + penWidth * 2,
                      20 * 8 + penWidth * 2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        int width = 30 * 8;
        int height = 20 * 8;
        painter->setPen(QColor(0xff, 0xff, 0xff));
        painter->drawRect(-2, -2, width + 3, height + 3);
        painter->setPen(QColor(0, 0, 0));
        painter->drawRect(-3, -3, width + 5, height + 5);
        painter->drawRect(-1, -1, width + 1, height + 1);
    }
    void updateLocation(int x, int y);
    bool *enabled;
};

#endif // PLAYERVIEWRECT_H
