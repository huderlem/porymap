#include "tileseteditormetatileselector.h"
#include "imageproviders.h"
#include "project.h"
#include <QPainter>

TilesetEditorMetatileSelector::TilesetEditorMetatileSelector(Tileset *primaryTileset, Tileset *secondaryTileset, Layout *layout)
  : SelectablePixmapItem(32, 32, 1, 1) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->numMetatilesWide = 8;
    this->layout = layout;
    setAcceptHoverEvents(true);
    this->usedMetatiles.resize(Project::getNumMetatilesTotal());
}

int TilesetEditorMetatileSelector::numRows(int numMetatiles) {
    int numMetatilesHigh = numMetatiles / this->numMetatilesWide;
    if (numMetatiles % this->numMetatilesWide != 0) {
        // Round up height for incomplete last row
        numMetatilesHigh++;
    }
    return numMetatilesHigh;
}

int TilesetEditorMetatileSelector::numRows() {
    return this->numRows(this->numPrimaryMetatilesRounded() + this->secondaryTileset->numMetatiles());
}

int TilesetEditorMetatileSelector::numPrimaryMetatilesRounded() const {
    // We round up the number of primary metatiles to keep the tilesets on separate rows.
    return ceil((double)this->primaryTileset->numMetatiles() / this->numMetatilesWide) * this->numMetatilesWide;
}

void TilesetEditorMetatileSelector::drawMetatile(uint16_t metatileId) {
    QPoint pos = getMetatileIdCoords(metatileId);

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

    drawSelection();
}

bool TilesetEditorMetatileSelector::select(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset)) return false;
    QPoint coords = this->getMetatileIdCoords(metatileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
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

void TilesetEditorMetatileSelector::updateSelectedMetatile() {
    QPoint origin = this->getSelectionStart();
    uint16_t metatileId = this->getMetatileId(origin.x(), origin.y());
    if (Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
        this->selectedMetatileId = metatileId;
    else
        this->selectedMetatileId = Project::getNumMetatilesPrimary() + this->secondaryTileset->numMetatiles() - 1;
    emit selectedMetatileChanged(this->selectedMetatileId);
}

uint16_t TilesetEditorMetatileSelector::getSelectedMetatileId() {
    return this->selectedMetatileId;
}

uint16_t TilesetEditorMetatileSelector::getMetatileId(int x, int y) {
    int index = y * this->numMetatilesWide + x;
    int numPrimary = numPrimaryMetatilesRounded();
    if (index < numPrimary) {
        return static_cast<uint16_t>(index);
    } else {
        return static_cast<uint16_t>(Project::getNumMetatilesPrimary() + index - numPrimary);
    }
}

bool TilesetEditorMetatileSelector::shouldAcceptEvent(QGraphicsSceneMouseEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    return Tileset::metatileIsValid(getMetatileId(pos.x(), pos.y()), this->primaryTileset, this->secondaryTileset);
}

void TilesetEditorMetatileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedMetatile();
}

void TilesetEditorMetatileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedMetatile();
    emit hoveredMetatileChanged(this->selectedMetatileId);
}

void TilesetEditorMetatileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (!shouldAcceptEvent(event)) return;
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedMetatile();
}

void TilesetEditorMetatileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    uint16_t metatileId = this->getMetatileId(pos.x(), pos.y());
    emit this->hoveredMetatileChanged(metatileId);
}

void TilesetEditorMetatileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
    emit this->hoveredMetatileCleared();
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoords(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset))
    {
        // Invalid metatile id.
        return QPoint(0, 0);
    }
    int index = metatileId < Project::getNumMetatilesPrimary()
                ? metatileId
                : metatileId - Project::getNumMetatilesPrimary() + this->numPrimaryMetatilesRounded();
    return QPoint(index % this->numMetatilesWide, index / this->numMetatilesWide);
}

QPoint TilesetEditorMetatileSelector::getMetatileIdCoordsOnWidget(uint16_t metatileId) {
    QPoint pos = getMetatileIdCoords(metatileId);
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
