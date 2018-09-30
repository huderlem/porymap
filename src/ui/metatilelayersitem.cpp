#include "metatilelayersitem.h"
#include "imageproviders.h"
#include <QPainter>
#include <QDebug>

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
        QImage tileImage = getColoredTileImage(tile.tile, this->primaryTileset, this->secondaryTileset, tile.palette)
                .mirrored(tile.xflip, tile.yflip)
                .scaled(16, 16);
        painter.drawImage(tileCoords.at(i), tileImage);
    }

    this->setPixmap(pixmap);
}

void MetatileLayersItem::setMetatile(Metatile *metatile) {
    this->metatile = metatile;
}

void MetatileLayersItem::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

void MetatileLayersItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    int tileIndex = (x / 2 * 4) + (y * 2) + (x % 2);
    qDebug() << tileIndex;
    emit this->tileChanged(tileIndex);
}
