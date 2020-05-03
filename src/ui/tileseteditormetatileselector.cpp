#include "tileseteditormetatileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>

void TilesetEditorMetatileSelector::draw() {
    if (!this->primaryTileset || !this->primaryTileset->metatiles
     || !this->secondaryTileset || !this->secondaryTileset->metatiles) {
        this->setPixmap(QPixmap());
    }

    int primaryLength = this->primaryTileset->metatiles->length();
    int length_ = primaryLength + this->secondaryTileset->metatiles->length();
    int height_ = length_ / this->numMetatilesWide;
    if (length_ % this->numMetatilesWide != 0) {
        height_++;
    }
    QImage image(this->numMetatilesWide * 32, height_ * 32, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        QImage metatile_image = getMetatileImage(tile, this->primaryTileset, this->secondaryTileset, true).scaled(32, 32);
        int map_y = i / this->numMetatilesWide;
        int map_x = i % this->numMetatilesWide;
        QPoint metatile_origin = QPoint(map_x * 32, map_y * 32);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
    this->drawSelection();
}

void TilesetEditorMetatileSelector::select(uint16_t metatileId) {
    metatileId = this->getValidMetatileId(metatileId);
    QPoint coords = this->getMetatileIdCoords(metatileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->selectedMetatile = metatileId;
    emit selectedMetatileChanged(metatileId);
}

void TilesetEditorMetatileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

void TilesetEditorMetatileSelector::updateSelectedMetatile() {
    QPoint origin = this->getSelectionStart();
    this->selectedMetatile = this->getValidMetatileId(this->getMetatileId(origin.x(), origin.y()));
}

uint16_t TilesetEditorMetatileSelector::getSelectedMetatile() {
    return this->selectedMetatile;
}

uint16_t TilesetEditorMetatileSelector::getMetatileId(int x, int y) {
    int index = y * this->numMetatilesWide + x;
    if (index < this->primaryTileset->metatiles->length()) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - this->primaryTileset->metatiles->length());
    }
}

void TilesetEditorMetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedMetatile();
    emit selectedMetatileChanged(this->selectedMetatile);
}

void TilesetEditorMetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedMetatile();
    emit hoveredMetatileChanged(this->selectedMetatile);
    emit selectedMetatileChanged(this->selectedMetatile);
}

void TilesetEditorMetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedMetatile();
    emit selectedMetatileChanged(this->selectedMetatile);
}

void TilesetEditorMetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t metatileId = this->getMetatileId(pos.x(), pos.y());
    emit this->hoveredMetatileChanged(metatileId);
}

void TilesetEditorMetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileCleared();
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->primaryTileset->metatiles->length();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}

uint16_t TilesetEditorMetatileSelector::getValidMetatileId(uint16_t metatileId) {
    if (metatileId >= Project::getNumMetatilesTotal()
       || (metatileId < Project::getNumMetatilesPrimary() && metatileId >= this->primaryTileset->metatiles->length())
       || (metatileId < Project::getNumMetatilesTotal() && metatileId >= Project::getNumMetatilesPrimary() + this->secondaryTileset->metatiles->length()))
    {
        return 0;
    }
    return metatileId;
}

