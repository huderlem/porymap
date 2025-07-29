#include "tileseteditormetatileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>

// TODO: This class has a decent bit of overlap with the MetatileSelector class.
//       They should be refactored to inherit from a single parent class.

TilesetEditorMetatileSelector::TilesetEditorMetatileSelector(int numMetatilesWide, Tileset *primaryTileset, Tileset *secondaryTileset, Layout *layout)
  : SelectablePixmapItem(32, 32, 1, 1),
    numMetatilesWide(qMax(numMetatilesWide, 1)) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->layout = layout;
    setAcceptHoverEvents(true);
    this->usedMetatiles.resize(Project::getNumMetatilesTotal());
}

int TilesetEditorMetatileSelector::numRows(int numMetatiles) const {
    int numMetatilesHigh = numMetatiles / this->numMetatilesWide;
    if (numMetatiles % this->numMetatilesWide != 0) {
        // Round up height for incomplete last row
        numMetatilesHigh++;
    }
    return numMetatilesHigh;
}

int TilesetEditorMetatileSelector::numRows() const {
    return this->numRows(this->numPrimaryMetatilesRounded() + this->secondaryTileset->numMetatiles());
}

int TilesetEditorMetatileSelector::numPrimaryMetatilesRounded() const {
    if (!this->primaryTileset)
        return 0;
    return Util::roundUpToMultiple(this->primaryTileset->numMetatiles(), this->numMetatilesWide);
}

void TilesetEditorMetatileSelector::drawMetatile(uint16_t metatileId) {
    bool ok;
    QPoint pos = metatileIdToPos(metatileId, &ok);
    if (!ok)
        return;

    QPainter painter(&this->baseImage);
    QImage metatile_image = getMetatileImage(
                metatileId,
                this->primaryTileset,
                this->secondaryTileset,
                this->layout->metatileLayerOrder(),
                this->layout->metatileLayerOpacity(),
                true)
            .scaled(this->cellWidth, this->cellHeight);
    painter.drawImage(QPoint(pos.x() * this->cellWidth, pos.y() * this->cellHeight), metatile_image);
    painter.end();

    this->basePixmap = QPixmap::fromImage(this->baseImage);
    draw();
}

void TilesetEditorMetatileSelector::drawSelectedMetatile() {
    drawMetatile(this->selectedMetatileId);
}

void TilesetEditorMetatileSelector::updateBasePixmap() {
    this->baseImage = getMetatileSheetImage(this->primaryTileset,
                                            this->secondaryTileset,
                                            this->numMetatilesWide,
                                            this->layout->metatileLayerOrder(),
                                            this->layout->metatileLayerOpacity(),
                                            QSize(this->cellWidth, this->cellHeight),
                                            true);
    this->basePixmap = QPixmap::fromImage(this->baseImage);
}

void TilesetEditorMetatileSelector::draw() {
    if (this->basePixmap.isNull())
        updateBasePixmap();
    setPixmap(this->basePixmap);

    drawGrid();
    drawDivider();
    drawFilters();

    if (this->inSwapMode) {
        for (const auto &metatileId : this->swapMetatileIds) {
            bool ok;
            QPoint pos = metatileIdToPos(metatileId, &ok);
            if (ok) drawSelectionRect(pos, QSize(1,1), Qt::DashLine);
        }
    } else if (isValidMetatileId(this->selectedMetatileId)) {
        drawSelection();
    }
}

bool TilesetEditorMetatileSelector::select(uint16_t metatileId) {
    bool ok;
    QPoint pos = metatileIdToPos(metatileId, &ok);
    if (!ok)
        return false;
    SelectablePixmapItem::select(pos);
    this->selectedMetatileId = metatileId;
    emit selectedMetatileChanged(metatileId);
    return true;
}

void TilesetEditorMetatileSelector::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;

    updateBasePixmap();
    draw();
}

void TilesetEditorMetatileSelector::addToSwapSelection(uint16_t metatileId) {
    if (this->swapMetatileIds.contains(metatileId)) {
        return;
    }
    if (this->swapMetatileIds.length() >= 2) {
        this->swapMetatileIds.clear();
    }

    this->swapMetatileIds.append(metatileId);
    draw();

    if (this->swapMetatileIds.length() == 2) {
        emit swapRequested(this->swapMetatileIds.at(0), this->swapMetatileIds.at(1));
    }
}

void TilesetEditorMetatileSelector::removeFromSwapSelection(uint16_t metatileId) {
    if (this->swapMetatileIds.removeOne(metatileId)) {
        draw();
    }
}

void TilesetEditorMetatileSelector::clearSwapSelection() {
    if (this->swapMetatileIds.isEmpty())
        return;
    this->swapMetatileIds.clear();
    draw();
}

void TilesetEditorMetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    bool ok;
    uint16_t metatileId = posToMetatileId(getCellPos(event->pos()), &ok);
    if (!ok) return;

    if (this->inSwapMode) {
        if (this->swapMetatileIds.contains(metatileId)) {
            this->removeFromSwapSelection(metatileId);
        } else {
            this->addToSwapSelection(metatileId);
        }
    }

    SelectablePixmapItem::mousePressEvent(event);
    this->selectedMetatileId = metatileId;
    emit selectedMetatileChanged(this->selectedMetatileId);
}

void TilesetEditorMetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (this->inSwapMode) return;

    bool ok;
    uint16_t metatileId = posToMetatileId(getCellPos(event->pos()), &ok);
    if (!ok) return;

    SelectablePixmapItem::mouseMoveEvent(event);
    this->selectedMetatileId = metatileId;
    emit selectedMetatileChanged(this->selectedMetatileId);
    emit hoveredMetatileChanged(this->selectedMetatileId);
}

void TilesetEditorMetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (this->inSwapMode) return;

    bool ok;
    uint16_t metatileId = posToMetatileId(getCellPos(event->pos()), &ok);
    if (!ok) return;

    SelectablePixmapItem::mouseReleaseEvent(event);
    this->selectedMetatileId = metatileId;
    emit selectedMetatileChanged(this->selectedMetatileId);
}

void TilesetEditorMetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    bool ok;
    uint16_t metatileId = posToMetatileId(getCellPos(event->pos()), &ok);
    if (ok) {
        emit this->hoveredMetatileChanged(metatileId);
    } else {
        emit this->hoveredMetatileCleared();
    }
}

void TilesetEditorMetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileCleared();
}

uint16_t TilesetEditorMetatileSelector::posToMetatileId(const QPoint &pos, bool *ok) const {
    return posToMetatileId(pos.x(), pos.y(), ok);
}

uint16_t TilesetEditorMetatileSelector::posToMetatileId(int x, int y, bool *ok) const {
    if (ok) *ok = true;
    int index = y * this->numMetatilesWide + x;
    uint16_t metatileId = static_cast<uint16_t>(index);
    if (this->primaryTileset && this->primaryTileset->contains(metatileId)) {
        return metatileId;
    }

    // There's some extra handling here because we round the tilesets to keep them on separate rows.
    // This means if the maximum number of primary metatiles is not divisible by the metatile width
    // then the metatiles we used to round the primary tileset would have the index of valid secondary metatiles.
    // These need to be ignored, or they'll appear to be duplicates of the subseqeunt secondary metatiles.
    int numPrimaryRounded = numPrimaryMetatilesRounded();
    int firstSecondaryRow = numPrimaryRounded / this->numMetatilesWide;
    metatileId = static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - numPrimaryRounded);
    if (this->secondaryTileset && this->secondaryTileset->contains(metatileId) && y >= firstSecondaryRow) {
        return metatileId;
    }

    if (ok) *ok = false;
    return 0;
}

QPoint TilesetEditorMetatileSelector::metatileIdToPos(uint16_t metatileId, bool *ok) const {
    if (this->primaryTileset && this->primaryTileset->contains(metatileId)) {
        if (ok) *ok = true;
        int index = metatileId;
        return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
    }
    if (this->secondaryTileset && this->secondaryTileset->contains(metatileId)) {
        if (ok) *ok = true;
        int index = metatileId - Project::getNumMetatilesPrimary() + numPrimaryMetatilesRounded();
        return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
    }

    if (ok) *ok = false;
    return QPoint(0,0);
}

bool TilesetEditorMetatileSelector::isValidMetatileId(uint16_t metatileId) const {
    bool ok;
    metatileIdToPos(metatileId, &ok);
    return ok;
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) const {
    QPoint pos = metatileIdToPos(metatileId);
    pos.rx() = (pos.x() * this->cellWidth) + (this->cellWidth / 2);
    pos.ry() = (pos.y() * this->cellHeight) + (this->cellHeight / 2);
    return pos;
}

void TilesetEditorMetatileSelector::drawGrid() {
    if (!this->showGrid)
        return;

    QPixmap pixmap = this->pixmap();
    QPainter painter(&pixmap);
    const int numColumns = this->numMetatilesWide;
    const int numRows = this->numRows();
    for (int column = 1; column < numColumns; column++) {
        int x = column * this->cellWidth;
        painter.drawLine(x, 0, x, numRows * this->cellHeight);
    }
    for (int row = 1; row < numRows; row++) {
        int y = row * this->cellHeight;
        painter.drawLine(0, y, numColumns * this->cellWidth, y);
    }
    painter.end();
    this->setPixmap(pixmap);
}

void TilesetEditorMetatileSelector::drawDivider() {
    if (!this->showDivider)
        return;

    const int y = this->numRows(this->numPrimaryMetatilesRounded()) * this->cellHeight;

    QPixmap pixmap = this->pixmap();
    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    painter.drawLine(0, y, this->numMetatilesWide * this->cellWidth, y);
    painter.end();
    this->setPixmap(pixmap);
}

void TilesetEditorMetatileSelector::drawFilters() {
    if (selectorShowUnused) {
        drawUnused();
    }
    if (selectorShowCounts) {
        drawCounts();
    }
}

void TilesetEditorMetatileSelector::drawUnused() {
    // setup the circle with a line through it image to layer above unused metatiles
    QPixmap redX(this->cellWidth, this->cellHeight);
    redX.fill(Qt::transparent);

    QPen whitePen(Qt::white);
    whitePen.setWidth(1);
    QPen pinkPen(Qt::magenta);
    pinkPen.setWidth(1);

    QPainter oPainter(&redX);

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(1, 1, this->cellWidth - 2, this->cellHeight - 2));
    oPainter.setPen(pinkPen);
    oPainter.drawEllipse(QRect(2, 2, this->cellWidth - 4, this->cellHeight - 4));
    oPainter.drawEllipse(QRect(3, 3, this->cellWidth - 6, this->cellHeight - 6));

    oPainter.setPen(whitePen);
    oPainter.drawEllipse(QRect(4, 4, this->cellHeight - 8, this->cellHeight - 8));

    whitePen.setWidth(5);
    oPainter.setPen(whitePen);
    oPainter.drawLine(0, 0, this->cellWidth - 1, this->cellHeight - 1);

    pinkPen.setWidth(3);
    oPainter.setPen(pinkPen);
    oPainter.drawLine(2, 2, this->cellWidth - 3, this->cellHeight - 3);

    oPainter.end();

    // draw symbol on unused metatiles
    QPixmap metatilesPixmap = this->pixmap();

    QPainter unusedPainter(&metatilesPixmap);
    unusedPainter.setOpacity(0.5);

    for (int metatileId = 0; metatileId < this->usedMetatiles.size(); metatileId++) {
        if (this->usedMetatiles.at(metatileId) || !Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
            continue;
        // Adjust position from center to top-left corner
        QPoint pos = getMetatileIdCoordsOnWidget(metatileId) - QPoint(this->cellWidth / 2, this->cellHeight / 2);
        unusedPainter.drawPixmap(pos.x(), pos.y(), redX);
    }
    unusedPainter.end();

    this->setPixmap(metatilesPixmap);
}

void TilesetEditorMetatileSelector::drawCounts() {
    QPen blackPen(Qt::black);
    blackPen.setWidth(1);
    QPen whitePen(Qt::white);
    whitePen.setWidth(1);

    QPixmap metatilesPixmap = this->pixmap();
    QPainter countPainter(&metatilesPixmap);

    for (int metatileId = 0; metatileId < this->usedMetatiles.size(); metatileId++) {
        if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
            continue;

        int count = this->usedMetatiles.at(metatileId);
        QString countText = (count > 1000) ? QStringLiteral(">1k") : QString::number(count);

        // Adjust position from center to bottom-left corner
        QPoint pos = getMetatileIdCoordsOnWidget(metatileId) + QPoint(-(this->cellWidth / 2), this->cellHeight / 2);

        // write in black and white for contrast
        countPainter.setPen(blackPen);
        countPainter.drawText(pos.x(), pos.y(), countText);
        countPainter.setPen(whitePen);
        countPainter.drawText(pos.x() + 1, pos.y() - 1, countText);
    }
    countPainter.end();

    this->setPixmap(metatilesPixmap);
}

void TilesetEditorMetatileSelector::setSwapMode(bool enabled) {
    if (enabled == this->inSwapMode)
        return;
    this->inSwapMode = enabled;
    this->swapMetatileIds.clear();
    if (porymapConfig.prettyCursors) {
        if (enabled) {
            static const QCursor cursor = QCursor(QPixmap(":/icons/swap_cursor.ico"), 10, 10);
            setCursor(cursor);
        } else {
            unsetCursor();
        }
    }
    draw();
}
