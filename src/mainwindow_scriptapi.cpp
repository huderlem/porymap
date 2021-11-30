#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "editcommands.h"
#include "config.h"

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
// full read of the map and its border/connections.
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
    // INT_MAX is used as an indicator value to refer to all overlays
    if (layer == INT_MAX)
        this->ui->graphicsView_Map->clearOverlays();
    else
        this->ui->graphicsView_Map->getOverlay(layer)->clearItems();
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addText(QString text, int x, int y, QString color, int fontSize, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || layer == INT_MAX)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addText(text, x, y, color, fontSize);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addRect(int x, int y, int width, int height, QString color, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || layer == INT_MAX)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addRect(x, y, width, height, color, false);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addFilledRect(int x, int y, int width, int height, QString color, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || layer == INT_MAX)
        return;
    this->ui->graphicsView_Map->getOverlay(layer)->addRect(x, y, width, height, color, true);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addImage(int x, int y, QString filepath, int width, int height, unsigned offset, bool hflip, bool vflip, bool setTransparency, int layer) {
    if (!this->ui || !this->ui->graphicsView_Map || layer == INT_MAX)
        return;
    if (this->ui->graphicsView_Map->getOverlay(layer)->addImage(x, y, filepath, width, height, offset, hflip, vflip, setTransparency))
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

QList<int> MainWindow::getMetatileLayerOrder() {
    if (!this->editor || !this->editor->map)
        return QList<int>();
    return this->editor->map->metatileLayerOrder;
}

void MainWindow::setMetatileLayerOrder(QList<int> order) {
    if (!this->editor || !this->editor->map)
        return;
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
    Tileset * tileset = Tileset::getBlockTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project)
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
    Tileset * tileset = Tileset::getBlockTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project)
        this->editor->project->saveTilesetMetatileAttributes(tileset);

    // If the Tileset Editor is currently displaying the updated metatile, refresh it
    if (this->tilesetEditor && this->tilesetEditor->getSelectedMetatile() == metatileId)
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

// TODO: Validate label
void MainWindow::setMetatileLabel(int metatileId, QString label) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;

    if (this->tilesetEditor && this->tilesetEditor->getSelectedMetatile() == metatileId) {
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
    int maxNumTiles = projectConfig.getTripleLayerMetatilesEnabled() ? 12 : 8;
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
    for (; i < numTileObjs; i++)
        metatile->tiles[i] = Scripting::toTile(tilesObj.property(i));

    // Fill remainder of specified length with empty Tiles
    for (; i < numTiles; i++)
        metatile->tiles[i] = Tile();

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
    for (int i = 0; i < numTiles; i++)
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
