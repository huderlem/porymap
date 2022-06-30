#include "regionmappixmapitem.h"
#include "regionmapeditcommands.h"

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
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = x + y * region_map->tilemapWidth();
        this->region_map->setTileData(index, 
                this->tile_selector->selectedTile,
                this->tile_selector->tile_hFlip,
                this->tile_selector->tile_vFlip,
                this->tile_selector->tile_palette
            );
        draw();
    }
}

void RegionMapPixmapItem::floodFill(int x, int y, std::shared_ptr<TilemapTile> oldTile, std::shared_ptr<TilemapTile> newTile) {
    // out of bounds
    if (x < 0
     || y < 0
     || x >= this->region_map->tilemapWidth()
     || y >= this->region_map->tilemapHeight()) {
        return;
    }

    auto tile = this->region_map->getTile(x, y);
    if (!tile->operator==(*oldTile) || tile->operator==(*newTile)) {
        return;
    }

    int index = x + y * this->region_map->tilemapWidth();
    this->region_map->setTileData(index,
        newTile->id(),
        newTile->hFlip(),
        newTile->vFlip(),
        newTile->palette()
    );

    floodFill(x + 1, y, oldTile, newTile);
    floodFill(x - 1, y, oldTile, newTile);
    floodFill(x, y + 1, oldTile, newTile);
    floodFill(x, y - 1, oldTile, newTile);
}

void RegionMapPixmapItem::fill(QGraphicsSceneMouseEvent *event) {
    if (region_map) {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = x + y * this->region_map->tilemapWidth();
        std::shared_ptr<TilemapTile> currentTile, newTile, oldTile;
        currentTile = this->region_map->getTile(index);
        switch(this->tile_selector->format) {
            case TilemapFormat::Plain:
                oldTile = std::make_shared<PlainTile>(currentTile->raw());
                newTile = std::make_shared<PlainTile>(currentTile->raw());
                newTile->setId(this->tile_selector->selectedTile);
                break;
            case TilemapFormat::BPP_4:
                oldTile = std::make_shared<BPP4Tile>(currentTile->raw());
                newTile = std::make_shared<BPP4Tile>(currentTile->raw());
                newTile->setId(this->tile_selector->selectedTile);
                newTile->setHFlip(this->tile_selector->tile_hFlip);
                newTile->setVFlip(this->tile_selector->tile_vFlip);
                newTile->setPalette(this->tile_selector->tile_palette);
                break;
            case TilemapFormat::BPP_8:
                oldTile = std::make_shared<BPP8Tile>(currentTile->raw());
                newTile = std::make_shared<BPP8Tile>(currentTile->raw());
                newTile->setId(this->tile_selector->selectedTile);
                newTile->setId(this->tile_selector->selectedTile);
                newTile->setHFlip(this->tile_selector->tile_hFlip);
                newTile->setVFlip(this->tile_selector->tile_vFlip);
                break;
        }
        floodFill(x, y, oldTile, newTile);
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
    if (x < this->region_map->tilemapWidth()  && x >= 0 
     && y < this->region_map->tilemapHeight() && y >= 0) {
        emit this->hoveredRegionMapTileChanged(x, y);
        emit mouseEvent(event, this);
    }
}

void RegionMapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
