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

void SelectablePixmapItem::setSelection(const QPoint &pos, const QSize &size) {
    this->selectionInitialX = pos.x();
    this->selectionInitialY = pos.y();
    this->selectionOffsetX = getBoundedWidth(size.width()) - 1;
    this->selectionOffsetY = getBoundedHeight(size.height()) - 1;
}

void SelectablePixmapItem::select(const QPoint &pos, const QSize &size) {
    setSelection(pos, size);
    draw();
    emit selectionChanged(pos, getSelectionDimensions());
}

void SelectablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPoint pos = getCellPos(event->pos());
    setSelection(pos, QSize(1,1));
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
        setSelection(getSelectionStart(), size);
        draw();
        // 'draw' is allowed to change the selection position/size,
        // so call these again rather than keep values from above.
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

void SelectablePixmapItem::setSelectionStyle(Qt::PenStyle style) {
    this->selectionStyle = style;
    draw();
}

QPoint SelectablePixmapItem::getCellPos(const QPointF &itemPos) {
    if (this->cellWidth == 0 || this->cellHeight == 0 || pixmap().isNull())
        return QPoint(0,0);

    int x = qBound(0, static_cast<int>(itemPos.x()), pixmap().width() - 1);
    int y = qBound(0, static_cast<int>(itemPos.y()), pixmap().height() - 1);
    return QPoint(x / this->cellWidth, y / this->cellHeight);
}

void SelectablePixmapItem::drawSelection() {
    drawSelectionRect(getSelectionStart(), getSelectionDimensions(), this->selectionStyle);
}

void SelectablePixmapItem::drawSelectionRect(const QPoint &origin, const QSize &dimensions, Qt::PenStyle style) {
    QRect selectionRect(origin.x() * this->cellWidth, origin.y() * this->cellHeight, dimensions.width() * this->cellWidth, dimensions.height() * this->cellHeight);

    // If a selection is fully outside the bounds of the selectable area, don't draw anything.
    // This prevents the border of the selection rectangle potentially being visible on an otherwise invisible selection.
    QPixmap pixmap = this->pixmap();
    if (!selectionRect.intersects(pixmap.rect()))
        return;

    auto fillPen = QPen(QColor(Qt::white));
    auto borderPen = QPen(QColor(Qt::black));
    borderPen.setStyle(style);

    QPainter painter(&pixmap);
    if (style == Qt::SolidLine) {
        painter.setPen(fillPen);
        painter.drawRect(selectionRect - QMargins(1,1,1,1));
        painter.setPen(borderPen);
        painter.drawRect(selectionRect);
        painter.drawRect(selectionRect - QMargins(2,2,2,2));
    } else {
        // Having separately sized rectangles with anything but a
        // solid line looks a little wonky because the dashes wont align.
        // For non-solid styles we'll draw a base white rectangle, then draw
        // a styled black rectangle on top
        painter.setPen(fillPen);
        painter.drawRect(selectionRect);
        painter.setPen(borderPen);
        painter.drawRect(selectionRect);
    }

    this->setPixmap(pixmap);
}
