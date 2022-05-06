#include "tilemaptileselector.h"

#include <QDebug>

void TilemapTileSelector::draw() {
    size_t width_  = this->tileset.width();
    this->pixelWidth = width_;
    size_t height_ = this->tileset.height();
    this->pixelHeight = height_;
    size_t ntiles_ = (width_/8) * (height_/8);

    this->numTilesWide = width_ / 8;
    this->numTiles     = ntiles_;

    this->setPixmap(QPixmap::fromImage(this->setPalette(this->tile_palette)));
    this->drawSelection();
}

void TilemapTileSelector::select(unsigned tileId) {
    QPoint coords = this->getTileIdCoords(tileId);
    SelectablePixmapItem::select(coords.x(), coords.y(), 0, 0);
    this->selectedTile = tileId;
    this->drawSelection();
    emit selectedTileChanged(tileId);
}

void TilemapTileSelector::updateSelectedTile() {
    QPoint origin = this->getSelectionStart();
    this->selectedTile = this->getTileId(origin.x(), origin.y());
}

unsigned TilemapTileSelector::getTileId(int x, int y) {
    unsigned index = y * this->numTilesWide + x;
    return index < this->numTiles ? index : this->numTiles % index;
}

QPoint TilemapTileSelector::getTileIdCoords(unsigned tileId) {
    int index = tileId < this->numTiles ? tileId : this->numTiles % tileId;
    return QPoint(index % this->numTilesWide, index / this->numTilesWide);
}

QImage TilemapTileSelector::setPalette(int paletteIndex) {
    QImage tilesetImage = this->tileset;
    tilesetImage.convertTo(QImage::Format::Format_Indexed8);

    // TODO: bounds check on the palette copying
    switch(this->format) {
        case TilemapFormat::Plain:
            break;
        case TilemapFormat::BPP_4:
        {
            QVector<QRgb> newColorTable;
            int palMinLength = paletteIndex * 16 + 16;
            if ((this->palette.count() < palMinLength) || (tilesetImage.colorTable().count() != 16)) {
                // either a) the palette has less than 256 colors, or b) the image is improperly indexed
                for (QRgb color : tilesetImage.colorTable()) {
                    int gray = qGray(color);
                    newColorTable.append(qRgb(gray, gray, gray));
                }
            } else {
                // use actual pal
// before Qt 6, the color table is a QVector which is deprecated now, and this method does not exits
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                newColorTable = this->palette.toVector().mid(paletteIndex * 16, 16);
#else
                newColorTable = this->palette.mid(paletteIndex * 16, 16);
#endif
            }
            tilesetImage.setColorTable(newColorTable);
            break;
        }
        case TilemapFormat::BPP_8:
        {
            if (tilesetImage.colorTable().count() == this->palette.count()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                tilesetImage.setColorTable(this->palette.toVector());
#else
                tilesetImage.setColorTable(this->palette);
#endif
            }
            break;
        }
        default: break;
    }

    return tilesetImage;
}

QImage TilemapTileSelector::tileImg(shared_ptr<TilemapTile> tile) {
    unsigned tileId = tile->id();
    QPoint pos = getTileIdCoords(tileId);

    QImage tilesetImage = setPalette(tile->palette());

    // take a tile from the tileset
    QImage img = tilesetImage.copy(pos.x() * 8, pos.y() * 8, 8, 8);
    img = img.mirrored(tile->hFlip(), tile->vFlip());
    return img;
}

void TilemapTileSelector::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mousePressEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}

void TilemapTileSelector::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseMoveEvent(event);
    this->updateSelectedTile();
    emit hoveredTileChanged(this->selectedTile);
    emit selectedTileChanged(this->selectedTile);
}

void TilemapTileSelector::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SelectablePixmapItem::mouseReleaseEvent(event);
    this->updateSelectedTile();
    emit selectedTileChanged(this->selectedTile);
}

void TilemapTileSelector::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QPoint pos = this->getCellPos(event->pos());
    unsigned tileId = this->getTileId(pos.x(), pos.y());
    emit this->hoveredTileChanged(tileId);
}

void TilemapTileSelector::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    emit this->hoveredTileCleared();
}
