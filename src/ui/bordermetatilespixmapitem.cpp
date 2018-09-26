#include "bordermetatilespixmapitem.h"
#include "imageproviders.h"
#include <QPainter>

void BorderMetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QList<uint16_t> *selectedMetatiles = this->metatileSelector->getSelectedMetatiles();
    QPoint selectionDimensions = this->metatileSelector->getSelectionDimensions();
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;

    for (int i = 0; i < selectionDimensions.x() && (i + x) < 2; i++) {
        for (int j = 0; j < selectionDimensions.y() && (j + y) < 2; j++) {
            int blockIndex = (j + y) * 2 + (i + x);
            uint16_t tile = selectedMetatiles->at(j * selectionDimensions.x() + i);
            (*map->layout->border->blocks)[blockIndex].tile = tile;
        }
    }

    draw();
    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::draw() {
    QImage image(32, 32, QImage::Format_RGBA8888);
    QPainter painter(&image);
    QList<Block> *blocks = map->layout->border->blocks;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            int x = i * 16;
            int y = j * 16;
            int index = j * 2 + i;
            QImage metatile_image = getMetatileImage(blocks->value(index).tile, map->layout->tileset_primary, map->layout->tileset_secondary);
            QPoint metatile_origin = QPoint(x, y);
            painter.drawImage(metatile_origin, metatile_image);
        }
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
}
