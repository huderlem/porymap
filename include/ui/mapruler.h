#ifndef MAPRULER_H
#define MAPRULER_H

#include <QGraphicsObject>
#include <QLine>


class MapRuler : public QGraphicsObject, private QLine
{
    Q_OBJECT

public:
    // thickness is given in scene pixels
    MapRuler(int thickness, QColor innerColor = Qt::yellow, QColor borderColor = Qt::black);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    bool eventFilter(QObject *, QEvent *event) override;

    bool isAnchored() const { return anchored; }
    bool isLocked() const { return locked; }

    // Ruler start point in metatiles
    QPoint anchor() const { return QLine::p1(); }
    // Ruler end point in metatiles
    QPoint endPos() const { return QLine::p2(); }
    // X-coordinate of the ruler's left edge in metatiles
    int left() const { return qMin(anchor().x(), endPos().x()); }
    // Y-coordinate of the ruler's top edge in metatiles
    int top() const { return qMin(anchor().y(), endPos().y()); }
    // Horizontal component of the ruler in metatiles
    int deltaX() const { return QLine::dx(); }
    // Vertical component of the ruler in metatiles
    int deltaY() const { return QLine::dy(); }
    // Ruler width in metatiles
    int width() const { return qAbs(deltaX()); }
    // Ruler height in metatiles
    int height() const { return qAbs(deltaY()); }

public slots:
    void mouseEvent(QGraphicsSceneMouseEvent *event);
    void setMapDimensions(const QSize &size);

signals:
    void statusChanged(const QString &statusMessage);

private:
    const int thickness;
    const qreal half_thickness;
    const QColor innerColor;
    const QColor borderColor;
    QSize mapSize;
    QRectF xRuler;
    QRectF yRuler;
    QLineF cornerTick;
    bool anchored;
    bool locked;

    void reset();
    void setAnchor(const QPointF &scenePos);
    void setEndPos(const QPointF &scenePos);
    QPoint snapToWithinBounds(QPoint pos) const;
    void updateGeometry();
    void updateStatus(Qt::Corner corner);
    int pixWidth() const { return width() * 16; } 
    int pixHeight() const { return height() * 16; }
};

#endif // MAPRULER_H
