#include "regionmappixmapitem.h"
#include "regionmapeditcommands.h"

static unsigned actionId_ = 0;

void RegionMapPixmapItem::draw() {
    if (!region_map) return;

    QImage image(region_map->pixelWidth(), region_map->pixelHeight(), QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < region_map->tilemapSize(); i++) {
        QImage img = this->tile_selector->tileImg(region_map->getTile(i));
        int x = i % region_map->tilemapWidth();
        int y = i / region_map->tilemapWidth();
        QPoint pos = QPoint(x * 8, y * 8);
        painter.drawImage(pos, img);
    }
    painter.end();

    this->setPixmap(QPixmap::fromImage(image));
}

void RegionMapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (region_map) {
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            actionId_++;
        } else {
            QPointF pos = event->pos();
            int x = static_cast<int>(pos.x()) / 8;
            int y = static_cast<int>(pos.y()) / 8;
            int index = x + y * region_map->tilemapWidth();
            QByteArray oldTilemap = this->region_map->getTilemap();
            this->region_map->setTileData(index, 
                    this->tile_selector->selectedTile,
                    this->tile_selector->tile_hFlip,
                    this->tile_selector->tile_vFlip,
                    this->tile_selector->tile_palette
                );
            QByteArray newTilemap = this->region_map->getTilemap();
            EditTilemap *command = new EditTilemap(this->region_map, oldTilemap, newTilemap, actionId_);
            this->region_map->commit(command);
            draw();
        }
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
    if (x < this->region_map->tilemapWidth()  && x >= 0 
     && y < this->region_map->tilemapHeight() && y >= 0) {
        emit this->hoveredRegionMapTileChanged(x, y);
        emit mouseEvent(event, this);
    }
}

void RegionMapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
