#include "bordermetatilespixmapitem.h"
#include "imageproviders.h"
#include <QPainter>

void BorderMetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QList<uint16_t> *selectedMetatiles = this->metatileSelector->getSelectedMetatiles();
    QPoint selectionDimensions = this->metatileSelector->getSelectionDimensions();
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;

    for (int i = 0; i < selectionDimensions.x() && (i + x) < map->getBorderWidth(); i++) {
        for (int j = 0; j < selectionDimensions.y() && (j + y) < map->getBorderHeight(); j++) {
            int blockIndex = (j + y) * map->getBorderWidth() + (i + x);
            uint16_t tile = selectedMetatiles->at(j * selectionDimensions.x() + i);
            (*map->layout->border->blocks)[blockIndex].tile = tile;
        }
    }

    draw();
    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::draw() {
    QImage image(16 * map->getBorderWidth(), 16 * map->getBorderHeight(), QImage::Format_RGBA8888);
    QPainter painter(&image);
    QVector<Block> *blocks = map->layout->border->blocks;

    for (int i = 0; i < map->getBorderWidth(); i++) {
        for (int j = 0; j < map->getBorderHeight(); j++) {
            int x = i * 16;
            int y = j * 16;
            int index = j * map->getBorderWidth() + i;
            QImage metatile_image = getMetatileImage(blocks->value(index).tile, map->layout->tileset_primary, map->layout->tileset_secondary);
            QPoint metatile_origin = QPoint(x, y);
            painter.drawImage(metatile_origin, metatile_image);
        }
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
}
