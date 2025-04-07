#include "connectionpixmapitem.h"
#include "editcommands.h"
#include "map.h"

#include <math.h>

ConnectionPixmapItem::ConnectionPixmapItem(MapConnection* connection)
    : QGraphicsPixmapItem(connection->render()),
      connection(connection)
{
    this->setEditable(true);
    setFlag(ItemIsFocusable, true);
    this->basePixmap = pixmap();
    updateOrigin();
    render(false);

    // If the connection changes externally we want to update the pixmap to reflect the change.
    connect(connection, &MapConnection::offsetChanged, this, &ConnectionPixmapItem::updatePos);
    connect(connection, &MapConnection::directionChanged, this, &ConnectionPixmapItem::refresh);
    connect(connection, &MapConnection::targetMapNameChanged, this, &ConnectionPixmapItem::refresh);
}

void ConnectionPixmapItem::refresh() {
    updateOrigin();
    render(true);
}

// Render additional visual effects on top of the base map image.
void ConnectionPixmapItem::render(bool ignoreCache) {
    if (ignoreCache)
        this->basePixmap = this->connection->render();

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

        qreal x = this->originX;
        qreal y = this->originY;
        int newOffset = this->connection->offset();

        // Restrict movement to the metatile grid and perpendicular to the connection direction.
        if (this->connection->isVertical()) {
            x = (round(newPos.x() / this->mWidth) * this->mWidth) - this->originX;
            newOffset = x / this->mWidth;
        } else if (this->connection->isHorizontal()) {
            y = (round(newPos.y() / this->mHeight) * this->mHeight) - this->originY;
            newOffset = y / this->mHeight;
        }

        // This is convoluted because of how our edit history works; this would otherwise just be 'this->connection->setOffset(newOffset);'
        if (this->connection->parentMap() && newOffset != this->connection->offset())
            this->connection->parentMap()->commit(new MapConnectionMove(this->connection, newOffset, this->actionId));

        return QPointF(x, y);
    }
    else {
        return QGraphicsItem::itemChange(change, value);
    }
}

// If connection->offset changed externally we call this to correct our position.
void ConnectionPixmapItem::updatePos() {
    qreal x = this->originX;
    qreal y = this->originY;

    if (this->connection->isVertical()) {
        x += this->connection->offset() * this->mWidth;
    } else if (this->connection->isHorizontal()) {
        y += this->connection->offset() * this->mHeight;
    }

    this->setPos(x, y);
    emit positionChanged(x, y);
}

void ConnectionPixmapItem::updateOrigin() {
    if (this->connection->isVertical()) {
        this->originX = 0;
        this->originY = this->connection->relativePos(true).y() * this->mHeight;
    } else if (this->connection->isHorizontal()) {
        this->originX = this->connection->relativePos(true).x() * this->mWidth;
        this->originY = 0;
    }
    updatePos();
}

void ConnectionPixmapItem::setEditable(bool editable) {
    setFlag(ItemIsMovable, editable);
    setFlag(ItemSendsGeometryChanges, editable);
}

bool ConnectionPixmapItem::getEditable() {
    return (this->flags() & ItemIsMovable) != 0;
}

void ConnectionPixmapItem::setSelected(bool selected) {
    if (selected && !hasFocus()) {
        setFocus(Qt::OtherFocusReason);
    }

    if (this->selected == selected)
        return;
    this->selected = selected;

    this->render();
    emit selectionChanged(selected);
}

void ConnectionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *) {
    setFocus(Qt::MouseFocusReason);
}

void ConnectionPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    this->actionId++; // Distinguish between move actions for the edit history
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

void ConnectionPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    emit connectionItemDoubleClicked(this->connection);
}

void ConnectionPixmapItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit deleteRequested(this->connection);
    } else {
        QGraphicsPixmapItem::keyPressEvent(event);
    }
}

void ConnectionPixmapItem::focusInEvent(QFocusEvent* event) {
    if (!this->getEditable())
        return;
    this->setSelected(true);
    QGraphicsPixmapItem::focusInEvent(event);
}
