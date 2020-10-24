#include "mapruler.h"
#include "metatile.h"

#include <QGraphicsObject>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QColor>
#include <QToolTip>

int MapRuler::thickness = 3;


void MapRuler::init() {
    setVisible(false);
    setPoints(QPoint(), QPoint());
    anchored = false;
    locked = false;
    statusMessage = QString("Ruler: 0");
    xRuler = QRect();
    yRuler = QRect();
    cornerTick = QLine();
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
    for (int x = 17.5; x < pixWidth(); x += 16)
        ruler.addRect(x, xRuler.y(), 0, thickness);
    for (int y = 17.5; y < pixHeight(); y += 16)
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
            event->accept();
            return true;
        }
    }

    return false;
}

void MapRuler::mouseEvent(QGraphicsSceneMouseEvent *event) {
    if (!anchored && event->button() == Qt::RightButton) {
        setAnchor(event->scenePos(), event->screenPos());
    } else if (anchored) {
        if (event->button() == Qt::LeftButton)
            locked = !locked;
        if (event->button() == Qt::RightButton)
            endAnchor();
        else
            setEndPos(event->scenePos(), event->screenPos());
    }
}

void MapRuler::setMapDimensions(const QSize &size) {
    mapSize = size;
    init();
}

void MapRuler::setEnabled(bool enabled) {
    QGraphicsItem::setEnabled(enabled);
    if (!enabled && anchored)
        endAnchor();
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

void MapRuler::setAnchor(const QPointF &scenePos, const QPoint &screenPos) {
    QPoint pos = Metatile::coordFromPixmapCoord(scenePos);
    pos = snapToWithinBounds(pos);
    anchored = true;
    locked = false;
    setPoints(pos, pos);
    updateGeometry();
    setVisible(true);
    showDimensions(screenPos);
}

void MapRuler::endAnchor() {
    emit deactivated(endPos());
    hideDimensions();
    prepareGeometryChange();
    init();
}

void MapRuler::setEndPos(const QPointF &scenePos, const QPoint &screenPos) {
    if (locked)
        return;
    QPoint pos = Metatile::coordFromPixmapCoord(scenePos);
    pos = snapToWithinBounds(pos);
    const QPoint lastEndPos = endPos();
    setP2(pos);
    if (pos != lastEndPos)
        updateGeometry();
    showDimensions(screenPos);
}

void MapRuler::showDimensions(const QPoint &screenPos) const {
    // This is a hack to make the tool tip follow the cursor since it won't change position if the text is the same.
    QToolTip::showText(screenPos + QPoint(16, -8), statusMessage + ' ');
    QToolTip::showText(screenPos + QPoint(16, -8), statusMessage);
}

void MapRuler::hideDimensions() const {
    QToolTip::hideText();
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
        statusMessage = QString("Ruler: Left %1, Up %2").arg(width()).arg(height());
    } else if (deltaX() < 0) {
        // Bottom-left
        xRuler = QRect(0, 0, pixWidth() + thickness, thickness);
        yRuler = QRect(0, 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(xRuler.x() + 0.5, xRuler.y() + 0.5, xRuler.x() + thickness, xRuler.y() + thickness);
        statusMessage = QString("Ruler: Left %1").arg(width());
        if (deltaY())
            statusMessage += QString(", Down %1").arg(height());
    } else if (deltaY() < 0) {
        // Top-right
        xRuler = QRect(0, pixHeight(), pixWidth() + thickness, thickness);
        yRuler = QRect(pixWidth(), 0, thickness, pixHeight() + thickness);
        cornerTick = QLineF(yRuler.x(), xRuler.y(), yRuler.x() + thickness - 0.5, xRuler.y() + thickness - 0.5);
        statusMessage = QString("Ruler: ");
        if (deltaX())
            statusMessage += QString("Right %1, ").arg(width());
        statusMessage += QString("Up %1").arg(height());
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
        } else {
            statusMessage += QString("0");
        }
    }
    emit lengthChanged();
}
