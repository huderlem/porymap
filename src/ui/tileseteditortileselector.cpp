#include "tileseteditortileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>

void TilesetEditorTileSelector::draw() {
    if (!this->primaryTileset || !this->primaryTileset->tiles
     || !this->secondaryTileset || !this->secondaryTileset->tiles) {
        this->setPixmap(QPixmap());
    }

    int totalTiles = Project::getNumTilesTotal();
    int primaryLength = this->primaryTileset->tiles->length();
    int secondaryLength = this->secondaryTileset->tiles->length();
    int height = totalTiles / this->numTilesWide;
    QList<QRgb> palette = this->getCurPaletteTable();
    QImage image(this->numTilesWide * 16, height * 16, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (uint16_t tile = 0; tile < totalTiles; tile++) {
        QImage tileImage;
        if (tile < primaryLength) {
            tileImage = getColoredTileImage(tile, this->primaryTileset, this->secondaryTileset, palette).scaled(16, 16);
        } else if (tile < Project::getNumTilesPrimary()) {
            tileImage = QImage(16, 16, QImage::Format_RGBA8888);
            tileImage.fill(palette.at(0));
        } else if (tile < Project::getNumTilesPrimary() + secondaryLength) {
            tileImage = getColoredTileImage(tile, this->primaryTileset, this->secondaryTileset, palette).scaled(16, 16);
        } else {
            tileImage = QImage(16, 16, QImage::Format_RGBA8888);
            QPainter painter(&tileImage);
            painter.fillRect(0, 0, 16, 16, palette.at(0));
        }

        int y = tile / this->numTilesWide;
        int x = tile % this->numTilesWide;
        QPoint origin = QPoint(x * 16, y * 16);
        painter.drawImage(origin, tileImage);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
    this->drawSelection();
}

QList<QRgb> TilesetEditorTileSelector::getCurPaletteTable() {
    QList<QRgb> paletteTable;
    for (int i = 0; i < this->primaryTileset->palettes->at(this->paletteNum).length(); i++) {
        paletteTable.append(this->primaryTileset->palettes->at(this->paletteNum).at(i));
    }

    return paletteTable;
}

void TilesetEditorTileSelector::select(uint16_t tile) {
    QPoint coords = this->getTileCoords(tile);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->selectedTile = tile;
    emit selectedTileChanged(tile);
}

void TilesetEditorTileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

void TilesetEditorTileSelector::setPaletteNum(int paletteNum) {
    this->paletteNum = paletteNum;
    this->draw();
}

void TilesetEditorTileSelector::updateSelectedTile() {
    QPoint origin = this->getSelectionStart();
    this->selectedTile = this->getTileId(origin.x(), origin.y());
}

uint16_t TilesetEditorTileSelector::getSelectedTile() {
    return this->selectedTile;
}

uint16_t TilesetEditorTileSelector::getTileId(int x, int y) {
    return static_cast<uint16_t>(y * this->numTilesWide + x);
}

void TilesetEditorTileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}

void TilesetEditorTileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedTile();

    QPoint pos = this->getCellPos(event->pos());
    uint16_t tile = this->getTileId(pos.x(), pos.y());
    emit hoveredTileChanged(tile);
    emit selectedTileChanged(tile);
}

void TilesetEditorTileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}

void TilesetEditorTileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t tile = this->getTileId(pos.x(), pos.y());
    emit this->hoveredTileChanged(tile);
}

void TilesetEditorTileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredTileCleared();
}

QPoint TilesetEditorTileSelector::getTileCoords(uint16_t tile) {
    if (tile >= Project::getNumTilesTotal())
    {
        // Invalid tile.
        return QPoint(0, 0);
    }

    return QPoint(tile % this->numTilesWide, tile / this->numTilesWide);
}
