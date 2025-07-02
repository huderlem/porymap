#include "imageproviders.h"
#include "metatileselector.h"
#include "project.h"
#include <QPainter>

QPoint MetatileSelector::getSelectionDimensions() {
    if (this->prefabSelection || this->externalSelection)
        return selection.dimensions;
    return SelectablePixmapItem::getSelectionDimensions();
}

int MetatileSelector::numPrimaryMetatilesRounded() const {
    // We round up the number of primary metatiles to keep the tilesets on separate rows.
    return ceil((double)this->primaryTileset()->numMetatiles() / this->numMetatilesWide) * this->numMetatilesWide;
}

void MetatileSelector::updateBasePixmap() {
    this->basePixmap = QPixmap::fromImage(getMetatileSheetImage(this->layout, this->numMetatilesWide));
}

void MetatileSelector::draw() {
    if (this->basePixmap.isNull())
        updateBasePixmap();
    setPixmap(this->basePixmap);
    drawSelection();
}

void MetatileSelector::drawSelection() {
    if (!this->prefabSelection && (!this->externalSelection || (this->externalSelectionWidth == 1 && this->externalSelectionHeight == 1))) {
        SelectablePixmapItem::drawSelection();
    }
}

bool MetatileSelector::select(uint16_t metatileId) {
    if (!this->layout->metatileIsValid(metatileId)) return false;
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

void MetatileSelector::selectFromMap(uint16_t metatileId, uint16_t collision, uint16_t elevation) {
    QPair<uint16_t, uint16_t> movePermissions(collision, elevation);
    this->setExternalSelection(1, 1, {metatileId}, {movePermissions});
}

void MetatileSelector::setLayout(Layout *layout) {
    this->layout = layout;
    if (this->externalSelection)
        this->updateExternalSelectedMetatiles();
    else
        this->updateSelectedMetatiles();

    updateBasePixmap();
    draw();
}

void MetatileSelector::refresh() {
    setLayout(this->layout);
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
        if (!this->layout->metatileIsValid(metatileId))
            metatileId = 0;
        this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
    }
    if (this->selection.metatileItems.length() == 1) {
        QPoint coords = this->getMetatileIdCoords(this->selection.metatileItems.first().metatileId);
        SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
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
    return this->layout->metatileIsValid(getMetatileId(pos.x(), pos.y()));
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
    if (this->cellPos == pos)
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
            if (!this->layout->metatileIsValid(metatileId))
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
        if (!this->layout->metatileIsValid(metatileId))
            metatileId = 0;
        this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
    }
    emit selectedMetatilesChanged();
}

uint16_t MetatileSelector::getMetatileId(int x, int y) const {
    int index = y * this->numMetatilesWide + x;
    int numPrimary = this->numPrimaryMetatilesRounded();
    if (index < numPrimary) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - numPrimary);
    }
}

QPoint MetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    if (!this->layout->metatileIsValid(metatileId)) {
        return QPoint(0, 0);
    }

    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->numPrimaryMetatilesRounded();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}

QPoint MetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) {
    QPoint pos = getMetatileIdCoords(metatileId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}
