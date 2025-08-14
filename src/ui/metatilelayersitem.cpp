#include "config.h"
#include "metatilelayersitem.h"
#include "imageproviders.h"
#include <QPainter>

MetatileLayersItem::MetatileLayersItem(Metatile *metatile, Tileset *primaryTileset, Tileset *secondaryTileset, Qt::Orientation orientation)
    : SelectablePixmapItem(16, 16, Metatile::tileWidth(),  Metatile::tileHeight()),
     metatile(metatile),
     primaryTileset(primaryTileset),
     secondaryTileset(secondaryTileset)
{
    setAcceptHoverEvents(true);
    setOrientation(orientation);
}

void MetatileLayersItem::setOrientation(Qt::Orientation orientation) {
    if (this->orientation == orientation)
        return;
    this->orientation = orientation;
    int maxWidth = Metatile::tileWidth();
    int maxHeight = Metatile::tileHeight();

    // Generate a table of tile positions that allows us to map between
    // the index of a tile in the metatile and its position in this layer view.
    this->tilePositions.clear();
    if (this->orientation == Qt::Horizontal) {
        // Tiles are laid out horizontally, with the bottom layer on the left:
        //  0  1   4  5   8  9
        //  2  3   6  7  10 11
        for (int layer = 0; layer < projectConfig.getNumLayersInMetatile(); layer++)
        for (int y = 0; y < Metatile::tileHeight(); y++)
        for (int x = 0; x < Metatile::tileWidth(); x++) {
            this->tilePositions.append(QPoint(x + layer * Metatile::tileWidth(), y));
        }
        maxWidth *= projectConfig.getNumLayersInMetatile();
    } else if (this->orientation == Qt::Vertical) {
        // Tiles are laid out vertically, with the bottom layer on the bottom:
        //  8  9
        // 10 11
        //  4  5
        //  6  7
        //  0  1
        //  2  3
        for (int layer = projectConfig.getNumLayersInMetatile() - 1; layer >= 0; layer--)
        for (int y = 0; y < Metatile::tileHeight(); y++)
        for (int x = 0; x < Metatile::tileWidth(); x++) {
            this->tilePositions.append(QPoint(x, y + layer * Metatile::tileHeight()));
        }
        maxHeight *= projectConfig.getNumLayersInMetatile();
    }
    setMaxSelectionSize(maxWidth, maxHeight);
    update();
    if (!this->pixmap().isNull()) {
        draw();
    }
}

void MetatileLayersItem::draw() {
    QPixmap pixmap(this->cellWidth * this->maxSelectionWidth, this->cellHeight * this->maxSelectionHeight);
    QPainter painter(&pixmap);

    // Draw tile images
    int numTiles = qMin(projectConfig.getNumTilesInMetatile(), this->metatile ? this->metatile->tiles.length() : 0);
    for (int i = 0; i < numTiles; i++) {
        Tile tile = this->metatile->tiles.at(i);
        QImage tileImage = getPalettedTileImage(tile.tileId,
                                                this->primaryTileset,
                                                this->secondaryTileset,
                                                tile.palette,
                                                true
                                                ).scaled(this->cellWidth, this->cellHeight);
        tile.flip(&tileImage);
        QPoint pos = tileIndexToPos(i);
        painter.drawImage(pos.x() * this->cellWidth, pos.y() * this->cellHeight, tileImage);
    }
    if (this->showGrid) {
        // Draw grid
        painter.setPen(Qt::white);
        const int layerWidth = this->cellWidth * Metatile::tileWidth();
        const int layerHeight = this->cellHeight * Metatile::tileHeight();
        for (int i = 1; i < projectConfig.getNumLayersInMetatile(); i++) {
            if (this->orientation == Qt::Vertical) {
                int y = i * layerHeight;
                painter.drawLine(0, y, layerWidth, y);
            } else if (this->orientation == Qt::Horizontal) {
                int x = i * layerWidth;
                painter.drawLine(x, 0, x, layerHeight);
            }
        }
    }

    this->setPixmap(pixmap);
}

void MetatileLayersItem::setMetatile(Metatile *metatile) {
    this->metatile = metatile;
    draw();
}

void MetatileLayersItem::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

void MetatileLayersItem::updateSelection() {
    drawSelection();
    emit selectedTilesChanged(getSelectionStart(), getSelectionDimensions());
}

void MetatileLayersItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    const QPoint pos = getBoundedPos(event->pos());
    setCursorCellPos(pos);

    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mousePressEvent(event);
        updateSelection();
    } else if (event->modifiers() & Qt::ControlModifier) {
        emit paletteChanged(pos);
    } else {
        emit tileChanged(pos);
    }
}

void MetatileLayersItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    const QPoint pos = getBoundedPos(event->pos());
    if (!setCursorCellPos(pos))
        return;

    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mouseMoveEvent(event);
        updateSelection();
    } else if (event->modifiers() & Qt::ControlModifier) {
        emit paletteChanged(pos);
    } else {
        emit tileChanged(pos);
    }
}

void MetatileLayersItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mouseReleaseEvent(event);
        updateSelection();
    }

    // Clear selection rectangle
    draw();
}

void MetatileLayersItem::hoverMoveEvent(QGraphicsSceneHoverEvent * event) {
    setCursorCellPos(getBoundedPos(event->pos()));
}

bool MetatileLayersItem::setCursorCellPos(const QPoint &pos) {
    if (this->cursorCellPos == pos)
        return false;
    this->cursorCellPos = pos;

    emit this->hoveredTileChanged(tileUnderCursor());
    return true;
}

void MetatileLayersItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    this->cursorCellPos = QPoint(-1,-1);
    emit this->hoveredTileCleared();
}

Tile MetatileLayersItem::tileUnderCursor() const {
    int tileIndex = posToTileIndex(this->cursorCellPos);
    if (tileIndex < 0 || !this->metatile || tileIndex >= this->metatile->tiles.length()) {
        return Tile();
    }
    return this->metatile->tiles.at(tileIndex);
}

QPoint MetatileLayersItem::getBoundedPos(const QPointF &pos) {
    int x = static_cast<int>(pos.x()) / this->cellWidth;
    int y = static_cast<int>(pos.y()) / this->cellHeight;
    return QPoint(qBound(0, x, this->maxSelectionWidth - 1),
                  qBound(0, y, this->maxSelectionHeight - 1));
}
