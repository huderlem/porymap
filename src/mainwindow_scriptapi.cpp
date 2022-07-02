#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "editcommands.h"
#include "config.h"
#include "imageproviders.h"

QJSValue MainWindow::getBlock(int x, int y) {
    if (!this->editor || !this->editor->map)
        return QJSValue();
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return Scripting::fromBlock(Block());
    }
    return Scripting::fromBlock(block);
}

// TODO: "needsFullRedraw" is used when redrawing the map after
// changing a metatile's tiles via script. It is unnecessarily
// resource intensive. The map metatiles that need to be updated are
// not marked as changed, so they will not be redrawn if the cache
// isn't ignored. Ideally the setMetatileTiles functions would properly
// set each of the map spaces that use the modified metatile so that
// the cache could be used, though this would lkely still require a
// full read of the map.
void MainWindow::tryRedrawMapArea(bool forceRedraw) {
    if (!forceRedraw) return;

    if (this->needsFullRedraw) {
        this->editor->map_item->draw(true);
        this->editor->collision_item->draw(true);
        this->editor->updateMapBorder();
        this->editor->updateMapConnections();
        this->needsFullRedraw = false;
    } else {
        this->editor->map_item->draw();
        this->editor->collision_item->draw();
    }
}

void MainWindow::tryCommitMapChanges(bool commitChanges) {
    if (commitChanges) {
        Map *map = this->editor->map;
        if (map) {
            map->editHistory.push(new ScriptEditMap(map,
                map->layout->lastCommitMapBlocks.dimensions, QSize(map->getWidth(), map->getHeight()),
                map->layout->lastCommitMapBlocks.blocks, map->layout->blockdata
            ));
        }
    }
}

void MainWindow::setBlock(int x, int y, int tile, int collision, int elevation, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map->setBlock(x, y, Block(tile, collision, elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::setBlocksFromSelection(int x, int y, bool forceRedraw, bool commitChanges) {
    if (this->editor && this->editor->map_item) {
        this->editor->map_item->paintNormal(x, y, true);
        this->tryCommitMapChanges(commitChanges);
        this->tryRedrawMapArea(forceRedraw);
    }
}

int MainWindow::getMetatileId(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return 0;
    }
    return block.metatileId;
}

void MainWindow::setMetatileId(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(metatileId, block.collision, block.elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

int MainWindow::getCollision(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return 0;
    }
    return block.collision;
}

void MainWindow::setCollision(int x, int y, int collision, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block.metatileId, collision, block.elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

int MainWindow::getElevation(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return 0;
    }
    return block.elevation;
}

void MainWindow::setElevation(int x, int y, int elevation, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block.metatileId, block.collision, elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::bucketFill(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map_item->floodFill(x, y, metatileId, true);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::bucketFillFromSelection(int x, int y, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map_item->floodFill(x, y, true);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::magicFill(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map_item->magicFill(x, y, metatileId, true);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::magicFillFromSelection(int x, int y, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map_item->magicFill(x, y, true);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::shift(int xDelta, int yDelta, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map_item->shift(xDelta, yDelta, true);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::redraw() {
    this->tryRedrawMapArea(true);
}

void MainWindow::commit() {
    this->tryCommitMapChanges(true);
}

QJSValue MainWindow::getDimensions() {
    if (!this->editor || !this->editor->map)
        return QJSValue();
    return Scripting::dimensions(this->editor->map->getWidth(), this->editor->map->getHeight());
}

int MainWindow::getWidth() {
    if (!this->editor || !this->editor->map)
        return 0;
    return this->editor->map->getWidth();
}

int MainWindow::getHeight() {
    if (!this->editor || !this->editor->map)
        return 0;
    return this->editor->map->getHeight();
}

void MainWindow::setDimensions(int width, int height) {
    if (!this->editor || !this->editor->map)
        return;
    if (!Project::mapDimensionsValid(width, height))
        return;
    this->editor->map->setDimensions(width, height);
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

void MainWindow::setWidth(int width) {
    if (!this->editor || !this->editor->map)
        return;
    if (!Project::mapDimensionsValid(width, this->editor->map->getHeight()))
        return;
    this->editor->map->setDimensions(width, this->editor->map->getHeight());
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

void MainWindow::setHeight(int height) {
    if (!this->editor || !this->editor->map)
        return;
    if (!Project::mapDimensionsValid(this->editor->map->getWidth(), height))
        return;
    this->editor->map->setDimensions(this->editor->map->getWidth(), height);
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

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

void MainWindow::createImage(int x, int y, QString filepath, int width, int height, unsigned offset, bool xflip, bool yflip, int paletteId, bool setTransparency, int layer, bool useCache) {
    if (!this->ui || !this->ui->graphicsView_Map || !this->editor || !this->editor->map || !this->editor->map->layout
     || !this->editor->map->layout->tileset_primary || !this->editor->map->layout->tileset_secondary)
        return;
    QList<QRgb> palette;
    if (paletteId != -1)
        palette = Tileset::getPalette(paletteId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, filepath, useCache, width, height, offset, xflip, yflip, palette, setTransparency))
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

void MainWindow::refreshAfterPaletteChange(Tileset *tileset) {
    if (this->tilesetEditor) {
        this->tilesetEditor->updateTilesets(this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label);
    }
    this->editor->metatile_selector_item->draw();
    this->editor->selected_border_metatiles_item->draw();
    this->editor->map_item->draw(true);
    this->editor->updateMapBorder();
    this->editor->updateMapConnections();
    this->editor->project->saveTilesetPalettes(tileset);
}

void MainWindow::setTilesetPalette(Tileset *tileset, int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return;
    if (paletteIndex >= tileset->palettes.size())
        return;
    if (colors.size() != 16)
        return;

    for (int i = 0; i < 16; i++) {
        if (colors[i].size() != 3)
            continue;
        tileset->palettes[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
        tileset->palettePreviews[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
    }
}

void MainWindow::setPrimaryTilesetPalette(int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    this->setTilesetPalette(this->editor->map->layout->tileset_primary, paletteIndex, colors);
    this->refreshAfterPaletteChange(this->editor->map->layout->tileset_primary);
}

void MainWindow::setPrimaryTilesetPalettes(QList<QList<QList<int>>> palettes) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalette(this->editor->map->layout->tileset_primary, i, palettes[i]);
    }
    this->refreshAfterPaletteChange(this->editor->map->layout->tileset_primary);
}

void MainWindow::setSecondaryTilesetPalette(int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    this->setTilesetPalette(this->editor->map->layout->tileset_secondary, paletteIndex, colors);
    this->refreshAfterPaletteChange(this->editor->map->layout->tileset_secondary);
}

void MainWindow::setSecondaryTilesetPalettes(QList<QList<QList<int>>> palettes) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalette(this->editor->map->layout->tileset_secondary, i, palettes[i]);
    }
    this->refreshAfterPaletteChange(this->editor->map->layout->tileset_secondary);
}

QJSValue MainWindow::getTilesetPalette(const QList<QList<QRgb>> &palettes, int paletteIndex) {
    if (paletteIndex >= palettes.size())
        return QJSValue();

    QList<QList<int>> palette;
    for (auto color : palettes.value(paletteIndex)) {
        palette.append(QList<int>({qRed(color), qGreen(color), qBlue(color)}));
    }
    return Scripting::getEngine()->toScriptValue(palette);
}

QJSValue MainWindow::getTilesetPalettes(const QList<QList<QRgb>> &palettes) {
    QList<QList<QList<int>>> outPalettes;
    for (int i = 0; i < palettes.size(); i++) {
        QList<QList<int>> colors;
        for (auto color : palettes.value(i)) {
            colors.append(QList<int>({qRed(color), qGreen(color), qBlue(color)}));
        }
        outPalettes.append(colors);
    }
    return Scripting::getEngine()->toScriptValue(outPalettes);
}

QJSValue MainWindow::getPrimaryTilesetPalette(int paletteIndex) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return QJSValue();
    return this->getTilesetPalette(this->editor->map->layout->tileset_primary->palettes, paletteIndex);
}

QJSValue MainWindow::getPrimaryTilesetPalettes() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return QJSValue();
    return this->getTilesetPalettes(this->editor->map->layout->tileset_primary->palettes);
}

QJSValue MainWindow::getSecondaryTilesetPalette(int paletteIndex) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return QJSValue();
    return this->getTilesetPalette(this->editor->map->layout->tileset_secondary->palettes, paletteIndex);
}

QJSValue MainWindow::getSecondaryTilesetPalettes() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return QJSValue();
    return this->getTilesetPalettes(this->editor->map->layout->tileset_secondary->palettes);
}

void MainWindow::refreshAfterPalettePreviewChange() {
    this->editor->metatile_selector_item->draw();
    this->editor->selected_border_metatiles_item->draw();
    this->editor->map_item->draw(true);
    this->editor->updateMapBorder();
    this->editor->updateMapConnections();
}

void MainWindow::setTilesetPalettePreview(Tileset *tileset, int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return;
    if (paletteIndex >= tileset->palettePreviews.size())
        return;
    if (colors.size() != 16)
        return;

    for (int i = 0; i < 16; i++) {
        if (colors[i].size() != 3)
            continue;
        tileset->palettePreviews[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
    }
}

void MainWindow::setPrimaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    this->setTilesetPalettePreview(this->editor->map->layout->tileset_primary, paletteIndex, colors);
    this->refreshAfterPalettePreviewChange();
}

void MainWindow::setPrimaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalettePreview(this->editor->map->layout->tileset_primary, i, palettes[i]);
    }
    this->refreshAfterPalettePreviewChange();
}

void MainWindow::setSecondaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    this->setTilesetPalettePreview(this->editor->map->layout->tileset_secondary, paletteIndex, colors);
    this->refreshAfterPalettePreviewChange();
}

void MainWindow::setSecondaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalettePreview(this->editor->map->layout->tileset_secondary, i, palettes[i]);
    }
    this->refreshAfterPalettePreviewChange();
}

QJSValue MainWindow::getPrimaryTilesetPalettePreview(int paletteIndex) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return QJSValue();
    return this->getTilesetPalette(this->editor->map->layout->tileset_primary->palettePreviews, paletteIndex);
}

QJSValue MainWindow::getPrimaryTilesetPalettesPreview() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return QJSValue();
    return this->getTilesetPalettes(this->editor->map->layout->tileset_primary->palettePreviews);
}

QJSValue MainWindow::getSecondaryTilesetPalettePreview(int paletteIndex) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return QJSValue();
    return this->getTilesetPalette(this->editor->map->layout->tileset_secondary->palettePreviews, paletteIndex);
}

QJSValue MainWindow::getSecondaryTilesetPalettesPreview() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return QJSValue();
    return this->getTilesetPalettes(this->editor->map->layout->tileset_secondary->palettePreviews);
}

int MainWindow::getNumPrimaryTilesetMetatiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return 0;
    return this->editor->map->layout->tileset_primary->metatiles.length();
}

int MainWindow::getMaxPrimaryTilesetMetatiles() {
    if (!this->editor || !this->editor->project)
        return 0;
    return this->editor->project->getNumMetatilesPrimary();
}

int MainWindow::getNumSecondaryTilesetMetatiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return 0;
    return this->editor->map->layout->tileset_secondary->metatiles.length();
}

int MainWindow::getMaxSecondaryTilesetMetatiles() {
    if (!this->editor || !this->editor->project)
        return 0;
    return this->editor->project->getNumMetatilesTotal() - this->editor->project->getNumMetatilesPrimary();
}


int MainWindow::getNumPrimaryTilesetTiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return 0;
    return this->editor->map->layout->tileset_primary->tiles.length();
}

int MainWindow::getMaxPrimaryTilesetTiles() {
    if (!this->editor || !this->editor->project)
        return 0;
    return this->editor->project->getNumTilesPrimary();
}

int MainWindow::getNumSecondaryTilesetTiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return 0;
    return this->editor->map->layout->tileset_secondary->tiles.length();
}

int MainWindow::getMaxSecondaryTilesetTiles() {
    if (!this->editor || !this->editor->project)
        return 0;
    return this->editor->project->getNumTilesTotal() - this->editor->project->getNumTilesPrimary();
}

bool MainWindow::isPrimaryTileset(QString tilesetName) {
    if (!this->editor || !this->editor->project)
        return false;
    return this->editor->project->tilesetLabels["primary"].contains(tilesetName);
}

bool MainWindow::isSecondaryTileset(QString tilesetName) {
    if (!this->editor || !this->editor->project)
        return false;
    return this->editor->project->tilesetLabels["secondary"].contains(tilesetName);
}

QString MainWindow::getPrimaryTileset() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return QString();
    return this->editor->map->layout->tileset_primary->name;
}

QString MainWindow::getSecondaryTileset() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return QString();
    return this->editor->map->layout->tileset_secondary->name;
}

void MainWindow::setPrimaryTileset(QString tileset) {
    this->on_comboBox_PrimaryTileset_currentTextChanged(tileset);
}

void MainWindow::setSecondaryTileset(QString tileset) {
    this->on_comboBox_SecondaryTileset_currentTextChanged(tileset);
}

void MainWindow::setGridVisibility(bool visible) {
    this->ui->checkBox_ToggleGrid->setChecked(visible);
}

bool MainWindow::getGridVisibility() {
    return this->ui->checkBox_ToggleGrid->isChecked();
}

void MainWindow::setBorderVisibility(bool visible) {
    this->editor->toggleBorderVisibility(visible);
}

bool MainWindow::getBorderVisibility() {
    return this->ui->checkBox_ToggleBorder->isChecked();
}

void MainWindow::setSmartPathsEnabled(bool visible) {
    this->ui->checkBox_smartPaths->setChecked(visible);
}

bool MainWindow::getSmartPathsEnabled() {
    return this->ui->checkBox_smartPaths->isChecked();
}

void MainWindow::registerAction(QString functionName, QString actionName, QString shortcut) {
    if (!this->ui || !this->ui->menuTools)
        return;

    Scripting::registerAction(functionName, actionName);
    if (Scripting::numRegisteredActions() == 1) {
        QAction *section = this->ui->menuTools->addSection("Custom Actions");
        this->registeredActions.append(section);
    }
    QAction *action = this->ui->menuTools->addAction(actionName, [actionName](){
       Scripting::invokeAction(actionName);
    });
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    this->registeredActions.append(action);
}

void MainWindow::setTimeout(QJSValue callback, int milliseconds) {
  if (!callback.isCallable() || milliseconds < 0)
      return;

    QTimer *timer = new QTimer(0);
    connect(timer, &QTimer::timeout, [=](){
        this->invokeCallback(callback);
    });
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->setSingleShot(true);
    timer->start(milliseconds);
}

void MainWindow::invokeCallback(QJSValue callback) {
    Scripting::tryErrorJS(callback.call());
}

void MainWindow::log(QString message) {
    logInfo(message);
}

void MainWindow::warn(QString message) {
    logWarn(message);
}

void MainWindow::error(QString message) {
    logError(message);
}

QList<int> MainWindow::getMetatileLayerOrder() {
    if (!this->editor || !this->editor->map)
        return QList<int>();
    return this->editor->map->metatileLayerOrder;
}

void MainWindow::setMetatileLayerOrder(QList<int> order) {
    if (!this->editor || !this->editor->map)
        return;

    const int numLayers = 3;
    int size = order.size();
    if (size < numLayers) {
        logError(QString("Metatile layer order has insufficient elements (%1), needs at least %2.").arg(size).arg(numLayers));
        return;
    }
    bool invalid = false;
    for (int i = 0; i < numLayers; i++) {
        int layer = order.at(i);
        if (layer < 0 || layer >= numLayers) {
            logError(QString("'%1' is not a valid metatile layer order value, must be in range 0-%2.").arg(layer).arg(numLayers - 1));
            invalid = true;
        }
    }
    if (invalid) return;

    this->editor->map->metatileLayerOrder = order;
    this->refreshAfterPalettePreviewChange();
}

QList<float> MainWindow::getMetatileLayerOpacity() {
    if (!this->editor || !this->editor->map)
        return QList<float>();
    return this->editor->map->metatileLayerOpacity;
}

void MainWindow::setMetatileLayerOpacity(QList<float> order) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map->metatileLayerOpacity = order;
    this->refreshAfterPalettePreviewChange();
}

void MainWindow::saveMetatilesByMetatileId(int metatileId) {
    Tileset * tileset = Tileset::getMetatileTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project && tileset)
        this->editor->project->saveTilesetMetatiles(tileset);

    // Refresh anything that can display metatiles (except the actual map view)
    if (this->tilesetEditor)
        this->tilesetEditor->updateTilesets(this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label);
    if (this->editor->metatile_selector_item)
        this->editor->metatile_selector_item->draw();
    if (this->editor->selected_border_metatiles_item)
        this->editor->selected_border_metatiles_item->draw();
    if (this->editor->current_metatile_selection_item)
        this->editor->current_metatile_selection_item->draw();
}

void MainWindow::saveMetatileAttributesByMetatileId(int metatileId) {
    Tileset * tileset = Tileset::getMetatileTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project && tileset)
        this->editor->project->saveTilesetMetatileAttributes(tileset);

    // If the Tileset Editor is currently displaying the updated metatile, refresh it
    if (this->tilesetEditor && this->tilesetEditor->getSelectedMetatileId() == metatileId)
        this->tilesetEditor->updateTilesets(this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label);
}

Metatile * MainWindow::getMetatile(int metatileId) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return nullptr;
    return Tileset::getMetatile(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
}

QString MainWindow::getMetatileLabel(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile || metatile->label.size() == 0)
        return QString();
    return metatile->label;
}

void MainWindow::setMetatileLabel(int metatileId, QString label) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;

    QRegularExpression expression("[_A-Za-z0-9]*$");
    QRegularExpressionValidator validator(expression);
    int pos = 0;
    if (validator.validate(label, pos) != QValidator::Acceptable) {
        logError(QString("Invalid metatile label %1").arg(label));
        return;
    }

    if (this->tilesetEditor && this->tilesetEditor->getSelectedMetatileId() == metatileId) {
        this->tilesetEditor->setMetatileLabel(label);
    } else if (metatile->label != label) {
        metatile->label = label;
        if (this->editor->project)
            this->editor->project->saveTilesetMetatileLabels(this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    }
}

int MainWindow::getMetatileLayerType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->layerType;
}

void MainWindow::setMetatileLayerType(int metatileId, int layerType) {
    Metatile * metatile = this->getMetatile(metatileId);
    uint8_t u_layerType = static_cast<uint8_t>(layerType);
    if (!metatile || metatile->layerType == u_layerType || u_layerType >= NUM_METATILE_LAYER_TYPES)
        return;
    metatile->layerType = u_layerType;
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileEncounterType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->encounterType;
}

void MainWindow::setMetatileEncounterType(int metatileId, int encounterType) {
    Metatile * metatile = this->getMetatile(metatileId);
    uint8_t u_encounterType = static_cast<uint8_t>(encounterType);
    if (!metatile || metatile->encounterType == u_encounterType || u_encounterType >= NUM_METATILE_ENCOUNTER_TYPES)
        return;
    metatile->encounterType = u_encounterType;
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileTerrainType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->terrainType;
}

void MainWindow::setMetatileTerrainType(int metatileId, int terrainType) {
    Metatile * metatile = this->getMetatile(metatileId);
    uint8_t u_terrainType = static_cast<uint8_t>(terrainType);
    if (!metatile || metatile->terrainType == u_terrainType ||  u_terrainType >= NUM_METATILE_TERRAIN_TYPES)
        return;
    metatile->terrainType = u_terrainType;
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileBehavior(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->behavior;
}

void MainWindow::setMetatileBehavior(int metatileId, int behavior) {
    Metatile * metatile = this->getMetatile(metatileId);
    uint16_t u_behavior = static_cast<uint16_t>(behavior);
    if (!metatile || metatile->behavior == u_behavior)
        return;
    metatile->behavior = u_behavior;
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::calculateTileBounds(int * tileStart, int * tileEnd) {
    int maxNumTiles = this->getNumTilesInMetatile();
    if (*tileEnd >= maxNumTiles || *tileEnd < 0)
        *tileEnd = maxNumTiles - 1;
    if (*tileStart >= maxNumTiles || *tileStart < 0)
        *tileStart = 0;
    return 1 + (*tileEnd - *tileStart);
}

QJSValue MainWindow::getMetatileTiles(int metatileId, int tileStart, int tileEnd) {
    Metatile * metatile = this->getMetatile(metatileId);
    int numTiles = calculateTileBounds(&tileStart, &tileEnd);
    if (!metatile || numTiles <= 0)
        return QJSValue();

    QJSValue tiles = Scripting::getEngine()->newArray(numTiles);
    for (int i = 0; i < numTiles; i++, tileStart++)
        tiles.setProperty(i, Scripting::fromTile(metatile->tiles[tileStart]));
    return tiles;
}

void MainWindow::setMetatileTiles(int metatileId, QJSValue tilesObj, int tileStart, int tileEnd, bool forceRedraw) {
    Metatile * metatile = this->getMetatile(metatileId);
    int numTiles = calculateTileBounds(&tileStart, &tileEnd);
    if (!metatile || numTiles <= 0)
        return;

    // Write to metatile using as many of the given Tiles as possible
    int numTileObjs = qMin(tilesObj.property("length").toInt(), numTiles);
    int i = 0;
    for (; i < numTileObjs; i++, tileStart++)
        metatile->tiles[tileStart] = Scripting::toTile(tilesObj.property(i));

    // Fill remainder of specified length with empty Tiles
    for (; i < numTiles; i++, tileStart++)
        metatile->tiles[tileStart] = Tile();

    this->saveMetatilesByMetatileId(metatileId);
    this->needsFullRedraw = true;
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::setMetatileTiles(int metatileId, int tileId, bool xflip, bool yflip, int palette, int tileStart, int tileEnd, bool forceRedraw) {
    Metatile * metatile = this->getMetatile(metatileId);
    int numTiles = calculateTileBounds(&tileStart, &tileEnd);
    if (!metatile || numTiles <= 0)
        return;

    // Write to metatile using Tiles of the specified value
    Tile tile = Tile(tileId, xflip, yflip, palette);
    for (int i = tileStart; i <= tileEnd; i++)
        metatile->tiles[i] = tile;

    this->saveMetatilesByMetatileId(metatileId);
    this->needsFullRedraw = true;
    this->tryRedrawMapArea(forceRedraw);
}

QJSValue MainWindow::getMetatileTile(int metatileId, int tileIndex) {
    QJSValue tilesObj = this->getMetatileTiles(metatileId, tileIndex, tileIndex);
    return tilesObj.property(0);
}

void MainWindow::setMetatileTile(int metatileId, int tileIndex, int tileId, bool xflip, bool yflip, int palette, bool forceRedraw) {
    this->setMetatileTiles(metatileId, tileId, xflip, yflip, palette, tileIndex, tileIndex, forceRedraw);
}

void MainWindow::setMetatileTile(int metatileId, int tileIndex, QJSValue tileObj, bool forceRedraw) {
    Tile tile = Scripting::toTile(tileObj);
    this->setMetatileTiles(metatileId, tile.tileId, tile.xflip, tile.yflip, tile.palette, tileIndex, tileIndex, forceRedraw);
}

QJSValue MainWindow::getTilePixels(int tileId) {
    if (tileId < 0 || !this->editor || !this->editor->project || !this->editor->map || !this->editor->map->layout)
        return QJSValue();
    QImage tileImage = getTileImage(tileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (tileImage.isNull() || tileImage.sizeInBytes() < 64)
        return QJSValue();
    const uchar * pixels = tileImage.constBits();
    QJSValue pixelArray = Scripting::getEngine()->newArray(64);
    for (int i = 0; i < 64; i++) {
        pixelArray.setProperty(i, pixels[i]);
    }
    return pixelArray;
}

int MainWindow::getNumTilesInMetatile() {
    return projectConfig.getTripleLayerMetatilesEnabled() ? 12 : 8;
}

int MainWindow::getNumMetatileLayers() {
    return projectConfig.getTripleLayerMetatilesEnabled() ? 3 : 2;
}

QString MainWindow::getBaseGameVersion() {
    return projectConfig.getBaseGameVersionString();
}

QList<QString> MainWindow::getCustomScripts() {
    return projectConfig.getCustomScripts();
}

int MainWindow::getMainTab() {
    if (!this->ui || !this->ui->mainTabBar)
        return -1;
    return this->ui->mainTabBar->currentIndex();
}

void MainWindow::setMainTab(int index) {
    if (!this->ui || !this->ui->mainTabBar || index < 0 || index >= this->ui->mainTabBar->count())
        return;
    // Can't select Wild Encounters tab if it's disabled
    if (index == 4 && !projectConfig.getEncounterJsonActive())
        return;
    this->on_mainTabBar_tabBarClicked(index);
}

int MainWindow::getMapViewTab() {
    if (!this->ui || !this->ui->mapViewTab)
        return -1;
    return this->ui->mapViewTab->currentIndex();
}

void MainWindow::setMapViewTab(int index) {
    if (this->getMainTab() != 0 || !this->ui->mapViewTab || index < 0 || index >= this->ui->mapViewTab->count())
        return;
    this->on_mapViewTab_tabBarClicked(index);
}

bool MainWindow::gameStringToBool(QString s) {
    return (s.toInt() > 0 || s == "TRUE");
}

QString MainWindow::getSong() {
    if (!this->editor || !this->editor->map)
        return QString();
    return this->editor->map->song;
}

void MainWindow::setSong(QString song) {
    if (!this->ui || !this->editor || !this->editor->project)
        return;
    if (!this->editor->project->songNames.contains(song)) {
        logError(QString("Unknown song '%1'").arg(song));
        return;
    }
    this->ui->comboBox_Song->setCurrentText(song);
}

QString MainWindow::getLocation() {
    if (!this->editor || !this->editor->map)
        return QString();
    return this->editor->map->location;
}

void MainWindow::setLocation(QString location) {
    if (!this->ui || !this->editor || !this->editor->project)
        return;
    if (!this->editor->project->mapSectionNameToValue.contains(location)) {
        logError(QString("Unknown location '%1'").arg(location));
        return;
    }
    this->ui->comboBox_Location->setCurrentText(location);
}

bool MainWindow::getRequiresFlash() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->gameStringToBool(this->editor->map->requiresFlash);
}

void MainWindow::setRequiresFlash(bool require) {
    if (!this->ui)
        return;
    this->ui->checkBox_Visibility->setChecked(require);
}

QString MainWindow::getWeather() {
    if (!this->editor || !this->editor->map)
        return QString();
    return this->editor->map->weather;
}

void MainWindow::setWeather(QString weather) {
    if (!this->ui || !this->editor || !this->editor->project)
        return;
    if (!this->editor->project->weatherNames.contains(weather)) {
        logError(QString("Unknown weather '%1'").arg(weather));
        return;
    }
    this->ui->comboBox_Weather->setCurrentText(weather);
}

QString MainWindow::getType() {
    if (!this->editor || !this->editor->map)
        return QString();
    return this->editor->map->type;
}

void MainWindow::setType(QString type) {
    if (!this->ui || !this->editor || !this->editor->project)
        return;
    if (!this->editor->project->mapTypes.contains(type)) {
        logError(QString("Unknown map type '%1'").arg(type));
        return;
    }
    this->ui->comboBox_Type->setCurrentText(type);
}

QString MainWindow::getBattleScene() {
    if (!this->editor || !this->editor->map)
        return QString();
    return this->editor->map->battle_scene;
}

void MainWindow::setBattleScene(QString battleScene) {
    if (!this->ui || !this->editor || !this->editor->project)
        return;
    if (!this->editor->project->mapBattleScenes.contains(battleScene)) {
        logError(QString("Unknown battle scene '%1'").arg(battleScene));
        return;
    }
    this->ui->comboBox_BattleScene->setCurrentText(battleScene);
}

bool MainWindow::getShowLocationName() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->gameStringToBool(this->editor->map->show_location);
}

void MainWindow::setShowLocationName(bool show) {
    if (!this->ui)
        return;
    this->ui->checkBox_ShowLocation->setChecked(show);
}

bool MainWindow::getAllowRunning() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->gameStringToBool(this->editor->map->allowRunning);
}

void MainWindow::setAllowRunning(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowRunning->setChecked(allow);
}

bool MainWindow::getAllowBiking() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->gameStringToBool(this->editor->map->allowBiking);
}

void MainWindow::setAllowBiking(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowBiking->setChecked(allow);
}

bool MainWindow::getAllowEscaping() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->gameStringToBool(this->editor->map->allowEscapeRope);
}

void MainWindow::setAllowEscaping(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowEscapeRope->setChecked(allow);
}

int MainWindow::getFloorNumber() {
    if (!this->editor || !this->editor->map)
        return 0;
    return this->editor->map->floorNumber;
}

void MainWindow::setFloorNumber(int floorNumber) {
    if (!this->ui)
        return;
    if (floorNumber < -128 || floorNumber > 127) {
        logError(QString("Invalid floor number '%1'").arg(floorNumber));
        return;
    }
    this->ui->spinBox_FloorNumber->setValue(floorNumber);
}

