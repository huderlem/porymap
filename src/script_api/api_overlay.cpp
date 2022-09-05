#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "config.h"
#include "imageproviders.h"

void MainWindow::clearOverlay(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->clearItems();
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::clearOverlays() {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->clearOverlays();
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::hideOverlay(int layer) {
    this->setOverlayVisibility(false, layer);
}

void MainWindow::hideOverlays() {
    this->setOverlaysVisibility(false);
}

void MainWindow::showOverlay(int layer) {
    this->setOverlayVisibility(true, layer);
}

void MainWindow::showOverlays() {
    this->setOverlaysVisibility(true);
}

bool MainWindow::getOverlayVisibility(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return false;
    return !(this->ui->graphicsView_Map->getOverlay(layer)->getHidden());
}

void MainWindow::setOverlayVisibility(bool visible, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->setHidden(!visible);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlaysVisibility(bool visible) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->setOverlaysHidden(!visible);
    this->ui->graphicsView_Map->scene()->update();
}

int MainWindow::getOverlayX(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return 0;
    return this->ui->graphicsView_Map->getOverlay(layer)->getX();
}

int MainWindow::getOverlayY(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return 0;
    return this->ui->graphicsView_Map->getOverlay(layer)->getY();
}

void MainWindow::setOverlayX(int x, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->setX(x);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlayY(int y, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->setY(y);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlaysX(int x) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->setOverlaysX(x);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlaysY(int y) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->setOverlaysY(y);
    this->ui->graphicsView_Map->scene()->update();
}

QJSValue MainWindow::getOverlayPosition(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return QJSValue();
    Overlay * overlay = this->ui->graphicsView_Map->getOverlay(layer);
    return Scripting::position(overlay->getX(), overlay->getY());
}

void MainWindow::setOverlayPosition(int x, int y, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->setPosition(x, y);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlaysPosition(int x, int y) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->setOverlaysPosition(x, y);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::moveOverlay(int deltaX, int deltaY, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->move(deltaX, deltaY);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::moveOverlays(int deltaX, int deltaY) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->moveOverlays(deltaX, deltaY);
    this->ui->graphicsView_Map->scene()->update();
}

int MainWindow::getOverlayOpacity(int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return 0;
    return this->ui->graphicsView_Map->getOverlay(layer)->getOpacity();
}

void MainWindow::setOverlayOpacity(int opacity, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->setOpacity(opacity);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::setOverlaysOpacity(int opacity) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->setOverlaysOpacity(opacity);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addText(QString text, int x, int y, QString color, int fontSize, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addText(text, x, y, color, fontSize);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addRect(int x, int y, int width, int height, QString color, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addRect(x, y, width, height, color, false);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addFilledRect(int x, int y, int width, int height, QString color, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addRect(x, y, width, height, color, true);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addImage(int x, int y, QString filepath, int layer, bool useCache) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, filepath, useCache))
        this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::createImage(int x, int y, QString filepath, int width, int height, unsigned offset, qreal hScale, qreal vScale, int paletteId, bool setTransparency, int layer, bool useCache) {
    if (!this->ui || !this->ui->graphicsView_Map || !this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QList<QRgb> palette;
    if (paletteId != -1)
        palette = Tileset::getPalette(paletteId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, filepath, useCache, width, height, offset, hScale, vScale, palette, setTransparency))
        this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addTileImage(int x, int y, int tileId, bool xflip, bool yflip, int paletteId, bool setTransparency, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || !this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QImage image = getPalettedTileImage(tileId,
                                        this->editor->map->layout->tileset_primary,
                                        this->editor->map->layout->tileset_secondary,
                                        paletteId)
                                        .mirrored(xflip, yflip);
    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, image))
        this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addTileImage(int x, int y, QJSValue tileObj, bool setTransparency, int layer) {
    Tile tile = Scripting::toTile(tileObj);
    this->addTileImage(x, y, tile.tileId, tile.xflip, tile.yflip, tile.palette, setTransparency, layer);
}

void MainWindow::addMetatileImage(int x, int y, int metatileId, bool setTransparency, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || !this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QImage image = getMetatileImage(static_cast<uint16_t>(metatileId),
                                    this->editor->map->layout->tileset_primary,
                                    this->editor->map->layout->tileset_secondary,
                                    this->editor->map->metatileLayerOrder,
                                    this->editor->map->metatileLayerOpacity);
    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, image))
        this->ui->graphicsView_Map->scene()->update();
}
