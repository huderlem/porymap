#ifndef MAPRULER_H
#define MAPRULER_H

#include <QGraphicsObject>
#include <QLine>


class MapRuler : public QGraphicsObject, private QLine
{
    Q_OBJECT

public:
    MapRuler(QColor innerColor = Qt::yellow, QColor borderColor = Qt::black) :
        innerColor(innerColor),
        borderColor(borderColor),
        mapSize(QSize()),
        statusMessage(QString()),
        xRuler(QRect()),
        yRuler(QRect()),
        cornerTick(QLine()),
        anchored(false),
        locked(false)
    {  }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    bool eventFilter(QObject *, QEvent *event) override;

    void setEnabled(bool enabled);
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
    QColor innerColor;
    QColor borderColor;
    QSize mapSize;
    QString statusMessage;
    QRect xRuler;
    QRect yRuler;
    QLineF cornerTick;
    bool anchored;
    bool locked;

    static int thickness;

    void init();
    void setAnchor(const QPointF &scenePos);
    void setEndPos(const QPointF &scenePos);
    QPoint snapToWithinBounds(QPoint pos) const;
    void updateGeometry();
    int pixWidth() const { return width() * 16; } 
    int pixHeight() const { return height() * 16; }
};

#endif // MAPRULER_H
