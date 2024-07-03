#include "connectionpixmapitem.h"

#include <math.h>

void ConnectionPixmapItem::render(qreal opacity) {
    QPixmap newPixmap = this->basePixmap.copy(0, 0, this->basePixmap.width(), this->basePixmap.height());
    if (opacity < 1) {
        QPainter painter(&newPixmap);
        int alpha = static_cast<int>(255 * (1 - opacity));
        painter.fillRect(0, 0, newPixmap.width(), newPixmap.height(), QColor(0, 0, 0, alpha));
        painter.end();
    }
    this->setPixmap(newPixmap);
}

QVariant ConnectionPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        QPointF newPos = value.toPointF();

        qreal x, y;
        int newOffset = this->initialOffset;
        if (this->connection->direction == "up" || this->connection->direction == "down") {
            x = round(newPos.x() / 16) * 16;
            newOffset += (x - initialX) / 16;
            x = newOffset * 16;
        }
        else {
            x = this->initialX;
        }

        if (this->connection->direction == "right" || this->connection->direction == "left") {
            y = round(newPos.y() / 16) * 16;
            newOffset += (y - this->initialY) / 16;
            y = newOffset * 16;
        }
        else {
            y = this->initialY;
        }

        this->connection->offset = newOffset;
        emit connectionMoved(this->connection);
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

// TODO: Consider whether it still makes sense to highlight the "current" connection (given you can edit them at any point now)
void ConnectionPixmapItem::updateHighlight(bool selected) {
    const int normalZ = -1;
    const qreal normalOpacity = 1.0;

    // When editing is inactive the current selection is ignored, all connections should appear normal.
    if (!this->getEditable()) {
        this->setZValue(normalZ);
        this->render(normalOpacity);
        return;
    }

    this->setZValue(selected ? normalZ : -2);
    this->render(selected ? normalOpacity : 0.75);

    if (selected) {
        // Draw highlight
        QPixmap pixmap = this->pixmap();
        QPainter painter(&pixmap);
        painter.setPen(QColor(255, 0, 255));
        painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
        painter.end();
        this->setPixmap(pixmap);
    }

    // Let the list UI know if the selection highlight changes so it can update accordingly
    if (this->highlighted != selected)
        emit highlightChanged(selected);
    this->highlighted = selected;
}

void ConnectionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *) {
    if (!this->getEditable())
        return;
    emit connectionItemSelected(this);
}

void ConnectionPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    emit connectionItemDoubleClicked(this);
}
