#include "mapruler.h"
#include "metatile.h"

#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QColor>

int MapRuler::thickness = 3;


MapRuler::MapRuler(QColor innerColor, QColor borderColor) :
    innerColor(innerColor),
    borderColor(borderColor),
    mapSize(QSize()),
    statusMessage(QString()),
    xRuler(QRect()),
    yRuler(QRect()),
    cornerTick(QLine()),
    anchored(false),
    locked(false)
{
    connect(this, &QGraphicsObject::enabledChanged, [this]() {
        if (!isEnabled() && anchored)
            init();
    });
}

QRectF MapRuler::boundingRect() const {
    return QRectF(-thickness, -thickness, pixWidth() + thickness * 2, pixHeight() + thickness * 2);
}

QPainterPath MapRuler::shape() const {
    QPainterPath ruler;
    ruler.setFillRule(Qt::WindingFill);
    ruler.addRect(xRuler);
    ruler.addRect(yRuler);
    ruler = ruler.simplified();
    for (int x = 17; x < pixWidth(); x += 16)
        ruler.addRect(x, xRuler.y(), 0, thickness);
    for (int y = 17; y < pixHeight(); y += 16)
        ruler.addRect(yRuler.x(), y, thickness, 0);
    return ruler;
}

void MapRuler::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setPen(QPen(borderColor));
    painter->setBrush(QBrush(innerColor));
    painter->drawPath(shape());
    if (deltaX() && deltaY())
        painter->drawLine(cornerTick);
}

bool MapRuler::eventFilter(QObject*, QEvent *event) {
    if (!isEnabled() || mapSize.isEmpty())
        return false;

    if (event->type() == QEvent::GraphicsSceneMousePress || event->type() == QEvent::GraphicsSceneMouseMove) {
        auto mouse_event = static_cast<QGraphicsSceneMouseEvent *>(event);
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
            init();
        else
            setEndPos(event->scenePos());
    }
}

void MapRuler::setMapDimensions(const QSize &size) {
    mapSize = size;
    init();
}

void MapRuler::init() {
    prepareGeometryChange();
    hide();
    setPoints(QPoint(), QPoint());
    statusMessage = QString();
    xRuler = QRect();
    yRuler = QRect();
    cornerTick = QLine();
    anchored = false;
    locked = false;
    emit statusChanged(statusMessage);
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
    setPos(QPoint(left() * 16 + 7, top() * 16 + 7));
    /* Determine what quadrant the end point is in relative to the anchor point. The anchor
     * point is the top-left corner of the metatile the ruler starts in, so a zero-length(s)
     * ruler is considered to be in the bottom-right quadrant from the anchor point. */
    if (deltaX() < 0 && deltaY() < 0) {
        // Top-left
        xRuler = QRect(0, pixHeight(), pixWidth() + thickness, thickness);
        yRuler = QRect(0, 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x() + 0.5, xRuler.y() + thickness - 0.5, yRuler.x() + thickness, xRuler.y());
        statusMessage = QString("Ruler: Left %1, Up %2\nStart(%3, %4), End(%5, %6)").arg(width()).arg(height())
                            .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
    } else if (deltaX() < 0) {
        // Bottom-left
        xRuler = QRect(0, 0, pixWidth() + thickness, thickness);
        yRuler = QRect(0, 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(xRuler.x() + 0.5, xRuler.y() + 0.5, xRuler.x() + thickness, xRuler.y() + thickness);
        statusMessage = QString("Ruler: Left %1").arg(width());
        if (deltaY())
            statusMessage += QString(", Down %1").arg(height());
        statusMessage += QString("\nStart(%1, %2), End(%3, %4)")
                                .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
    } else if (deltaY() < 0) {
        // Top-right
        xRuler = QRect(0, pixHeight(), pixWidth() + thickness, thickness);
        yRuler = QRect(pixWidth(), 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x(), xRuler.y(), yRuler.x() + thickness - 0.5, xRuler.y() + thickness - 0.5);
        statusMessage = QString("Ruler: ");
        if (deltaX())
            statusMessage += QString("Right %1, ").arg(width());
        statusMessage += QString("Up %1\nStart(%2, %3), End(%4, %5)").arg(height())
                            .arg(anchor().x()).arg(anchor().y()).arg(endPos().x()).arg(endPos().y());
    } else {
        // Bottom-right
        xRuler = QRect(0, 0, pixWidth() + thickness, thickness);
        yRuler = QRect(pixWidth(), 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x(), yRuler.y() + thickness, yRuler.x() + thickness - 0.5, yRuler.y() + 0.5);
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
            statusMessage += QString("0\nStart(%1, %2)").arg(anchor().x()).arg(anchor().y());
        }
    }
    emit statusChanged(statusMessage);
}
