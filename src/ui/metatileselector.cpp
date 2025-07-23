#include "imageproviders.h"
#include "metatileselector.h"
#include "project.h"
#include <QPainter>

QPoint MetatileSelector::getSelectionDimensions() const {
    if (this->prefabSelection || this->externalSelection)
        return selection.dimensions;
    return SelectablePixmapItem::getSelectionDimensions();
}

int MetatileSelector::numPrimaryMetatilesRounded() const {
    if (!primaryTileset())
        return 0;
    return Util::roundUpToMultiple(primaryTileset()->numMetatiles(), this->numMetatilesWide);
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
    bool ok;
    QPoint pos = metatileIdToPos(metatileId, &ok);
    if (!ok) {
        return false;
    }
    this->externalSelection = false;
    this->prefabSelection = false;
    this->selection = MetatileSelection{
            QPoint(1, 1),
            false,
            QList<MetatileSelectionItem>({MetatileSelectionItem{true, metatileId}}),
            QList<CollisionSelectionItem>(),
    };
    SelectablePixmapItem::select(pos);
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

void MetatileSelector::setExternalSelection(int width, int height, const QList<uint16_t> &metatiles, const QList<QPair<uint16_t, uint16_t>> &collisions) {
    this->prefabSelection = false;
    this->externalSelection = true;
    this->externalSelectionWidth = width;
    this->externalSelectionHeight = height;
    this->externalSelectedMetatiles.clear();
    this->selection.metatileItems.clear();
    this->selection.collisionItems.clear();
    this->selection.hasCollision = true;
    this->selection.dimensions = QPoint(width, height);
    for (int i = 0; i < qMin(metatiles.length(), collisions.length()); i++) {
        uint16_t metatileId = metatiles.at(i);
        uint16_t collision = collisions.at(i).first;
        uint16_t elevation = collisions.at(i).second;
        this->selection.collisionItems.append(CollisionSelectionItem{true, collision, elevation});
        this->externalSelectedMetatiles.append(metatileId);
        if (!this->layout->metatileIsValid(metatileId))
            metatileId = 0;
        this->selection.metatileItems.append(MetatileSelectionItem{true, metatileId});
    }
    if (this->selection.metatileItems.length() == 1) {
        SelectablePixmapItem::select(metatileIdToPos(this->selection.metatileItems.first().metatileId));
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
    bool ok;
    posToMetatileId(pos, &ok);
    return ok;
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
    bool ok;
    uint16_t metatileId = posToMetatileId(this->cellPos, &ok);
    if (ok) {
        emit this->hoveredMetatileSelectionChanged(metatileId);
    } else {
        emit this->hoveredMetatileSelectionCleared();
        this->cellPos = QPoint(-1, -1);
    }
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
            uint16_t metatileId = posToMetatileId(origin.x() + i, origin.y() + j);
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

uint16_t MetatileSelector::posToMetatileId(const QPoint &pos, bool *ok) const {
    return posToMetatileId(pos.x(), pos.y(), ok);
}

uint16_t MetatileSelector::posToMetatileId(int x, int y, bool *ok) const {
    if (ok) *ok = true;
    int index = y * this->numMetatilesWide + x;
    uint16_t metatileId = static_cast<uint16_t>(index);
    if (primaryTileset() && primaryTileset()->contains(metatileId)) {
        return metatileId;
    }

    // There's some extra handling here because we round the tilesets to keep them on separate rows.
    // This means if the maximum number of primary metatiles is not divisible by the metatile width
    // then the metatiles we used to round the primary tileset would have the index of valid secondary metatiles.
    // These need to be ignored, or they'll appear to be duplicates of the subseqeunt secondary metatiles.
    int numPrimaryRounded = numPrimaryMetatilesRounded();
    int firstSecondaryRow = numPrimaryRounded / this->numMetatilesWide;
    metatileId = static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - numPrimaryRounded);
    if (secondaryTileset() && secondaryTileset()->contains(metatileId) && y >= firstSecondaryRow) {
        return metatileId;
    }

    if (ok) *ok = false;
    return 0;
}

QPoint MetatileSelector::metatileIdToPos(uint16_t metatileId, bool *ok) const {
    if (primaryTileset() && primaryTileset()->contains(metatileId)) {
        if (ok) *ok = true;
        int index = metatileId;
        return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
    }
    if (secondaryTileset() && secondaryTileset()->contains(metatileId)) {
        if (ok) *ok = true;
        int index = metatileId - Project::getNumMetatilesPrimary() + numPrimaryMetatilesRounded();
        return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
    }

    if (ok) *ok = false;
    return QPoint(0,0);
}

QPoint MetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) const {
    QPoint pos = metatileIdToPos(metatileId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}
