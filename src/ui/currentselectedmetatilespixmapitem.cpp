#include "currentselectedmetatilespixmapitem.h"
#include "imageproviders.h"
#include <QPainter>

void CurrentSelectedMetatilesPixmapItem::draw() {
    QList<uint16_t> *selectedMetatiles = metatileSelector->getSelectedMetatiles();
    QPoint selectionDimensions = metatileSelector->getSelectionDimensions();
    int width = selectionDimensions.x() * 16;
    int height = selectionDimensions.y() * 16;
    QImage image(width, height, QImage::Format_RGBA8888);
    QPainter painter(&image);

    for (int i = 0; i < selectionDimensions.x(); i++) {
        for (int j = 0; j < selectionDimensions.y(); j++) {
            int x = i * 16;
            int y = j * 16;
            int index = j * selectionDimensions.x() + i;
            QImage metatile_image = getMetatileImage(
                        selectedMetatiles->at(index),
                        map->layout->tileset_primary,
                        map->layout->tileset_secondary,
                        map->metatileLayerOrder,
                        map->metatileLayerOpacity);
            QPoint metatile_origin = QPoint(x, y);
            painter.drawImage(metatile_origin, metatile_image);
        }
    }

    painter.end();
    setPixmap(QPixmap::fromImage(image));
}
