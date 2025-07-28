#include "tileseteditortileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>
#include <QVector>

QSize TilesetEditorTileSelector::getSelectionDimensions() const {
    if (this->externalSelection) {
        return QSize(this->externalSelectionWidth, this->externalSelectionHeight);
    } else {
        return SelectablePixmapItem::getSelectionDimensions();
    }
}

void TilesetEditorTileSelector::updateBasePixmap() {
    if (!this->primaryTileset || !this->secondaryTileset || this->numTilesWide == 0) {
        this->basePixmap = QPixmap();
        return;
    }

    int totalTiles = Project::getNumTilesTotal();
    int height = totalTiles / this->numTilesWide;
    QImage image(this->numTilesWide * this->cellWidth, height * this->cellHeight, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (uint16_t tileId = 0; tileId < totalTiles; tileId++) {
        QImage tileImage = getPalettedTileImage(tileId, this->primaryTileset, this->secondaryTileset, this->paletteId, true)
                                                .scaled(this->cellWidth, this->cellHeight);
        int x = (tileId % this->numTilesWide) * this->cellWidth;
        int y = (tileId / this->numTilesWide) * this->cellHeight;
        painter.drawImage(x, y, tileImage);
    }

    if (this->showDivider) {
        int row = Util::roundUpToMultiple(Project::getNumTilesPrimary(), this->numTilesWide) / this->numTilesWide;
        const int y = row * this->cellHeight;
        painter.setPen(Qt::white);
        painter.drawLine(0, y, this->numTilesWide * this->cellWidth, y);
    }

    painter.end();
    this->basePixmap = QPixmap::fromImage(image);
}

void TilesetEditorTileSelector::draw() {
    if (this->basePixmap.isNull())
        updateBasePixmap();
    setPixmap(this->basePixmap);

    if (!this->externalSelection || (this->externalSelectionWidth == 1 && this->externalSelectionHeight == 1)) {
        this->drawSelection();
    }

    if (showUnused) drawUnused();
}

void TilesetEditorTileSelector::select(uint16_t tile) {
    this->externalSelection = false;
    this->highlight(tile);
    this->updateSelectedTiles();
    emit selectedTilesChanged();
}

void TilesetEditorTileSelector::highlight(uint16_t tile) {
    QPoint coords = this->getTileCoords(tile);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
}

void TilesetEditorTileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->updateBasePixmap();
    this->draw();
}

void TilesetEditorTileSelector::setPaletteId(int paletteId) {
    this->paletteId = paletteId;
    this->paletteChanged = true;
    this->updateBasePixmap();
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
    QSize dimensions = this->getSelectionDimensions();
    for (int j = 0; j < dimensions.height(); j++) {
        for (int i = 0; i < dimensions.width(); i++) {
            uint16_t metatileId = this->getTileId(origin.x() + i, origin.y() + j);
            this->selectedTiles.append(metatileId);
        }
    }
}

QList<Tile> TilesetEditorTileSelector::getSelectedTiles() {
    if (this->externalSelection) {
        return buildSelectedTiles(this->externalSelectionWidth, this->externalSelectionHeight, this->externalSelectedTiles);
    } else {
        QSize dimensions = this->getSelectionDimensions();
        QList<Tile> tiles;
        for (int i = 0; i < this->selectedTiles.length(); i++) {
            uint16_t tile = this->selectedTiles.at(i);
            tiles.append(Tile(tile, false, false, this->paletteId));
        }
        return buildSelectedTiles(dimensions.width(), dimensions.height(), tiles);
    }
}

QList<Tile> TilesetEditorTileSelector::buildSelectedTiles(int width, int height, QList<Tile> selected) {
    QList<Tile> tiles;
    QList<QList<Tile>> tileMatrix;
    for (int j = 0; j < height; j++) {
        QList<Tile> row;
        QList<Tile> layerRow;
        for (int i = 0; i < width; i++) {
            int index = i + j * width;
            Tile tile = selected.at(index);
            tile.xflip ^= this->xFlip;
            tile.yflip ^= this->yFlip;
            if (this->paletteChanged)
                tile.palette = this->paletteId;
            if (this->xFlip)
                layerRow.prepend(tile);
            else
                layerRow.append(tile);

            // If we've completed a layer row, or its the last tile of an incompletely
            // selected layer, then append the layer row to the full row
            // If not an external selection, treat the whole row as 1 "layer"
            if (i == width - 1 || (this->externalSelection && (this->externalSelectedPos.at(index) % 4) & 1)) {
                row.append(layerRow);
                layerRow.clear();
            }
        }
        if (this->yFlip)
            tileMatrix.prepend(row);
        else
            tileMatrix.append(row);
    }
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            tiles.append(tileMatrix.at(j).at(i));
        }
    }
    return tiles;
}

void TilesetEditorTileSelector::setExternalSelection(int width, int height, QList<Tile> tiles, QList<int> tileIdxs) {
    this->externalSelection = true;
    this->paletteChanged = false;
    this->externalSelectionWidth = width;
    this->externalSelectionHeight = height;
    this->externalSelectedTiles.clear();
    this->externalSelectedTiles.append(tiles);
    this->externalSelectedPos.clear();
    this->externalSelectedPos.append(tileIdxs);
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
    if (tile >= Project::getNumTilesTotal() || this->numTilesWide == 0)
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
    if (!this->primaryTileset)
        return QImage();

    return buildImage(0, this->primaryTileset->numTiles());
}

QImage TilesetEditorTileSelector::buildSecondaryTilesIndexedImage() {
    if (!this->secondaryTileset)
        return QImage();

    return buildImage(Project::getNumTilesPrimary(), this->secondaryTileset->numTiles());
}

QImage TilesetEditorTileSelector::buildImage(int tileIdStart, int numTiles) {
    if (this->numTilesWide == 0)
        return QImage();

    int height = qCeil(numTiles / static_cast<double>(this->numTilesWide));
    QImage image(this->numTilesWide * Tile::pixelWidth(), height * Tile::pixelHeight(), QImage::Format_RGBA8888);
    image.fill(0);

    QPainter painter(&image);
    for (int i = 0; i < numTiles; i++) {
        QImage tileImage = getGreyscaleTileImage(tileIdStart + i, this->primaryTileset, this->secondaryTileset);
        int x = (i % this->numTilesWide) * Tile::pixelWidth();
        int y = (i / this->numTilesWide) * Tile::pixelHeight();
        painter.drawImage(x, y, tileImage);
    }
    painter.end();

    // Image is first converted using greyscale so that palettes with duplicate colors
    // are properly represented in the final image.
    QImage indexedImage = image.convertToFormat(QImage::Format::Format_Indexed8, greyscalePalette.toVector());
    QList<QRgb> palette = Tileset::getPalette(this->paletteId, this->primaryTileset, this->secondaryTileset, true);
    indexedImage.setColorTable(palette.toVector());
    return indexedImage;
}

void TilesetEditorTileSelector::drawUnused() {
    // setup the circle with a line through it image to layer above unused metatiles
    QPixmap redX(16, 16);
    redX.fill(Qt::transparent);

    QPen whitePen(Qt::white);
    whitePen.setWidth(1);
    QPen pinkPen(Qt::magenta);
    pinkPen.setWidth(1);

    QPainter oPainter(&redX);

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(1, 1, 14, 14));
    oPainter.setPen(pinkPen);
    oPainter.drawEllipse(QRect(2, 2, 12, 12));
    oPainter.drawEllipse(QRect(3, 3, 10, 10));

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(4, 4, 8, 8));

    whitePen.setWidth(3);
    oPainter.setPen(whitePen);
    oPainter.drawLine(0, 0, 15, 15);

    pinkPen.setWidth(1);
    oPainter.setPen(pinkPen);
    oPainter.drawLine(2, 2, 13, 13);

    oPainter.end();

    // draw symbol on unused metatiles
    QPixmap tilesPixmap = this->pixmap();

    QPainter unusedPainter(&tilesPixmap);
    unusedPainter.setOpacity(0.5);

    for (int tile = 0; tile < this->usedTiles.size(); tile++) {
        if (!this->usedTiles[tile]) {
            unusedPainter.drawPixmap((tile % this->cellWidth) * this->cellWidth, (tile / this->cellWidth) * this->cellHeight, redX);
        }
    }

    unusedPainter.end();

    this->setPixmap(tilesPixmap);
}
