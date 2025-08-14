#include "bordermetatilespixmapitem.h"
#include "imageproviders.h"
#include "metatile.h"
#include "editcommands.h"
#include <QPainter>

void BorderMetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    if (event->buttons() & Qt::RightButton) {
        // Selecting metatiles
        this->metatileSelector->select(this->layout->getBorderMetatileId(pos.x(), pos.y()));
        return;
    }

    // Painting metatiles
    MetatileSelection selection = this->metatileSelector->getMetatileSelection();

    int width = layout->getBorderWidth();
    int height = layout->getBorderHeight();

    Blockdata oldBorder = layout->border;

    for (int i = 0; i < selection.dimensions.width() && (i + pos.x()) < width; i++) {
        for (int j = 0; j < selection.dimensions.height() && (j + pos.y()) < height; j++) {
            MetatileSelectionItem item = selection.metatileItems.value(j * selection.dimensions.width() + i);
            layout->setBorderMetatileId(pos.x() + i, pos.y() + j, item.metatileId, true);
        }
    }

    if (layout->border != oldBorder) {
        layout->editHistory.push(new PaintBorder(layout, oldBorder, layout->border, 0));
    }

    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::draw() {
    layout->setBorderItem(this);

    int width = layout->getBorderWidth();
    int height = layout->getBorderHeight();
    QImage image(width * Metatile::pixelWidth(), height * Metatile::pixelHeight(), QImage::Format_RGBA8888);
    QPainter painter(&image);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            QImage metatile_image = getMetatileImage(layout->getBorderMetatileId(i, j), layout);
            int x = i * Metatile::pixelWidth();
            int y = j * Metatile::pixelHeight();
            painter.drawImage(x, y, metatile_image);
        }
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));

    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::hoverUpdate(const QPointF &pixmapPos) {
    QPoint pos = Metatile::coordFromPixmapCoord(pixmapPos);
    uint16_t metatileId = this->layout->getBorderMetatileId(pos.x(), pos.y());
    emit this->hoveredBorderMetatileSelectionChanged(metatileId);
}

void BorderMetatilesPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    this->hoverUpdate(event->pos());
}

void BorderMetatilesPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    this->hoverUpdate(event->pos());
}

void BorderMetatilesPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredBorderMetatileSelectionCleared();
}
