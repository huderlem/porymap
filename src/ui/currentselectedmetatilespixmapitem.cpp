#include "currentselectedmetatilespixmapitem.h"
#include "imageproviders.h"
#include <QPainter>

QPixmap drawMetatileSelection(MetatileSelection selection, Map *map) {
    int width = selection.dimensions.x() * 16;
    int height = selection.dimensions.y() * 16;
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(QColor(0, 0, 0, 0));
    QPainter painter(&image);

    for (int i = 0; i < selection.dimensions.x(); i++) {
        for (int j = 0; j < selection.dimensions.y(); j++) {
            int x = i * 16;
            int y = j * 16;
            QPoint metatile_origin = QPoint(x, y);
            int index = j * selection.dimensions.x() + i;
            MetatileSelectionItem item = selection.metatileItems.at(index);
            if (item.enabled) {
                QImage metatile_image = getMetatileImage(
                            item.metatileId,
                            map->layout->tileset_primary,
                            map->layout->tileset_secondary,
                            map->metatileLayerOrder,
                            map->metatileLayerOpacity);
                painter.drawImage(metatile_origin, metatile_image);
            }
        }
    }

    painter.end();
    return QPixmap::fromImage(image);
}

void CurrentSelectedMetatilesPixmapItem::draw() {
    MetatileSelection selection = metatileSelector->getMetatileSelection();
    setPixmap(drawMetatileSelection(selection, this->map));
}
