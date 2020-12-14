#include "mapruler.h"
#include "metatile.h"

#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QColor>
#include <QVector>


MapRuler::MapRuler(int thickness, QColor innerColor, QColor borderColor) :
    /* The logical representation of rectangles are always one less than
     * the rendered shape, so we subtract 1 from thickness. */
    thickness(thickness - 1),
    half_thickness(qreal(thickness - 1) / 2.0),
    innerColor(innerColor),
    borderColor(borderColor),
    mapSize(QSize()),
    xRuler(QRectF()),
    yRuler(QRectF()),
    cornerTick(QLineF()),
    anchored(false),
    locked(false)
{
    connect(this, &QGraphicsObject::enabledChanged, [this]() {
        if (!isEnabled() && anchored)
            reset();
    });
}

QRectF MapRuler::boundingRect() const {
    return QRectF(-(half_thickness + 1), -(half_thickness + 1), pixWidth() + thickness + 2, pixHeight() + thickness + 2);
}

QPainterPath MapRuler::shape() const {
    QPainterPath ruler;
    ruler.setFillRule(Qt::WindingFill);
    ruler.addRect(xRuler);
    ruler.addRect(yRuler);
    ruler = ruler.simplified();
    for (int x = 16; x < pixWidth(); x += 16)
        ruler.addRect(x, xRuler.y(), 0, thickness);
    for (int y = 16; y < pixHeight(); y += 16)
        ruler.addRect(yRuler.x(), y, thickness, 0);
    if (deltaX() && deltaY())
        ruler.addPolygon(QVector<QPointF>({ cornerTick.p1(), cornerTick.p2() }));
    return ruler;
}

void MapRuler::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setPen(QPen(borderColor));
    painter->setBrush(QBrush(innerColor));
    painter->drawPath(shape());
}

bool MapRuler::eventFilter(QObject *, QEvent *event) {
    if (!isEnabled() || mapSize.isEmpty())
        return false;

    if (event->type() == QEvent::GraphicsSceneMousePress || event->type() == QEvent::GraphicsSceneMouseMove) {
        auto *mouse_event = static_cast<QGraphicsSceneMouseEvent *>(event);
        if (mouse_event->button() == Qt::RightButton || anchored) {
            mouseEvent(mouse_event);
        }
    }

    return false;
}

void MapRuler::mouseEvent(QGraphicsSceneMouseEvent *event) {
    if (!anchored && event->button() == Qt::RightButton) {
        setAnchor(event->scenePos());
    } else if (anchored) {
        if (event->button() == Qt::LeftButton)
            locked = !locked;
        if (event->button() == Qt::RightButton)
            reset();
        else
            setEndPos(event->scenePos());
    }
}

void MapRuler::setMapDimensions(const QSize &size) {
    mapSize = size;
    reset();
}

void MapRuler::reset() {
    prepareGeometryChange();
    hide();
    setPoints(QPoint(), QPoint());
    xRuler = QRectF();
    yRuler = QRectF();
    cornerTick = QLineF();
    anchored = false;
    locked = false;
    emit statusChanged(QString());
}

void MapRuler::setAnchor(const QPointF &scenePos) {
    QPoint pos = Metatile::coordFromPixmapCoord(scenePos);
    pos = snapToWithinBounds(pos);
    anchored = true;
    locked = false;
    setPoints(pos, pos);
    updateGeometry();
    show();
}

void MapRuler::setEndPos(const QPointF &scenePos) {
    if (locked)
        return;
    QPoint pos = Metatile::coordFromPixmapCoord(scenePos);
    pos = snapToWithinBounds(pos);
    const QPoint lastEndPos = endPos();
    setP2(pos);
    if (pos != lastEndPos)
        updateGeometry();
}

QPoint MapRuler::snapToWithinBounds(QPoint pos) const {
    if (pos.x() < 0)
        pos.setX(0);
    if (pos.y() < 0)
        pos.setY(0);
    if (pos.x() >= mapSize.width())
        pos.setX(mapSize.width() - 1);
    if (pos.y() >= mapSize.height())
        pos.setY(mapSize.height() - 1);
    return pos;
}

void MapRuler::updateGeometry() {
    prepareGeometryChange();
    setPos(QPoint(left() * 16 + 8, top() * 16 + 8));
    /* Determine what quadrant the end point is in relative to the anchor point. The anchor
     * point is the top-left corner of the metatile the ruler starts in, so a zero-length
     * ruler is considered to be in the bottom-right quadrant from the anchor point. */
    if (deltaX() < 0 && deltaY() < 0) {
        // Top-left
        xRuler = QRectF(-half_thickness, pixHeight() - half_thickness, pixWidth() + thickness, thickness);
        yRuler = QRectF(-half_thickness, -half_thickness, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x() + 0.5, xRuler.y() + thickness - 0.5, yRuler.x() + thickness, xRuler.y());
        updateStatus(Qt::TopLeftCorner);
    } else if (deltaX() < 0) {
        // Bottom-left
        xRuler = QRectF(-half_thickness, -half_thickness, pixWidth() + thickness, thickness);
        yRuler = QRectF(-half_thickness, -half_thickness, thickness, pixHeight() + thickness);
        cornerTick = QLineF(xRuler.x() + 0.5, xRuler.y() + 0.5, xRuler.x() + thickness, xRuler.y() + thickness);
        updateStatus(Qt::BottomLeftCorner);
    } else if (deltaY() < 0) {
        // Top-right
        xRuler = QRectF(-half_thickness, pixHeight() - half_thickness, pixWidth() + thickness, thickness);
        yRuler = QRectF(pixWidth() - half_thickness, -half_thickness, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x(), xRuler.y(), yRuler.x() + thickness - 0.5, xRuler.y() + thickness - 0.5);
        updateStatus(Qt::TopRightCorner);
    } else {
        // Bottom-right
        xRuler = QRectF(-half_thickness, -half_thickness, pixWidth() + thickness, thickness);
        yRuler = QRectF(pixWidth() - half_thickness, -half_thickness, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x(), yRuler.y() + thickness, yRuler.x() + thickness - 0.5, yRuler.y() + 0.5);
        updateStatus(Qt::BottomRightCorner);
    }
}

void MapRuler::updateStatus(Qt::Corner corner) {
    QString statusMessage;
    switch (corner)
    {
    case Qt::TopLeftCorner:
        statusMessage = QString("Ruler: Left %1, Up %2\nStart(%3, %4), End(%5, %6)").arg(width()).arg(height())
                        .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
        break;
    case Qt::BottomLeftCorner:
        statusMessage = QString("Ruler: Left %1").arg(width());
        if (deltaY())
            statusMessage += QString(", Down %1").arg(height());
        statusMessage += QString("\nStart(%1, %2), End(%3, %4)")
                        .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
        break;
    case Qt::TopRightCorner:
        statusMessage = QString("Ruler: ");
        if (deltaX())
            statusMessage += QString("Right %1, ").arg(width());
        statusMessage += QString("Up %1\nStart(%2, %3), End(%4, %5)").arg(height())
                        .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
        break;
    case Qt::BottomRightCorner:
        statusMessage = QString("Ruler: ");
        if (deltaX() || deltaY()) {
            if (deltaX())
                statusMessage += QString("Right %1").arg(width());
            if (deltaY()) {
                if (deltaX())
                    statusMessage += ", ";
                statusMessage += QString("Down: %1").arg(height());
            }
            statusMessage += QString("\nStart(%1, %2), End(%3, %4)")
                            .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
        } else {
            statusMessage += QString("0\nStart(%1, %2)")
                            .arg(anchor().x()).arg(anchor().y());
        }
        break;
    }
    emit statusChanged(statusMessage);
}
