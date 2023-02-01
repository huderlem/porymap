#include "mapview.h"
#include "scripting.h"
#include "imageproviders.h"

void MapView::clear(int layer) {
    this->getOverlay(layer)->clearItems();
    this->scene()->update();
}

// Overload. No layer provided, clear all layers
void MapView::clear() {
    this->clearOverlayMap();
    this->scene()->update();
}

void MapView::hide(int layer) {
    this->setVisibility(false, layer);
}

// Overload. No layer provided, hide all layers
void MapView::hide() {
    this->setVisibility(false);
}

void MapView::show(int layer) {
    this->setVisibility(true, layer);
}

// Overload. No layer provided, show all layers
void MapView::show() {
    this->setVisibility(true);
}

bool MapView::getVisibility(int layer) {
    return !(this->getOverlay(layer)->getHidden());
}

void MapView::setVisibility(bool visible, int layer) {
    this->getOverlay(layer)->setHidden(!visible);
    this->scene()->update();
}

// Overload. No layer provided, set visibility of all layers
void MapView::setVisibility(bool visible) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setHidden(!visible);
    this->scene()->update();
}

int MapView::getX(int layer) {
    return this->getOverlay(layer)->getX();
}

int MapView::getY(int layer) {
    return this->getOverlay(layer)->getY();
}

void MapView::setX(int x, int layer) {
    this->getOverlay(layer)->setX(x);
    this->scene()->update();
}

// Overload. No layer provided, set x of all layers
void MapView::setX(int x) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setX(x);
    this->scene()->update();
}

void MapView::setY(int y, int layer) {
    this->getOverlay(layer)->setY(y);
    this->scene()->update();
}

// Overload. No layer provided, set y of all layers
void MapView::setY(int y) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setY(y);
    this->scene()->update();
}

void MapView::setClippingRect(int x, int y, int width, int height, int layer) {
    this->getOverlay(layer)->setClippingRect(QRectF(x, y, width, height));
    this->scene()->update();
}

void MapView::setClippingRect(int x, int y, int width, int height) {
    QRectF rect = QRectF(x, y, width, height);
    foreach (Overlay * layer, this->overlayMap)
        layer->setClippingRect(rect);
    this->scene()->update();
}

void MapView::clearClippingRect(int layer) {
    this->getOverlay(layer)->clearClippingRect();
    this->scene()->update();
}

void MapView::clearClippingRect() {
    foreach (Overlay * layer, this->overlayMap)
        layer->clearClippingRect();
    this->scene()->update();
}

QJSValue MapView::getPosition(int layer) {
    Overlay * overlay = this->getOverlay(layer);
    return Scripting::position(overlay->getX(), overlay->getY());
}

void MapView::setPosition(int x, int y, int layer) {
    this->getOverlay(layer)->setPosition(x, y);
    this->scene()->update();
}

// Overload. No layer provided, set position of all layers
void MapView::setPosition(int x, int y) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setPosition(x, y);
    this->scene()->update();
}

void MapView::move(int deltaX, int deltaY, int layer) {
    this->getOverlay(layer)->move(deltaX, deltaY);
    this->scene()->update();
}

// Overload. No layer provided, move all layers
void MapView::move(int deltaX, int deltaY) {
    foreach (Overlay * layer, this->overlayMap)
        layer->move(deltaX, deltaY);
    this->scene()->update();
}

int MapView::getOpacity(int layer) {
    return this->getOverlay(layer)->getOpacity();
}

void MapView::setOpacity(int opacity, int layer) {
    this->getOverlay(layer)->setOpacity(opacity);
    this->scene()->update();
}

// Overload. No layer provided, set opacity of all layers
void MapView::setOpacity(int opacity) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setOpacity(opacity);
    this->scene()->update();
}

qreal MapView::getHorizontalScale(int layer) {
    return this->getOverlay(layer)->getHScale();
}

qreal MapView::getVerticalScale(int layer) {
    return this->getOverlay(layer)->getVScale();
}

void MapView::setHorizontalScale(qreal scale, int layer) {
    this->getOverlay(layer)->setHScale(scale);
    this->scene()->update();
}

// Overload. No layer provided, set horizontal scale of all layers
void MapView::setHorizontalScale(qreal scale) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setHScale(scale);
    this->scene()->update();
}

void MapView::setVerticalScale(qreal scale, int layer) {
    this->getOverlay(layer)->setVScale(scale);
    this->scene()->update();
}

// Overload. No layer provided, set vertical scale of all layers
void MapView::setVerticalScale(qreal scale) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setVScale(scale);
    this->scene()->update();
}

void MapView::setScale(qreal hScale, qreal vScale, int layer) {
    this->getOverlay(layer)->setScale(hScale, vScale);
    this->scene()->update();
}

// Overload. No layer provided, set scale of all layers
void MapView::setScale(qreal hScale, qreal vScale) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setScale(hScale, vScale);
    this->scene()->update();
}

int MapView::getRotation(int layer) {
    return this->getOverlay(layer)->getRotation();
}

void MapView::setRotation(int angle, int layer) {
    this->getOverlay(layer)->setRotation(angle);
    this->scene()->update();
}

// Overload. No layer provided, set rotation of all layers
void MapView::setRotation(int angle) {
    foreach (Overlay * layer, this->overlayMap)
        layer->setRotation(angle);
    this->scene()->update();
}

void MapView::rotate(int degrees, int layer) {
    this->getOverlay(layer)->rotate(degrees);
    this->scene()->update();
}

// Overload. No layer provided, rotate all layers
void MapView::rotate(int degrees) {
    foreach (Overlay * layer, this->overlayMap)
        layer->rotate(degrees);
    this->scene()->update();
}

void MapView::addText(QString text, int x, int y, QString color, int fontSize, int layer) {
    this->getOverlay(layer)->addText(text, x, y, color, fontSize);
    this->scene()->update();
}

void MapView::addRect(int x, int y, int width, int height, QString borderColor, QString fillColor, int rounding, int layer) {
    if (this->getOverlay(layer)->addRect(x, y, width, height, borderColor, fillColor, rounding))
        this->scene()->update();
}

void MapView::addPath(QList<int> xCoords, QList<int> yCoords, QString borderColor, QString fillColor, int layer) {
    if (this->getOverlay(layer)->addPath(xCoords, yCoords, borderColor, fillColor))
        this->scene()->update();
}

void MapView::addPath(QList<QList<int>> coords, QString borderColor, QString fillColor, int layer) {
    QList<int> xCoords;
    QList<int> yCoords;
    for (int i = 0; i < coords.length(); i++) {
        if (coords[i].length() < 2){
            logWarn(QString("Element %1 of overlay path does not have an x and y value.").arg(i));
            continue;
        }
        xCoords.append(coords[i][0]);
        yCoords.append(coords[i][1]);
    }
    this->addPath(xCoords, yCoords, borderColor, fillColor, layer);
}

void MapView::addImage(int x, int y, QString filepath, int layer, bool useCache) {
    if (this->getOverlay(layer)->addImage(x, y, filepath, useCache))
        this->scene()->update();
}

void MapView::createImage(int x, int y, QString filepath, int width, int height, int xOffset, int yOffset, qreal hScale, qreal vScale, int paletteId, bool setTransparency, int layer, bool useCache) {
    if (!this->editor || !this->editor->map || !this->editor->layout
     || !this->editor->layout->tileset_primary || !this->editor->layout->tileset_secondary)
        return;
    QList<QRgb> palette;
    if (paletteId != -1)
        palette = Tileset::getPalette(paletteId, this->editor->layout->tileset_primary, this->editor->layout->tileset_secondary);
    if (this->getOverlay(layer)->addImage(x, y, filepath, useCache, width, height, xOffset, yOffset, hScale, vScale, palette, setTransparency))
        this->scene()->update();
}

void MapView::addTileImage(int x, int y, int tileId, bool xflip, bool yflip, int paletteId, bool setTransparency, int layer) {
    if (!this->editor || !this->editor->map || !this->editor->layout
     || !this->editor->layout->tileset_primary || !this->editor->layout->tileset_secondary)
        return;
    QImage image = getPalettedTileImage(tileId,
                                        this->editor->layout->tileset_primary,
                                        this->editor->layout->tileset_secondary,
                                        paletteId)
                                        .mirrored(xflip, yflip);
    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));
    if (this->getOverlay(layer)->addImage(x, y, image))
        this->scene()->update();
}

void MapView::addTileImage(int x, int y, QJSValue tileObj, bool setTransparency, int layer) {
    Tile tile = Scripting::toTile(tileObj);
    this->addTileImage(x, y, tile.tileId, tile.xflip, tile.yflip, tile.palette, setTransparency, layer);
}

void MapView::addMetatileImage(int x, int y, int metatileId, bool setTransparency, int layer) {
    if (!this->editor || !this->editor->map || !this->editor->layout
     || !this->editor->layout->tileset_primary || !this->editor->layout->tileset_secondary)
        return;
    QImage image = getMetatileImage(static_cast<uint16_t>(metatileId),
                                    this->editor->layout->tileset_primary,
                                    this->editor->layout->tileset_secondary,
                                    this->editor->layout->metatileLayerOrder,
                                    this->editor->layout->metatileLayerOpacity);
    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));
    if (this->getOverlay(layer)->addImage(x, y, image))
        this->scene()->update();
}
