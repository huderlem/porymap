#ifndef MAPRULER_H
#define MAPRULER_H

#include <QGraphicsItem>
#include <QPainter>
#include <QColor>


class MapRuler : public QGraphicsItem, private QLine
{
public:
    MapRuler(QColor interior = Qt::yellow, QColor exterior = Qt::black)
    : interiorColor(interior), exteriorColor(exterior)
    {
        init();
        setAcceptedMouseButtons(Qt::RightButton);
    }
    void init();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    // Anchor the ruler on metatile 'tilePos' and show the ruler
    void setAnchor(const QPoint &tilePos);
    // Anchor the ruler on metatile (tileX,tileY) and show the ruler
    void setAnchor(int tileX, int tileY) { setAnchor(QPoint(tileX, tileY)); }
    // Release the ruler anchor and hide the ruler
    void endAnchor();
    // Set the ruler end point to metatile 'tilePos' and repaint
    void setEndPos(const QPoint &tilePos);
    // Set the ruler end point to metatile (tileX, tileY) and repaint
    void setEndPos(int tileX, int tileY) { setEndPos(QPoint(tileX, tileY)); }
    
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

    bool mousePressed(Qt::MouseButtons buttons) { return buttons & acceptedMouseButtons(); }

private:
    QRect xRuler;
    QRect yRuler;
    QLineF cornerTick;
    QRect widthTextBox;
    QRect heightTextBox;
    QColor interiorColor;
    QColor exteriorColor;

    static int padding;
    static int thickness;
    
    void updateShape();
};

#endif // MAPRULER_H
