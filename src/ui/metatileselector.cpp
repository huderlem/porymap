#include "imageproviders.h"
#include "metatileselector.h"
#include "project.h"
#include <QPainter>

QPoint MetatileSelector::getSelectionDimensions() {
    if (this->prefabSelection || this->externalSelection)
        return selection.dimensions;
    return SelectablePixmapItem::getSelectionDimensions();
}

void MetatileSelector::draw() {
    if (!this->primaryTileset || !this->secondaryTileset) {
        this->setPixmap(QPixmap());
    }

    int primaryLength = this->primaryTileset->metatiles.length();
    int length_ = primaryLength + this->secondaryTileset->metatiles.length();
    int height_ = length_ / this->numMetatilesWide;
    if (length_ % this->numMetatilesWide != 0) {
        height_++;
    }
    QImage image(this->numMetatilesWide * 16, height_ * 16, QImage::Format_RGBA8888);
    image.fill(Qt::magenta);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primaryLength) {
            tile += Project::getNumMetatilesPrimary() - primaryLength;
        }
        QImage metatile_image = getMetatileImage(tile, this->primaryTileset, this->secondaryTileset, map->metatileLayerOrder, map->metatileLayerOpacity);
        int map_y = i / this->numMetatilesWide;
        int map_x = i % this->numMetatilesWide;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    this->setPixmap(QPixmap::fromImage(image));

    if (!this->prefabSelection && (!this->externalSelection || (this->externalSelectionWidth == 1 && this->externalSelectionHeight == 1))) {
        this->drawSelection();
    }
}

bool MetatileSelector::select(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset)) return false;
    this->externalSelection = false;
    this->prefabSelection = false;
    this->selection = MetatileSelection{
            QPoint(1, 1),
            false,
            QList<MetatileSelectionItem>({MetatileSelectionItem{true, metatileId}}),
            QList<CollisionSelectionItem>(),
    };
    QPoint coords = this->getMetatileIdCoords(metatileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->updateSelectedMetatiles();
    return true;
}

bool MetatileSelector::selectFromMap(uint16_t metatileId, uint16_t collision, uint16_t elevation) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset)) return false;
    this->select(metatileId);
    this->selection.collisionItems.append(CollisionSelectionItem{true, collision, elevation});
    this->selection.hasCollision = true;
    return true;
}

void MetatileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    if (this->externalSelection)
        this->updateExternalSelectedMetatiles();
    else
        this->updateSelectedMetatiles();
    this->draw();
}

MetatileSelection MetatileSelector::getMetatileSelection() {
    return selection;
}

void MetatileSelector::setExternalSelection(int width, int height, QList<uint16_t> metatiles, QList<QPair<uint16_t, uint16_t>> collisions) {
    this->prefabSelection = false;
    this->externalSelection = true;
    this->externalSelectionWidth = width;
    this->externalSelectionHeight = height;
    this->externalSelectedMetatiles.clear();
    this->selection.metatileItems.clear();
    this->selection.collisionItems.clear();
    this->selection.hasCollision = true;
    this->selection.dimensions = QPoint(width, height);
    for (int i = 0; i < metatiles.length(); i++) {
        auto collision = collisions.at(i);
        this->selection.collisionItems.append(CollisionSelectionItem{true, collision.first, collision.second});
        uint16_t metatileId = metatiles.at(i);
        this->externalSelectedMetatiles.append(metatileId);
        if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
            metatileId = 0;
        this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
    }

    this->draw();
    emit selectedMetatilesChanged();
}

void MetatileSelector::setPrefabSelection(MetatileSelection selection) {
    this->externalSelection = false;
    this->prefabSelection = true;
    this->externalSelectedMetatiles.clear();
    this->selection = selection;
    this->draw();
    emit selectedMetatilesChanged();
}

bool MetatileSelector::positionIsValid(const QPoint &pos) const {
    return Tileset::metatileIsValid(getMetatileId(pos.x(), pos.y()), this->primaryTileset, this->secondaryTileset);
}

void MetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    if (!positionIsValid(pos))
        return;

    this->cellPos = pos;
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedMetatiles();
}

void MetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    if (!positionIsValid(pos) || this->cellPos == pos)
        return;

    this->cellPos = pos;
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedMetatiles();
    this->hoverChanged();
}

void MetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    if (!positionIsValid(pos))
        return;
    SelectablePixmapItem::mouseReleaseEvent(event);
}

void MetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    if (!positionIsValid(pos) || this->cellPos == pos)
        return;

    this->cellPos = pos;
    this->hoverChanged();
}

void MetatileSelector::hoverChanged() {
    uint16_t metatileId = this->getMetatileId(this->cellPos.x(), this->cellPos.y());
    emit this->hoveredMetatileSelectionChanged(metatileId);
}

void MetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileSelectionCleared();
    this->cellPos = QPoint(-1, -1);
}

void MetatileSelector::updateSelectedMetatiles() {
    this->externalSelection = false;
    this->prefabSelection = false;
    this->selection.metatileItems.clear();
    this->selection.collisionItems.clear();
    this->selection.hasCollision = false;
    this->selection.dimensions = this->getSelectionDimensions();
    QPoint origin = this->getSelectionStart();
    for (int j = 0; j < this->selection.dimensions.y(); j++) {
        for (int i = 0; i < this->selection.dimensions.x(); i++) {
            uint16_t metatileId = this->getMetatileId(origin.x() + i, origin.y() + j);
            if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
                metatileId = 0;
            this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
        }
    }
    emit selectedMetatilesChanged();
}

void MetatileSelector::updateExternalSelectedMetatiles() {
    this->selection.metatileItems.clear();
    this->selection.dimensions = QPoint(this->externalSelectionWidth, this->externalSelectionHeight);
    for (int i = 0; i < this->externalSelectedMetatiles.count(); ++i) {
        uint16_t metatileId = this->externalSelectedMetatiles.at(i);
        if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
            metatileId = 0;
        this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
    }
    emit selectedMetatilesChanged();
}

uint16_t MetatileSelector::getMetatileId(int x, int y) const {
    int index = y * this->numMetatilesWide + x;
    if (index < this->primaryTileset->metatiles.length()) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - this->primaryTileset->metatiles.length());
    }
}

QPoint MetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
    {
        // Invalid metatile id.
        return QPoint(0, 0);
    }

    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->primaryTileset->metatiles.length();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}

QPoint MetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) {
    QPoint pos = getMetatileIdCoords(metatileId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}

void MetatileSelector::setMap(Map *map) {
    this->map = map;
}
