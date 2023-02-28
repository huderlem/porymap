#include "stampselector.h"
#include "imageproviders.h"
#include "project.h"

#include <QPainter>

QPoint StampSelector::getSelectionDimensions() {
    return selection->dimensions;
}

void StampSelector::draw() {
    if (!this->primaryTileset || !this->secondaryTileset) {
        this->setPixmap(QPixmap());
    }

    int primaryLength = this->primaryTileset->metatiles.length();
    int length_ = primaryLength + this->secondaryTileset->metatiles.length();
    int height_ = length_ / this->numStampsWide;
    if (length_ % this->numStampsWide != 0) {
        height_++;
    }
    QImage image(this->numStampsWide * 16, height_ * 16, QImage::Format_RGBA8888);
    image.fill(Qt::magenta);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        QImage metatile_image = getMetatileImage(tile, this->primaryTileset, this->secondaryTileset, map->metatileLayerOrder, map->metatileLayerOpacity);
        int map_y = i / this->numStampsWide;
        int map_x = i % this->numStampsWide;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));
    this->drawSelection();
}

bool StampSelector::select(uint16_t stampId) {
    // TODO:
    if (stampId > 9999999999999) return false;

    this->selection->dimensions = QPoint(1, 1);
    this->selection->stampIds = QList<uint16_t>(stampId);

    QPoint coords = this->getStampIdCoords(stampId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->updateSelectedStamps();
    return true;
}

void StampSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->updateSelectedStamps();
    this->draw();
}

StampSelection* StampSelector::getStampSelection() {
    return selection;
}

bool StampSelector::shouldAcceptEvent(QGraphicsSceneMouseEvent *event) {
//    QPoint pos = this->getCellPos(event->pos());
//    return Tileset::metatileIsValid(getStampId(pos.x(), pos.y()), this->primaryTileset, this->secondaryTileset);
    // TODO:
    return true;
}

void StampSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedStamps();
}

void StampSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedStamps();
}

void StampSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedStamps();
}

void StampSelector::updateSelectedStamps() {
    this->selection->stampIds.clear();
    this->selection->dimensions = SelectablePixmapItem::getSelectionDimensions();
    QPoint origin = this->getSelectionStart();
    for (int j = 0; j < this->selection->dimensions.y(); j++) {
        for (int i = 0; i < this->selection->dimensions.x(); i++) {
            uint16_t stampId = this->getStampId(origin.x() + i, origin.y() + j);
            // TODO: check if stamp id is valid
            if (false)
                stampId = 0;
            this->selection->stampIds.append(stampId);
        }
    }
    emit selectedStampsChanged();
}

uint16_t StampSelector::getStampId(int x, int y) {
    return y * this->numStampsWide + x;
}

QPoint StampSelector::getStampIdCoords(uint16_t stampId) {
    // TODO: check if stamp id is valid
    if (false)
    {
        // Invalid stamp id.
        return QPoint(0, 0);
    }

    return QPoint(stampId % this->numStampsWide, stampId / this->numStampsWide);
}

QPoint StampSelector::getStampIdCoordsOnWidget(uint16_t stampId) {
    QPoint pos = getStampIdCoords(stampId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}

void StampSelector::setMap(Map *map) {
    this->map = map;
}
