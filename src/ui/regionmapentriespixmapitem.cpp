#include "regionmapentriespixmapitem.h"

void RegionMapEntriesPixmapItem::draw() {
    if (!region_map) return;

    MapSectionEntry entry = this->region_map->getEntry(currentSection);
    bool selectingEntry = false;

    int entry_x, entry_y, entry_w, entry_h;

    if (!entry.valid || entry.name == region_map->default_map_section) {
        entry_x = entry_y = 0;
        entry_w = entry_h = 1;
    } else {
        selectingEntry = true;
        entry_x = entry.x, entry_y = entry.y;
        entry_w = entry.width, entry_h = entry.height;
    }

    QImage image(region_map->tilemapWidth() * 8, region_map->tilemapHeight() * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < region_map->tilemapSize(); i++) {
        QImage bottom_img = this->tile_selector->tileImg(region_map->getTile(i));
        QImage top_img(8, 8, QImage::Format_RGBA8888);
        int x = i % region_map->tilemapWidth();
        int y = i / region_map->tilemapWidth();
        bool insideEntry = false;
        if (selectingEntry) {
            if (x == entry_x + this->region_map->padLeft() && y == entry_y + this->region_map->padTop())
                insideEntry = true;
            else if (x - this->region_map->padLeft() - entry_x < entry_w && x >= entry_x + this->region_map->padLeft()
                  && y - this->region_map->padTop() - entry_y < entry_h && y >= entry_y + this->region_map->padTop())
                insideEntry = true;
        }
        if (insideEntry) {
            top_img.fill(QColor(255, 68, 68));
        } else if (region_map->squareHasMap(i)) {
            top_img.fill(Qt::gray);
        } else {
            top_img.fill(Qt::black);
        }
        QPoint pos = QPoint(x * 8, y * 8);
        painter.setOpacity(1);
        painter.drawImage(pos, bottom_img);
        painter.save();
        painter.setOpacity(0.65);
        painter.drawImage(pos, top_img);
        painter.restore();
    }
    painter.end();

    this->selectionOffsetX = entry_w - 1;
    this->selectionOffsetY = entry_h - 1;

    this->setPixmap(QPixmap::fromImage(image));

    if (selectingEntry) this->drawSelection();
}

void RegionMapEntriesPixmapItem::select(int x, int y) {
    int index = this->region_map->getMapSquareIndex(x, y);
    SelectablePixmapItem::select(x, y, 0, 0);
    this->selectedTile = index;
    this->updateSelectedTile();

    emit selectedTileChanged(this->region_map->squareMapSection(index));
}

void RegionMapEntriesPixmapItem::select(int index) {
    int x = index % this->region_map->tilemapWidth();
    int y = index / this->region_map->tilemapWidth();
    SelectablePixmapItem::select(x, y, 0, 0);
    this->selectedTile = index;
    this->updateSelectedTile();

    emit selectedTileChanged(this->region_map->squareMapSection(index));
}

void RegionMapEntriesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    int x = pos.x() - this->region_map->padLeft();
    int y = pos.y() - this->region_map->padTop();

    MapSectionEntry entry = this->region_map->getEntry(currentSection);
    pressedX = x - entry.x;
    pressedY = y - entry.y;
    if (entry.x == x && entry.y == y) {
        this->draggingEntry = true;
    }
    else if (pressedX < entry.width && x >= entry.x && pressedY < entry.height && y >= entry.y) {
        this->draggingEntry = true;
    }
}

// TODO: update this function, especially bounds check
void RegionMapEntriesPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!draggingEntry) {
        event->ignore();
        return;
    }

    QPoint pos = this->getCellPos(event->pos());
    int new_x = pos.x() - this->region_map->padLeft() - pressedX;
    int new_y = pos.y() - this->region_map->padTop() - pressedY;

    MapSectionEntry entry = this->region_map->getEntry(currentSection);

    // check to make sure not moving out of bounds
    if (new_x + entry.width > this->region_map->tilemapWidth() - this->region_map->padLeft() //- this->region_map->padRight
     || new_y + entry.height > this->region_map->tilemapHeight() - this->region_map->padTop() //- this->region_map->padBottom
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
