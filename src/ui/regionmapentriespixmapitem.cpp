#include "regionmapentriespixmapitem.h"

void RegionMapEntriesPixmapItem::draw() {
    if (!region_map) return;

    RegionMapEntry entry = region_map->mapSecToMapEntry.value(currentSection);
    bool selectingEntry = false;

    int entry_x, entry_y, entry_w, entry_h;

    if (entry.name.isEmpty() || entry.name == "MAPSEC_NONE") {
        entry_x = entry_y = 0;
        entry_w = entry_h = 1;
    } else {
        selectingEntry = true;
        entry_x = entry.x, entry_y = entry.y;
        entry_w = entry.width, entry_h = entry.height;
    }

    QImage image(region_map->width() * 8, region_map->height() * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < region_map->map_squares.size(); i++) {
        QImage bottom_img = this->tile_selector->tileImg(region_map->map_squares[i].tile_img_id);
        QImage top_img(8, 8, QImage::Format_RGBA8888);
        int x = i % region_map->width();
        int y = i / region_map->width();
        bool insideEntry = false;
        if (selectingEntry) {
            if (x == entry_x + this->region_map->padLeft && y == entry_y + this->region_map->padTop)
                insideEntry = true;
            else if (x - this->region_map->padLeft - entry_x < entry_w && x >= entry_x + this->region_map->padLeft
                  && y - this->region_map->padTop - entry_y < entry_h && y >= entry_y + this->region_map->padTop)
                insideEntry = true;
        }
        if (insideEntry) {
            top_img.fill(QColor(255, 68, 68));
        } else if (region_map->map_squares[i].has_map) {
            top_img.fill(Qt::gray);
        } else {
            top_img.fill(Qt::black);
        }
        QPoint pos = QPoint(x * 8, y * 8);
        painter.setOpacity(1);
        painter.drawImage(pos, bottom_img);
        painter.save();
        painter.setOpacity(0.55);
        painter.drawImage(pos, top_img);
        painter.restore();
    }
    painter.end();

    this->selectionOffsetX = entry_w - 1;
    this->selectionOffsetY = entry_h - 1;

    this->setPixmap(QPixmap::fromImage(image));
    this->drawSelection();
}

void RegionMapEntriesPixmapItem::select(int x, int y) {
    int index = this->region_map->getMapSquareIndex(x, y);
    SelectablePixmapItem::select(x, y, 0, 0);
    this->selectedTile = index;
    this->updateSelectedTile();

    emit selectedTileChanged(this->region_map->map_squares[index].mapsec);
}

void RegionMapEntriesPixmapItem::select(int index) {
    int x = index % this->region_map->width();
    int y = index / this->region_map->width();
    SelectablePixmapItem::select(x, y, 0, 0);
    this->selectedTile = index;
    this->updateSelectedTile();

    emit selectedTileChanged(this->region_map->map_squares[index].mapsec);
}

void RegionMapEntriesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    int x = pos.x() - this->region_map->padLeft;
    int y = pos.y() - this->region_map->padTop;

    RegionMapEntry entry = this->region_map->mapSecToMapEntry.value(currentSection);
    pressedX = x - entry.x;
    pressedY = y - entry.y;
    if (entry.x == x && entry.y == y) {
        this->draggingEntry = true;
    }
    else if (pressedX < entry.width && x >= entry.x && pressedY < entry.height && y >= entry.y) {
        this->draggingEntry = true;
    }
}

void RegionMapEntriesPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!draggingEntry) {
        event->ignore();
        return;
    }

    QPoint pos = this->getCellPos(event->pos());
    int new_x = pos.x() - this->region_map->padLeft - pressedX;
    int new_y = pos.y() - this->region_map->padTop - pressedY;

    RegionMapEntry entry = this->region_map->mapSecToMapEntry.value(currentSection);

    // check to make sure not moving out of bounds
    if (new_x + entry.width > this->region_map->width() - this->region_map->padLeft - this->region_map->padRight
     || new_y + entry.height > this->region_map->height() - this->region_map->padTop - this->region_map->padBottom
     || new_x < 0 || new_y < 0)
        return;

    if (new_x != entry.x || new_y != entry.y) {
        emit entryPositionChanged(new_x, new_y);
    }
}

void RegionMapEntriesPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    this->draggingEntry = false;
}

void RegionMapEntriesPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    event->ignore();
}

void RegionMapEntriesPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    event->ignore();
}

void RegionMapEntriesPixmapItem::updateSelectedTile() {
    QPoint origin = this->getSelectionStart();
    this->selectedTile = this->region_map->getMapSquareIndex(origin.x(), origin.y());
    this->highlightedTile = -1;
    draw();
}
