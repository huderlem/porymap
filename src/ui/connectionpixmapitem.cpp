#include "connectionpixmapitem.h"

#include <math.h>

void ConnectionPixmapItem::render() {
    QPixmap pixmap = this->basePixmap.copy(0, 0, this->basePixmap.width(), this->basePixmap.height());
    this->setZValue(-1);

    // When editing is inactive the current selection is ignored, all connections should appear normal.
    if (this->getEditable()) {
        if (this->selected) {
            // Draw highlight
            QPainter painter(&pixmap);
            painter.setPen(QColor(255, 0, 255));
            painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
            painter.end();
        } else {
            // Darken the image
            this->setZValue(-2);
            QPainter painter(&pixmap);
            int alpha = static_cast<int>(255 * 0.25);
            painter.fillRect(0, 0, pixmap.width(), pixmap.height(), QColor(0, 0, 0, alpha));
            painter.end();
        }
    }
    this->setPixmap(pixmap);
}

QVariant ConnectionPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        QPointF newPos = value.toPointF();

        qreal x, y;
        int newOffset = this->initialOffset;
        if (MapConnection::isVertical(this->connection->direction())) {
            x = round(newPos.x() / 16) * 16;
            newOffset += (x - initialX) / 16;
            x = newOffset * 16;
        }
        else {
            x = this->initialX;
        }

        if (MapConnection::isHorizontal(this->connection->direction())) {
            y = round(newPos.y() / 16) * 16;
            newOffset += (y - this->initialY) / 16;
            y = newOffset * 16;
        }
        else {
            y = this->initialY;
        }

        if (this->connection->offset() != newOffset)
            emit connectionMoved(this->connection, newOffset);
        return QPointF(x, y);
    }
    else {
        return QGraphicsItem::itemChange(change, value);
    }
}

void ConnectionPixmapItem::setEditable(bool editable) {
    setFlag(ItemIsMovable, editable);
    setFlag(ItemSendsGeometryChanges, editable);
}

bool ConnectionPixmapItem::getEditable() {
    return (this->flags() & ItemIsMovable) != 0;
}

void ConnectionPixmapItem::setSelected(bool selected) {
    if (this->selected == selected)
        return;
    this->selected = selected;
    this->render();
    emit selectionChanged(selected);
}

void ConnectionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *) {
    if (!this->getEditable())
        return;
    this->setSelected(true);
}

void ConnectionPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    emit connectionItemDoubleClicked(this);
}
