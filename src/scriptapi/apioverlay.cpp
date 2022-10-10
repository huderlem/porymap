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

void MapView::addText(QString text, int x, int y, QString color, int fontSize, int layer) {
    this->getOverlay(layer)->addText(text, x, y, color, fontSize);
    this->scene()->update();
}

void MapView::addRect(int x, int y, int width, int height, QString color, int layer) {
    this->getOverlay(layer)->addRect(x, y, width, height, color, false);
    this->scene()->update();
}

void MapView::addFilledRect(int x, int y, int width, int height, QString color, int layer) {
    this->getOverlay(layer)->addRect(x, y, width, height, color, true);
    this->scene()->update();
}

void MapView::addImage(int x, int y, QString filepath, int layer, bool useCache) {
    if (this->getOverlay(layer)->addImage(x, y, filepath, useCache))
        this->scene()->update();
}

void MapView::createImage(int x, int y, QString filepath, int width, int height, int xOffset, int yOffset, qreal hScale, qreal vScale, int paletteId, bool setTransparency, int layer, bool useCache) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QList<QRgb> palette;
    if (paletteId != -1)
        palette = Tileset::getPalette(paletteId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->getOverlay(layer)->addImage(x, y, filepath, useCache, width, height, xOffset, yOffset, hScale, vScale, palette, setTransparency))
        this->scene()->update();
}

void MapView::addTileImage(int x, int y, int tileId, bool xflip, bool yflip, int paletteId, bool setTransparency, int layer) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QImage image = getPalettedTileImage(tileId,
                                        this->editor->map->layout->tileset_primary,
                                        this->editor->map->layout->tileset_secondary,
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
    if (!this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QImage image = getMetatileImage(static_cast<uint16_t>(metatileId),
                                    this->editor->map->layout->tileset_primary,
                                    this->editor->map->layout->tileset_secondary,
                                    this->editor->map->metatileLayerOrder,
                                    this->editor->map->metatileLayerOpacity);
    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));
    if (this->getOverlay(layer)->addImage(x, y, image))
        this->scene()->update();
}
