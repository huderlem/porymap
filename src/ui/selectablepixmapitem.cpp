#include "selectablepixmapitem.h"
#include <QPainter>

QPoint SelectablePixmapItem::getSelectionDimensions()
{
    return QPoint(abs(this->selectionOffsetX) + 1, abs(this->selectionOffsetY) + 1);
}

QPoint SelectablePixmapItem::getSelectionStart()
{
    int x = this->selectionInitialX;
    int y = this->selectionInitialY;
    if (this->selectionOffsetX < 0) x += this->selectionOffsetX;
    if (this->selectionOffsetY < 0) y += this->selectionOffsetY;
    return QPoint(x, y);
}

void SelectablePixmapItem::select(int x, int y, int width, int height)
{
    this->selectionInitialX = x;
    this->selectionInitialY = y;
    this->selectionOffsetX = qMax(0, qMin(width, this->maxSelectionWidth));
    this->selectionOffsetY = qMax(0, qMin(height, this->maxSelectionHeight));
    this->draw();
    emit this->selectionChanged(x, y, width, height);
}

void SelectablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = this->getCellPos(event->pos());
    this->selectionInitialX = pos.x();
    this->selectionInitialY = pos.y();
    this->selectionOffsetX = 0;
    this->selectionOffsetY = 0;
    this->updateSelection(pos.x(), pos.y());
}

void SelectablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = this->getCellPos(event->pos());
    this->updateSelection(pos.x(), pos.y());
}

void SelectablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = this->getCellPos(event->pos());
    this->updateSelection(pos.x(), pos.y());
}

void SelectablePixmapItem::updateSelection(int x, int y)
{
    // Snap to a valid position inside the selection area.
    int width = pixmap().width() / this->cellWidth;
    int height = pixmap().height() / this->cellHeight;
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;

    this->selectionOffsetX = x - this->selectionInitialX;
    this->selectionOffsetY = y - this->selectionInitialY;

    // Respect max selection dimensions by moving the selection's origin.
    if (this->selectionOffsetX >= this->maxSelectionWidth) {
        this->selectionInitialX += this->selectionOffsetX - this->maxSelectionWidth + 1;
        this->selectionOffsetX = this->maxSelectionWidth - 1;
    } else if (this->selectionOffsetX <= -this->maxSelectionWidth) {
        this->selectionInitialX += this->selectionOffsetX + this->maxSelectionWidth - 1;
        this->selectionOffsetX = -this->maxSelectionWidth + 1;
    }
    if (this->selectionOffsetY >= this->maxSelectionHeight) {
        this->selectionInitialY += this->selectionOffsetY - this->maxSelectionHeight + 1;
        this->selectionOffsetY = this->maxSelectionHeight - 1;
    } else if (this->selectionOffsetY <= -this->maxSelectionHeight) {
        this->selectionInitialY += this->selectionOffsetY + this->maxSelectionHeight - 1;
        this->selectionOffsetY = -this->maxSelectionHeight + 1;
    }

    this->draw();
    emit this->selectionChanged(x, y, width, height);
}

QPoint SelectablePixmapItem::getCellPos(QPointF pos)
{
    if (pos.x() < 0) pos.setX(0);
    if (pos.y() < 0) pos.setY(0);
    if (pos.x() >= this->pixmap().width()) pos.setX(this->pixmap().width() - 1);
    if (pos.y() >= this->pixmap().height()) pos.setY(this->pixmap().height() - 1);
    return QPoint(static_cast<int>(pos.x()) / this->cellWidth,
                  static_cast<int>(pos.y()) / this->cellHeight);
}

void SelectablePixmapItem::drawSelection()
{
    QPixmap pixmap = this->pixmap();
    QPainter painter(&pixmap);
    QPoint origin = this->getSelectionStart();
    QPoint dimensions = this->getSelectionDimensions();

    int rectWidth = dimensions.x() * this->cellWidth;
    int rectHeight = dimensions.y() * this->cellHeight;

    painter.setPen(QColor(0xff, 0xff, 0xff));
    painter.drawRect(origin.x() * this->cellWidth, origin.y() * this->cellHeight, rectWidth - 1, rectHeight -1);
    painter.setPen(QColor(0, 0, 0));
    painter.drawRect(origin.x() * this->cellWidth - 1, origin.y() * this->cellHeight - 1, rectWidth + 1, rectHeight + 1);
    painter.drawRect(origin.x() * this->cellWidth + 1, origin.y() * this->cellHeight + 1, rectWidth - 3, rectHeight - 3);

    this->setPixmap(pixmap);
}
