#include "tileseteditortileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>
#include <QVector>

QPoint TilesetEditorTileSelector::getSelectionDimensions() {
    if (this->externalSelection) {
        return QPoint(this->externalSelectionWidth, this->externalSelectionHeight);
    } else {
        return SelectablePixmapItem::getSelectionDimensions();
    }
}

void TilesetEditorTileSelector::draw() {
    if (!this->primaryTileset || !this->primaryTileset->tiles
     || !this->secondaryTileset || !this->secondaryTileset->tiles) {
        this->setPixmap(QPixmap());
    }

    int totalTiles = Project::getNumTilesTotal();
    int primaryLength = this->primaryTileset->tiles->length();
    int secondaryLength = this->secondaryTileset->tiles->length();
    int height = totalTiles / this->numTilesWide;
    QList<QRgb> palette = Tileset::getPalette(this->paletteId, this->primaryTileset, this->secondaryTileset);
    QImage image(this->numTilesWide * 16, height * 16, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (uint16_t tile = 0; tile < totalTiles; tile++) {
        QImage tileImage;
        if (tile < primaryLength) {
            tileImage = getColoredTileImage(tile, this->primaryTileset, this->secondaryTileset, this->paletteId).scaled(16, 16);
        } else if (tile < Project::getNumTilesPrimary()) {
            tileImage = QImage(16, 16, QImage::Format_RGBA8888);
            tileImage.fill(palette.at(0));
        } else if (tile < Project::getNumTilesPrimary() + secondaryLength) {
            tileImage = getColoredTileImage(tile, this->primaryTileset, this->secondaryTileset, this->paletteId).scaled(16, 16);
        } else {
            tileImage = QImage(16, 16, QImage::Format_RGBA8888);
            QPainter painter(&tileImage);
            painter.fillRect(0, 0, 16, 16, palette.at(0));
        }

        int y = tile / this->numTilesWide;
        int x = tile % this->numTilesWide;
        QPoint origin = QPoint(x * 16, y * 16);
        painter.drawImage(origin, tileImage.mirrored(this->xFlip, this->yFlip));
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));

    if (!this->externalSelection) {
        this->drawSelection();
    }
}

void TilesetEditorTileSelector::select(uint16_t tile) {
    this->externalSelection = false;
    QPoint coords = this->getTileCoords(tile);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->updateSelectedTiles();
    emit selectedTilesChanged();
}

void TilesetEditorTileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

void TilesetEditorTileSelector::setPaletteId(int paletteId) {
    this->paletteId = paletteId;
    this->draw();
}

void TilesetEditorTileSelector::setTileFlips(bool xFlip, bool yFlip) {
    this->xFlip = xFlip;
    this->yFlip = yFlip;
    this->draw();
}

void TilesetEditorTileSelector::updateSelectedTiles() {
    this->externalSelection = false;
    this->selectedTiles.clear();
    QPoint origin = this->getSelectionStart();
    QPoint dimensions = this->getSelectionDimensions();
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            uint16_t metatileId = this->getTileId(origin.x() + i, origin.y() + j);
            this->selectedTiles.append(metatileId);
        }
    }
}

QList<Tile> TilesetEditorTileSelector::getSelectedTiles() {
    if (this->externalSelection) {
        return this->externalSelectedTiles;
    } else {
        QList<Tile> tiles;
        for (uint16_t tile : this->selectedTiles) {
            tiles.append(Tile(tile, this->xFlip, this->yFlip, this->paletteId));
        }
        return tiles;
    }
}

void TilesetEditorTileSelector::setExternalSelection(int width, int height, QList<Tile> tiles) {
    this->externalSelection = true;
    this->externalSelectionWidth = width;
    this->externalSelectionHeight = height;
    this->externalSelectedTiles.clear();
    for (int i = 0; i < tiles.length(); i++) {
        this->externalSelectedTiles.append(tiles.at(i));
    }

    this->draw();
    emit selectedTilesChanged();
}

uint16_t TilesetEditorTileSelector::getTileId(int x, int y) {
    return static_cast<uint16_t>(y * this->numTilesWide + x);
}

void TilesetEditorTileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedTiles();
    emit selectedTilesChanged();
}

void TilesetEditorTileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedTiles();

    QPoint pos = this->getCellPos(event->pos());
    uint16_t tile = this->getTileId(pos.x(), pos.y());
    emit hoveredTileChanged(tile);
    emit selectedTilesChanged();
}

void TilesetEditorTileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedTiles();
    emit selectedTilesChanged();
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

QPoint TilesetEditorTileSelector::getTileCoordsOnWidget(uint16_t tile) {
    QPoint pos = getTileCoords(tile);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}

QImage TilesetEditorTileSelector::buildPrimaryTilesIndexedImage() {
    if (!this->primaryTileset || !this->primaryTileset->tiles
     || !this->secondaryTileset || !this->secondaryTileset->tiles) {
        return QImage();
    }

    int primaryLength = this->primaryTileset->tiles->length();
    int height = primaryLength / this->numTilesWide;
    QList<QRgb> palette = Tileset::getPalette(this->paletteId, this->primaryTileset, this->secondaryTileset);
    QImage image(this->numTilesWide * 8, height * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (uint16_t tile = 0; tile < primaryLength; tile++) {
        QImage tileImage;
        if (tile < primaryLength) {
            tileImage = getColoredTileImage(tile, this->primaryTileset, this->secondaryTileset, this->paletteId);
        } else {
            tileImage = QImage(8, 8, QImage::Format_RGBA8888);
            tileImage.fill(palette.at(0));
        }

        int y = tile / this->numTilesWide;
        int x = tile % this->numTilesWide;
        QPoint origin = QPoint(x * 8, y * 8);
        painter.drawImage(origin, tileImage.mirrored(this->xFlip, this->yFlip));
    }

    painter.end();

    QVector<QRgb> colorTable = palette.toVector();
    return image.convertToFormat(QImage::Format::Format_Indexed8, colorTable);
}

QImage TilesetEditorTileSelector::buildSecondaryTilesIndexedImage() {
    if (!this->primaryTileset || !this->primaryTileset->tiles
     || !this->secondaryTileset || !this->secondaryTileset->tiles) {
        return QImage();
    }

    int secondaryLength = this->secondaryTileset->tiles->length();
    int height = secondaryLength / this->numTilesWide;
    QList<QRgb> palette = Tileset::getPalette(this->paletteId, this->primaryTileset, this->secondaryTileset);
    QImage image(this->numTilesWide * 8, height * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    uint16_t primaryLength = static_cast<uint16_t>(Project::getNumTilesPrimary());
    for (uint16_t tile = 0; tile < secondaryLength; tile++) {
        QImage tileImage;
        if (tile < secondaryLength) {
            tileImage = getColoredTileImage(tile + primaryLength, this->primaryTileset, this->secondaryTileset, this->paletteId);
        } else {
            tileImage = QImage(8, 8, QImage::Format_RGBA8888);
            tileImage.fill(palette.at(0));
        }

        int y = tile / this->numTilesWide;
        int x = tile % this->numTilesWide;
        QPoint origin = QPoint(x * 8, y * 8);
        painter.drawImage(origin, tileImage.mirrored(this->xFlip, this->yFlip));
    }

    painter.end();

    QVector<QRgb> colorTable = palette.toVector();
    return image.convertToFormat(QImage::Format::Format_Indexed8, colorTable);
}
