#ifndef MOVABLERECT_H
#define MOVABLERECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRgb>



class MovableRect : public QGraphicsRectItem
{
public:
    MovableRect(bool *enabled, const QRectF &rect, const QRgb &color);
    QRectF boundingRect() const override {
        qreal penWidth = 4;
        return QRectF(-penWidth,
                      -penWidth,
                      this->rect().width() + penWidth * 2,
                      this->rect().height() + penWidth * 2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        if (!(*enabled)) return;
        painter->setPen(this->color);
        painter->drawRect(this->rect() + QMargins(1,1,1,1)); // Fill
        painter->setPen(Qt::black);
        painter->drawRect(this->rect() + QMargins(2,2,2,2)); // Outer border
        painter->drawRect(this->rect()); // Inner border
    }
    void updateLocation(int x, int y);
    bool *enabled;

protected:
    QRectF baseRect;
    QRgb color;
};



/// A MovableRect with the addition of being resizable.
class ResizableRect : public QObject, public MovableRect
{
    Q_OBJECT
public:
    ResizableRect(QObject *parent, bool *enabled, int width, int height, QRgb color);

    QRectF boundingRect() const override {
        return QRectF(this->rect() + QMargins(lineWidth, lineWidth, lineWidth, lineWidth));
    }

    QPainterPath shape() const override {
        QPainterPath path;
        path.addRect(this->rect() + QMargins(lineWidth, lineWidth, lineWidth, lineWidth));
        path.addRect(this->rect() - QMargins(lineWidth, lineWidth, lineWidth, lineWidth));
        return path;
    }

    void updatePosFromRect(QRect newPos);
    void setLimit(QRect limit) { this->limit = limit; }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
    enum class Edge { None, Left, Right, Top, Bottom, TopLeft, BottomLeft, TopRight, BottomRight };
    ResizableRect::Edge detectEdge(int x, int y);

    // Variables for keeping state of original rect while resizing
    ResizableRect::Edge clickedEdge = ResizableRect::Edge::None;
    QPointF clickedPos = QPointF();
    QRect clickedRect;

    QRect limit = QRect();

    int lineWidth = 8;

signals:
    void rectUpdated(QRect rect);
};

#endif // MOVABLERECT_H
