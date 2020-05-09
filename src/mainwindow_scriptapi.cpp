#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"

QJSValue MainWindow::getBlock(int x, int y) {
    if (!this->editor || !this->editor->map)
        return QJSValue();
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return Scripting::fromBlock(Block());
    }
    return Scripting::fromBlock(*block);
}

void MainWindow::tryRedrawMapArea(bool forceRedraw) {
    if (forceRedraw) {
        this->editor->map_item->draw();
        this->editor->collision_item->draw();
    }
}

void MainWindow::tryCommitMapChanges(bool commitChanges) {
    if (commitChanges) {
        this->editor->map->commit();
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
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return 0;
    }
    return block->tile;
}

void MainWindow::setMetatileId(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(metatileId, block->collision, block->elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

int MainWindow::getCollision(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return 0;
    }
    return block->collision;
}

void MainWindow::setCollision(int x, int y, int collision, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block->tile, collision, block->elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

int MainWindow::getElevation(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return 0;
    }
    return block->elevation;
}

void MainWindow::setElevation(int x, int y, int elevation, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block *block = this->editor->map->getBlock(x, y);
    if (!block) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block->tile, block->collision, elevation));
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
    this->editor->map_item->shift(xDelta, yDelta);
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
    if (!MainWindow::mapDimensionsValid(width, height))
        return;
    this->editor->map->setDimensions(width, height);
    this->editor->map->commit();
    this->onMapNeedsRedrawing();
}

void MainWindow::setWidth(int width) {
    if (!this->editor || !this->editor->map)
        return;
    if (!MainWindow::mapDimensionsValid(width, this->editor->map->getHeight()))
        return;
    this->editor->map->setDimensions(width, this->editor->map->getHeight());
    this->editor->map->commit();
    this->onMapNeedsRedrawing();
}

void MainWindow::setHeight(int height) {
    if (!this->editor || !this->editor->map)
        return;
    if (!MainWindow::mapDimensionsValid(this->editor->map->getWidth(), height))
        return;
    this->editor->map->setDimensions(this->editor->map->getWidth(), height);
    this->editor->map->commit();
    this->onMapNeedsRedrawing();
}

void MainWindow::clearOverlay() {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->overlay.clearItems();
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addText(QString text, int x, int y, QString color, int fontSize) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->overlay.addText(text, x, y, color, fontSize);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addRect(int x, int y, int width, int height, QString color) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->overlay.addRect(x, y, width, height, color, false);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addFilledRect(int x, int y, int width, int height, QString color) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->overlay.addRect(x, y, width, height, color, true);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::addImage(int x, int y, QString filepath) {
    if (!this->ui || !this->ui->graphicsView_Map)
        return;
    this->ui->graphicsView_Map->overlay.addImage(x, y, filepath);
    this->ui->graphicsView_Map->scene()->update();
}

void MainWindow::refreshAfterPaletteChange(Tileset *tileset) {
    if (this->tilesetEditor) {
        this->tilesetEditor->setTilesets(this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label);
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
    if (paletteIndex >= tileset->palettes->size())
        return;
    if (colors.size() != 16)
        return;

    for (int i = 0; i < 16; i++) {
        if (colors[i].size() != 3)
            continue;
        (*tileset->palettes)[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
        (*tileset->palettePreviews)[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
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

QJSValue MainWindow::getTilesetPalette(QList<QList<QRgb>> *palettes, int paletteIndex) {
    if (paletteIndex >= palettes->size())
        return QJSValue();

    QList<QList<int>> palette;
    for (auto color : palettes->value(paletteIndex)) {
        palette.append(QList<int>({qRed(color), qGreen(color), qBlue(color)}));
    }
    return Scripting::getEngine()->toScriptValue(palette);
}

QJSValue MainWindow::getTilesetPalettes(QList<QList<QRgb>> *palettes) {
    QList<QList<QList<int>>> outPalettes;
    for (int i = 0; i < palettes->size(); i++) {
        QList<QList<int>> colors;
        for (auto color : palettes->value(i)) {
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
    if (paletteIndex >= tileset->palettePreviews->size())
        return;
    if (colors.size() != 16)
        return;

    for (int i = 0; i < 16; i++) {
        if (colors[i].size() != 3)
            continue;
        auto palettes = tileset->palettePreviews;
        (*palettes)[paletteIndex][i] = qRgb(colors[i][0], colors[i][1], colors[i][2]);
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
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    timer->setSingleShot(true);
    timer->start(milliseconds);
}

void MainWindow::invokeCallback(QJSValue callback) {
    callback.call();
}

void MainWindow::log(QString message) {
    logInfo(message);
}
