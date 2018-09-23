#include "metatileselector.h"
#include "project.h"
#include <QPainter>

QPoint MetatileSelector::getSelectionDimensions() {
    if (this->externalSelection) {
        return QPoint(this->externalSelectionWidth, this->externalSelectionHeight);
    } else {
        return SelectablePixmapItem::getSelectionDimensions();
    }
}

void MetatileSelector::draw() {
    if (!this->primaryTileset || !this->primaryTileset->metatiles
     || !this->secondaryTileset || !this->secondaryTileset->metatiles) {
        this->setPixmap(QPixmap());
    }

    int primaryLength = this->primaryTileset->metatiles->length();
    int length_ = primaryLength + this->secondaryTileset->metatiles->length();
    int height_ = length_ / this->numMetatilesWide;
    QImage image(this->numMetatilesWide * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        QImage metatile_image = Tileset::getMetatileImage(tile, this->primaryTileset, this->secondaryTileset);
        int map_y = i / this->numMetatilesWide;
        int map_x = i % this->numMetatilesWide;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));

    if (!this->externalSelection) {
        this->drawSelection();
    }
}

void MetatileSelector::select(uint16_t metatileId) {
    this->externalSelection = false;
    QPoint coords = this->getMetatileIdCoords(metatileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->updateSelectedMetatiles();
    emit selectedMetatilesChanged();
}

void MetatileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->draw();
}

QList<uint16_t>* MetatileSelector::getSelectedMetatiles() {
    if (this->externalSelection) {
        return this->externalSelectedMetatiles;
    } else {
        return this->selectedMetatiles;
    }
}

void MetatileSelector::setExternalSelection(int width, int height, QList<uint16_t> *metatiles) {
    this->externalSelection = true;
    this->externalSelectionWidth = width;
    this->externalSelectionHeight = height;
    this->externalSelectedMetatiles->clear();
    for (int i = 0; i < metatiles->length(); i++) {
        this->externalSelectedMetatiles->append(metatiles->at(i));
    }

    this->draw();
    emit selectedMetatilesChanged();
}

void MetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedMetatiles();
    emit selectedMetatilesChanged();
}

void MetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedMetatiles();

    QPoint pos = this->getCellPos(event->pos());
    uint16_t metatileId = this->getMetatileId(pos.x(), pos.y());
    emit this->hoveredMetatileSelectionChanged(metatileId);
    emit selectedMetatilesChanged();
}

void MetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedMetatiles();
    emit selectedMetatilesChanged();
}

void MetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t metatileId = this->getMetatileId(pos.x(), pos.y());
    emit this->hoveredMetatileSelectionChanged(metatileId);
}

void MetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileSelectionCleared();
}

void MetatileSelector::updateSelectedMetatiles() {
    this->externalSelection = false;
    this->selectedMetatiles->clear();
    QPoint origin = this->getSelectionStart();
    QPoint dimensions = this->getSelectionDimensions();
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            uint16_t metatileId = this->getMetatileId(origin.x() + i, origin.y() + j);
            this->selectedMetatiles->append(metatileId);
        }
    }
}

uint16_t MetatileSelector::getMetatileId(int x, int y) {
    int index = y * this->numMetatilesWide + x;
    if (index < this->primaryTileset->metatiles->length()) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - this->primaryTileset->metatiles->length());
    }
}

QPoint MetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    if (metatileId >= Project::getNumMetatilesTotal()
       || (metatileId < Project::getNumMetatilesPrimary() && metatileId >= this->primaryTileset->metatiles->length())
       || (metatileId < Project::getNumMetatilesTotal() && metatileId >= Project::getNumMetatilesPrimary() + this->secondaryTileset->metatiles->length()))
    {
        // Invalid metatile id.
        return QPoint(0, 0);
    }

    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->primaryTileset->metatiles->length();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}
