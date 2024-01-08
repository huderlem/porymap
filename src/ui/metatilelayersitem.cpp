#include "config.h"
#include "metatilelayersitem.h"
#include "imageproviders.h"
#include <QPainter>

static const QList<QPoint> tilePositions = {
    QPoint(0, 0),
    QPoint(1, 0),
    QPoint(0, 1),
    QPoint(1, 1),
    QPoint(2, 0),
    QPoint(3, 0),
    QPoint(2, 1),
    QPoint(3, 1),
    QPoint(4, 0),
    QPoint(5, 0),
    QPoint(4, 1),
    QPoint(5, 1),
};

void MetatileLayersItem::draw() {
    const int numLayers = projectConfig.getNumLayersInMetatile();
    QPixmap pixmap(numLayers * 32, 32);
    QPainter painter(&pixmap);

    // Draw tile images
    int numTiles = projectConfig.getNumTilesInMetatile();
    for (int i = 0; i < numTiles; i++) {
        Tile tile = this->metatile->tiles.at(i);
        QImage tileImage = getPalettedTileImage(tile.tileId, this->primaryTileset, this->secondaryTileset, tile.palette, true)
                .mirrored(tile.xflip, tile.yflip)
                .scaled(16, 16);
        painter.drawImage(tilePositions.at(i) * 16, tileImage);
    }
    if (this->showGrid) {
        // Draw grid
        painter.setPen(Qt::white);
        for (int i = 1; i < numLayers; i++) {
            int x = i * 32;
            painter.drawLine(x, 0, x, 32);
        }
    }

    this->setPixmap(pixmap);
}

void MetatileLayersItem::setMetatile(Metatile *metatile) {
    this->metatile = metatile;
    this->clearLastModifiedCoords();
    this->clearLastHoveredCoords();
}

void MetatileLayersItem::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
    this->clearLastModifiedCoords();
    this->clearLastHoveredCoords();
}

void MetatileLayersItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mousePressEvent(event);
        QPoint selectionOrigin = this->getSelectionStart();
        QPoint dimensions = this->getSelectionDimensions();
        emit this->selectedTilesChanged(selectionOrigin, dimensions.x(), dimensions.y());
        this->drawSelection();
    } else {
        const QPoint pos = this->getBoundedPos(event->pos());
        this->prevChangedPos = pos;
        this->clearLastHoveredCoords();
        emit this->tileChanged(pos.x(), pos.y());
    }
}

void MetatileLayersItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mouseMoveEvent(event);
        QPoint selectionOrigin = this->getSelectionStart();
        QPoint dimensions = this->getSelectionDimensions();
        emit this->selectedTilesChanged(selectionOrigin, dimensions.x(), dimensions.y());
        this->drawSelection();
    } else {
       const QPoint pos = this->getBoundedPos(event->pos());
        if (prevChangedPos != pos) {
            this->prevChangedPos = pos;
            this->clearLastHoveredCoords();
            emit this->tileChanged(pos.x(), pos.y());
        }
    }
}

void MetatileLayersItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mouseReleaseEvent(event);
        QPoint selectionOrigin = this->getSelectionStart();
        QPoint dimensions = this->getSelectionDimensions();
        emit this->selectedTilesChanged(selectionOrigin, dimensions.x(), dimensions.y());
    }

    this->draw();
}

void MetatileLayersItem::hoverMoveEvent(QGraphicsSceneHoverEvent * event) {
    const QPoint pos = this->getBoundedPos(event->pos());
    if (pos == this->prevHoveredPos)
        return;
    this->prevHoveredPos = pos;

    int tileIndex = tilePositions.indexOf(pos);
    if (tileIndex < 0 || tileIndex >= this->metatile->tiles.length())
        return;

    emit this->hoveredTileChanged(this->metatile->tiles.at(tileIndex).tileId);
}

void MetatileLayersItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    this->clearLastHoveredCoords();
    emit this->hoveredTileCleared();
}

void MetatileLayersItem::clearLastModifiedCoords() {
    this->prevChangedPos = QPoint(-1, -1);
}

void MetatileLayersItem::clearLastHoveredCoords() {
    this->prevHoveredPos = QPoint(-1, -1);
}

QPoint MetatileLayersItem::getBoundedPos(const QPointF &pos) {
    int x, y;
    int maxX = (projectConfig.getNumLayersInMetatile() * 2) - 1;
    x = static_cast<int>(pos.x()) / 16;
    y = static_cast<int>(pos.y()) / 16;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > maxX) x = maxX;
    if (y > 1) y = 1;
    return QPoint(x, y);
}
