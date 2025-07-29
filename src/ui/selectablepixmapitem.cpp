#include "selectablepixmapitem.h"
#include <QPainter>

QPoint SelectablePixmapItem::getSelectionStart()
{
    int x = this->selectionInitialX;
    int y = this->selectionInitialY;
    if (this->selectionOffsetX < 0) x += this->selectionOffsetX;
    if (this->selectionOffsetY < 0) y += this->selectionOffsetY;
    return QPoint(x, y);
}

void SelectablePixmapItem::select(const QPoint &pos, const QSize &size)
{
    this->selectionInitialX = pos.x();
    this->selectionInitialY = pos.y();
    this->selectionOffsetX = qBound(0, size.width(), this->maxSelectionWidth - 1);
    this->selectionOffsetY = qBound(0, size.height(), this->maxSelectionHeight - 1);
    draw();
    emit selectionChanged(pos, getSelectionDimensions());
}

void SelectablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = getCellPos(event->pos());
    this->selectionInitialX = pos.x();
    this->selectionInitialY = pos.y();
    this->selectionOffsetX = 0;
    this->selectionOffsetY = 0;
    this->prevCellPos = pos;
    updateSelection(pos);
}

void SelectablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = getCellPos(event->pos());
    if (pos == this->prevCellPos)
        return;
    this->prevCellPos = pos;
    updateSelection(pos);
}

void SelectablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    updateSelection(getCellPos(event->pos()));
}

void SelectablePixmapItem::setMaxSelectionSize(int width, int height) {
    this->maxSelectionWidth = qMax(width, 1);
    this->maxSelectionHeight = qMax(height, 1);

    // Update the selection if we shrank below the current selection size.
    QSize size = getSelectionDimensions();
    if (size.width() > this->maxSelectionWidth || size.height() > this->maxSelectionHeight) {
        QPoint origin = getSelectionStart();
        this->selectionInitialX = origin.x();
        this->selectionInitialY = origin.y();
        this->selectionOffsetX = qMin(size.width(), this->maxSelectionWidth) - 1;
        this->selectionOffsetY = qMin(size.height(), this->maxSelectionHeight) - 1;
        draw();
        emit selectionChanged(getSelectionStart(), getSelectionDimensions());
    }
}

void SelectablePixmapItem::updateSelection(const QPoint &pos) {
    // Snap to a valid position inside the selection area.
    int x = qBound(0, pos.x(), cellsWide() - 1);
    int y = qBound(0, pos.y(), cellsTall() - 1);

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

    draw();
    emit selectionChanged(QPoint(x, y), getSelectionDimensions());
}

QPoint SelectablePixmapItem::getCellPos(QPointF pos) {
    if (this->cellWidth == 0 || this->cellHeight == 0 || pixmap().isNull())
        return QPoint(0,0);

    int x = qBound(0, static_cast<int>(pos.x()), pixmap().width() - 1);
    int y = qBound(0, static_cast<int>(pos.y()), pixmap().height() - 1);
    return QPoint(x / this->cellWidth, y / this->cellHeight);
}

void SelectablePixmapItem::drawSelection() {
    QPoint origin = this->getSelectionStart();
    QSize dimensions = this->getSelectionDimensions();
    QRect selectionRect(origin.x() * this->cellWidth, origin.y() * this->cellHeight, dimensions.width() * this->cellWidth, dimensions.height() * this->cellHeight);

    // If a selection is fully outside the bounds of the selectable area, don't draw anything.
    // This prevents the border of the selection rectangle potentially being visible on an otherwise invisible selection.
    QPixmap pixmap = this->pixmap();
    if (!selectionRect.intersects(pixmap.rect()))
        return;

    QPainter painter(&pixmap);
    painter.setPen(QColor(0xff, 0xff, 0xff));
    painter.drawRect(selectionRect.x(), selectionRect.y(), selectionRect.width() - 1, selectionRect.height() - 1);
    painter.setPen(QColor(0, 0, 0));
    painter.drawRect(selectionRect.x() - 1, selectionRect.y() - 1, selectionRect.width() + 1, selectionRect.height() + 1);
    painter.drawRect(selectionRect.x() + 1, selectionRect.y() + 1, selectionRect.width() - 3, selectionRect.height() - 3);

    this->setPixmap(pixmap);
}
