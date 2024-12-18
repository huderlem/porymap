#ifndef MOVABLERECT_H
#define MOVABLERECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRgb>



class MovableRect : public QGraphicsRectItem
{
public:
    MovableRect(bool *enabled, int width, int height, QRgb color);
    QRectF boundingRect() const override {
        qreal penWidth = 4;
        return QRectF(-penWidth,
                      -penWidth,
                      30 * 8 + penWidth * 2,
                      20 * 8 + penWidth * 2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        if (!(*enabled)) return;
        painter->setPen(this->color);
        painter->drawRect(this->rect().x() - 2, this->rect().y() - 2, this->rect().width() + 3, this->rect().height() + 3);
        painter->setPen(QColor(0, 0, 0));
        painter->drawRect(this->rect().x() - 3, this->rect().y() - 3, this->rect().width() + 5, this->rect().height() + 5);
        painter->drawRect(this->rect().x() - 1, this->rect().y() - 1, this->rect().width() + 1, this->rect().height() + 1);
    }
    void updateLocation(int x, int y);
    bool *enabled;

protected:
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
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
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
