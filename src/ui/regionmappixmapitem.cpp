#include "regionmappixmapitem.h"

void RegionMapPixmapItem::draw() {
    if (!region_map) return;

    QImage image(region_map->width() * 8, region_map->height() * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < region_map->map_squares.size(); i++) {
        QImage img = this->tile_selector->tileImg(region_map->map_squares[i].tile_img_id);
        int x = i % region_map->width();
        int y = i / region_map->width();
        QPoint pos = QPoint(x * 8, y * 8);
        painter.drawImage(pos, img);
    }
    painter.end();

    this->setPixmap(QPixmap::fromImage(image));
}

void RegionMapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (region_map) {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = x + y * region_map->width();
        this->region_map->map_squares[index].tile_img_id = this->tile_selector->selectedTile;
        draw();
    }
}

void RegionMapPixmapItem::select(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    this->tile_selector->select(this->region_map->getTileId(x, y));
}

void RegionMapPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    int x = static_cast<int>(event->pos().x()) / 8;
    int y = static_cast<int>(event->pos().y()) / 8;
    emit this->hoveredRegionMapTileChanged(x, y);
}

void RegionMapPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit this->hoveredRegionMapTileCleared();
}

void RegionMapPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void RegionMapPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    if (x < this->region_map->width()  && x >= 0 
     && y < this->region_map->height() && y >= 0) {
        emit this->hoveredRegionMapTileChanged(x, y);
        emit mouseEvent(event, this);
    }
}

void RegionMapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
