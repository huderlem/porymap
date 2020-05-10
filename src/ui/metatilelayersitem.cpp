#include "metatilelayersitem.h"
#include "imageproviders.h"
#include <QPainter>

void MetatileLayersItem::draw() {
    const QList<QPoint> tileCoords = QList<QPoint>{
        QPoint(0, 0),
        QPoint(16, 0),
        QPoint(0, 16),
        QPoint(16, 16),
        QPoint(32, 0),
        QPoint(48, 0),
        QPoint(32, 16),
        QPoint(48, 16),
    };

    QPixmap pixmap(64, 32);
    QPainter painter(&pixmap);
    for (int i = 0; i < 8; i++) {
        Tile tile = this->metatile->tiles->at(i);
        QImage tileImage = getPalettedTileImage(tile.tile, this->primaryTileset, this->secondaryTileset, tile.palette, true)
                .mirrored(tile.xflip, tile.yflip)
                .scaled(16, 16);
        painter.drawImage(tileCoords.at(i), tileImage);
    }

    this->setPixmap(pixmap);
}

void MetatileLayersItem::setMetatile(Metatile *metatile) {
    this->metatile = metatile;
    this->clearLastModifiedCoords();
}

void MetatileLayersItem::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
    this->clearLastModifiedCoords();
}

void MetatileLayersItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        SelectablePixmapItem::mousePressEvent(event);
        QPoint selectionOrigin = this->getSelectionStart();
        QPoint dimensions = this->getSelectionDimensions();
        emit this->selectedTilesChanged(selectionOrigin, dimensions.x(), dimensions.y());
        this->drawSelection();
    } else {
        int x, y;
        this->getBoundedCoords(event->pos(), &x, &y);
        if (prevChangedTile.x() != x || prevChangedTile.y() != y) {
            this->prevChangedTile.setX(x);
            this->prevChangedTile.setY(y);
            emit this->tileChanged(x, y);
        }
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
        int x, y;
        this->getBoundedCoords(event->pos(), &x, &y);
        if (prevChangedTile.x() != x || prevChangedTile.y() != y) {
            this->prevChangedTile.setX(x);
            this->prevChangedTile.setY(y);
            emit this->tileChanged(x, y);
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

void MetatileLayersItem::clearLastModifiedCoords() {
    this->prevChangedTile.setX(-1);
    this->prevChangedTile.setY(-1);
}

void MetatileLayersItem::getBoundedCoords(QPointF pos, int *x, int *y) {
    *x = static_cast<int>(pos.x()) / 16;
    *y = static_cast<int>(pos.y()) / 16;
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x > 3) *x = 3;
    if (*y > 1) *y = 1;
}
