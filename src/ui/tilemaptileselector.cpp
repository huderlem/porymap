#include "tilemaptileselector.h"

#include <QDebug>

void TilemapTileSelector::draw() {
    size_t width_  = this->pixmap.width();
    this->pixelWidth = width_;
    size_t height_ = this->pixmap.height();
    this->pixelHeight = height_;
    size_t ntiles_ = (width_/8) * (height_/8);// length_

    this->numTilesWide = width_ / 8;
    this->numTiles     = ntiles_;

    this->setPixmap(this->pixmap);
    this->drawSelection();
}

//*
void TilemapTileSelector::select(unsigned tileId) {
    QPoint coords = this->getTileIdCoords(tileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->selectedTile = tileId;
    emit selectedTileChanged(tileId);
}
//*/

//*
void TilemapTileSelector::updateSelectedTile() {
    QPoint origin = this->getSelectionStart();
    this->selectedTile = this->getTileId(origin.x(), origin.y());
}
//*/

unsigned TilemapTileSelector::getSelectedTile() {
    return this->selectedTile;
}

//*
unsigned TilemapTileSelector::getTileId(int x, int y) {
    int index = y * this->numTilesWide + x;
    return index < this->numTiles ? index : this->numTiles % index;
}
//*/

//*
void TilemapTileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}
//*/

//*
void TilemapTileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedTile();
    emit hoveredTileChanged(this->selectedTile);
    emit selectedTileChanged(this->selectedTile);
}
//*/

//*
void TilemapTileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}
//*/

//*
void TilemapTileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    unsigned tileId = this->getTileId(pos.x(), pos.y());
    emit this->hoveredTileChanged(tileId);
}
//*/

//*
void TilemapTileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    emit this->hoveredTileCleared();
}
//*/

//*
QPoint TilemapTileSelector::getTileIdCoords(unsigned tileId) {
    int index = tileId < this->numTiles ? tileId : this->numTiles % tileId;// TODO: change this?
    return QPoint(index % this->numTilesWide, index / this->numTilesWide);// ? is this right?
}
//*/

QImage TilemapTileSelector::tileImg(unsigned tileId) {
    //
    QPoint pos = getTileIdCoords(tileId);
    return pixmap.copy(pos.x() * 8, pos.y() * 8, 8, 8).toImage();
}
















