#include "bordermetatilespixmapitem.h"
#include "imageproviders.h"
#include <QPainter>

void BorderMetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QList<uint16_t> *selectedMetatiles = this->metatileSelector->getSelectedMetatiles();
    QPoint selectionDimensions = this->metatileSelector->getSelectionDimensions();
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    int width = map->getBorderWidth();
    int height = map->getBorderHeight();

    for (int i = 0; i < selectionDimensions.x() && (i + x) < width; i++) {
        for (int j = 0; j < selectionDimensions.y() && (j + y) < height; j++) {
            int blockIndex = (j + y) * width + (i + x);
            uint16_t tile = selectedMetatiles->at(j * selectionDimensions.x() + i);
            (*map->layout->border->blocks)[blockIndex].tile = tile;
        }
    }

    draw();
    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::draw() {
    int width = map->getBorderWidth();
    int height = map->getBorderHeight();
    QImage image(16 * width, 16 * height, QImage::Format_RGBA8888);
    QPainter painter(&image);
    QVector<Block> *blocks = map->layout->border->blocks;

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            int x = i * 16;
            int y = j * 16;
            int index = j * width + i;
            QImage metatile_image = getMetatileImage(blocks->value(index).tile, map->layout->tileset_primary, map->layout->tileset_secondary);
            QPoint metatile_origin = QPoint(x, y);
            painter.drawImage(metatile_origin, metatile_image);
        }
    }

    painter.end();
    map->commit();
    this->setPixmap(QPixmap::fromImage(image));
}
