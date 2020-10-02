#ifndef MAPRULER_H
#define MAPRULER_H

#include <QGraphicsItem>
#include <QPainter>
#include <QColor>


class MapRuler : public QGraphicsItem, private QLine
{
public:
    MapRuler(QColor innerColor = Qt::yellow, QColor borderColor = Qt::black) :
        innerColor(innerColor),
        borderColor(borderColor)
    {
        init();
        setAcceptedMouseButtons(Qt::RightButton);
    }
    void init();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    // Anchor the ruler and make it visible
    void setAnchor(const QPointF &scenePos, const QPoint &screenPos);
    // Release the anchor and hide the ruler
    void endAnchor();
    // Set the end point and repaint
    void setEndPos(const QPointF &scenePos, const QPoint &screenPos);

    // Ruler start point in metatiles
    QPoint anchor() const { return QLine::p1(); }
    // Ruler end point in metatiles
    QPoint endPos() const { return QLine::p2(); }
    // X-coordinate of the ruler left edge in metatiles
    int left() const { return qMin(anchor().x(), endPos().x()); }
    // Y-coordinate of the ruler top edge in metatiles
    int top() const { return qMin(anchor().y(), endPos().y()); }
    // Horizontal component of the ruler in metatiles
    int deltaX() const { return QLine::dx(); }
    // Vertical component of the ruler in metatiles
    int deltaY() const { return QLine::dy(); }
    // Ruler width in metatiles
    int width() const { return qAbs(deltaX()); }
    // Ruler height in metatiles
    int height() const { return qAbs(deltaY()); }
    // Ruler width in map pixels
    int pixWidth() const { return width() * 16; } 
    // Ruler height in map pixels
    int pixHeight() const { return height() * 16; }

    bool isMousePressed(QGraphicsSceneMouseEvent *event) const;
    bool isAnchored() const { return anchored; }

    bool locked;
    QString statusMessage;

private:
    QColor innerColor;
    QColor borderColor;
    QRect xRuler;
    QRect yRuler;
    QLineF cornerTick;
    bool anchored;

    static int thickness;
    
    void showDimensions(const QPoint &screenPos);
    void hideDimensions() const;
    void updateGeometry();
};

#endif // MAPRULER_H
