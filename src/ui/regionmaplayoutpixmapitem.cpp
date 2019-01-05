#include "regionmaplayoutpixmapitem.h"



// TODO: make this connected (by duplicating scene rect maybe?) to background image tab
//
void RegionMapLayoutPixmapItem::draw() {
    if (!region_map) return;

    QImage image(region_map->width() * 8, region_map->height() * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < region_map->map_squares.size(); i++) {
        QImage bottom_img = this->tile_selector->tileImg(region_map->map_squares[i].tile_img_id);
        QImage top_img(8, 8, QImage::Format_RGBA8888);
        if (region_map->map_squares[i].has_map) {
            top_img.fill(Qt::gray);
        } else {
            top_img.fill(Qt::black);
        }
        int x = i % region_map->width();
        int y = i / region_map->width();
        QPoint pos = QPoint(x * 8, y * 8);
        painter.setOpacity(1);
        painter.drawImage(pos, bottom_img);
        painter.save();
        painter.setOpacity(0.55);
        painter.drawImage(pos, top_img);
        painter.restore();
    }
    painter.end();

    this->setPixmap(QPixmap::fromImage(image));
    this->drawSelection();
}

void RegionMapLayoutPixmapItem::setDefaultSelection() {
    this->select(1,2);
}

void RegionMapLayoutPixmapItem::select(int x, int y) {
    int index = this->region_map->getMapSquareIndex(x, y);
    SelectablePixmapItem::select(x, y, 0, 0);
    this->selectedTile = index;
    this->updateSelectedTile();

    emit selectedTileChanged(index);
}

void RegionMapLayoutPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    int index = this->region_map->getMapSquareIndex(pos.x(), pos.y());
    if (this->region_map->map_squares[index].x >= 0
     && this->region_map->map_squares[index].y >= 0) {
        SelectablePixmapItem::mousePressEvent(event);
        this->updateSelectedTile();
        emit selectedTileChanged(this->selectedTile);
    }
}

void RegionMapLayoutPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedTile();
    emit hoveredTileChanged(this->selectedTile);
    emit selectedTileChanged(this->selectedTile);
}

void RegionMapLayoutPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    //
}

void RegionMapLayoutPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    int tileId = this->region_map->getMapSquareIndex(pos.x(), pos.y());
    emit this->hoveredTileChanged(tileId);
}

void RegionMapLayoutPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    emit this->hoveredTileCleared();
}

void RegionMapLayoutPixmapItem::updateSelectedTile() {
    QPoint origin = this->getSelectionStart();
    this->selectedTile = this->region_map->getMapSquareIndex(origin.x(), origin.y());
}
