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

int ConnectionPixmapItem::getMinOffset() {
    if (this->connection->direction == "up" || this->connection->direction == "down")
        return -(this->pixmap().width() / 16) - 6;
    else
        return -(this->pixmap().height() / 16) - 6;
}

int ConnectionPixmapItem::getMaxOffset() {
    if (this->connection->direction == "up" || this->connection->direction == "down")
        return this->baseMapWidth + 6;
    else
        return this->baseMapHeight + 6;
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
            newOffset = qMin(newOffset, this->getMaxOffset());
            newOffset = qMax(newOffset, this->getMinOffset());
            x = newOffset * 16;
        }
        else {
            x = this->initialX;
        }

        if (this->connection->direction == "right" || this->connection->direction == "left") {
            y = round(newPos.y() / 16) * 16;
            newOffset += (y - this->initialY) / 16;
            newOffset = qMin(newOffset, this->getMaxOffset());
            newOffset = qMax(newOffset, this->getMinOffset());
            y = newOffset * 16;
        }
        else {
            y = this->initialY;
        }

        this->connection->offset = QString::number(newOffset);
        emit connectionMoved(this->connection);
        return QPointF(x, y);
    }
    else {
        return QGraphicsItem::itemChange(change, value);
    }
}

void ConnectionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *) {
    emit connectionItemSelected(this);
}

void ConnectionPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    emit connectionItemDoubleClicked(this);
}
