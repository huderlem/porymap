#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "editcommands.h"
#include "config.h"
#include "imageproviders.h"

// TODO: "tilesetNeedsRedraw" is used when redrawing the map after
// changing a metatile's tiles via script. It is unnecessarily
// resource intensive. The map metatiles that need to be updated are
// not marked as changed, so they will not be redrawn if the cache
// isn't ignored. Ideally the setMetatileTiles functions would properly
// set each of the map spaces that use the modified metatile so that
// the cache could be used, though this would lkely still require a
// full read of the map.
void MainWindow::tryRedrawMapArea(bool forceRedraw) {
    if (!forceRedraw) return;

    if (this->tilesetNeedsRedraw) {
        // Refresh anything that can display metatiles
        this->editor->map_item->draw(true);
        this->editor->collision_item->draw(true);
        this->editor->selected_border_metatiles_item->draw();
        this->editor->updateMapBorder();
        this->editor->updateMapConnections();
        if (this->tilesetEditor)
            this->tilesetEditor->updateTilesets(this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label);
        if (this->editor->metatile_selector_item)
            this->editor->metatile_selector_item->draw();
        if (this->editor->selected_border_metatiles_item)
            this->editor->selected_border_metatiles_item->draw();
        if (this->editor->current_metatile_selection_item)
            this->editor->current_metatile_selection_item->draw();
        this->tilesetNeedsRedraw = false;
    } else {
        this->editor->map_item->draw();
        this->editor->collision_item->draw();
        this->editor->selected_border_metatiles_item->draw();
        this->editor->updateMapBorder();
    }
}

void MainWindow::tryCommitMapChanges(bool commitChanges) {
    if (commitChanges) {
        Map *map = this->editor->map;
        if (map) {
            map->editHistory.push(new ScriptEditMap(map,
                map->layout->lastCommitBlocks.mapDimensions, QSize(map->getWidth(), map->getHeight()),
                map->layout->lastCommitBlocks.blocks, map->layout->blockdata,
                map->layout->lastCommitBlocks.borderDimensions, QSize(map->getBorderWidth(), map->getBorderHeight()),
                map->layout->lastCommitBlocks.border, map->layout->border
            ));
        }
    }
}

//=====================
// Editing map blocks
//=====================

QJSValue MainWindow::getBlock(int x, int y) {
    if (!this->editor || !this->editor->map)
        return QJSValue();
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return Scripting::fromBlock(Block());
    }
    return Scripting::fromBlock(block);
}

void MainWindow::setBlock(int x, int y, int metatileId, int collision, int elevation, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map->setBlock(x, y, Block(metatileId, collision, elevation));
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

void MainWindow::setBlock(int x, int y, int rawValue, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map->setBlock(x, y, Block(static_cast<uint16_t>(rawValue)));
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
    return block.metatileId();
}

void MainWindow::setMetatileId(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(metatileId, block.collision(), block.elevation()));
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
    return block.collision();
}

void MainWindow::setCollision(int x, int y, int collision, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block.metatileId(), collision, block.elevation()));
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
    return block.elevation();
}

void MainWindow::setElevation(int x, int y, int elevation, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    Block block;
    if (!this->editor->map->getBlock(x, y, &block)) {
        return;
    }
    this->editor->map->setBlock(x, y, Block(block.metatileId(), block.collision(), elevation));
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

//=====================
// Editing map border
//=====================

int MainWindow::getBorderMetatileId(int x, int y) {
    if (!this->editor || !this->editor->map)
        return 0;
    if (!this->editor->map->isWithinBorderBounds(x, y))
        return 0;
    return this->editor->map->getBorderMetatileId(x, y);
}

void MainWindow::setBorderMetatileId(int x, int y, int metatileId, bool forceRedraw, bool commitChanges) {
    if (!this->editor || !this->editor->map)
        return;
    if (!this->editor->map->isWithinBorderBounds(x, y))
        return;
    this->editor->map->setBorderMetatileId(x, y, metatileId);
    this->tryCommitMapChanges(commitChanges);
    this->tryRedrawMapArea(forceRedraw);
}

QJSValue MainWindow::getBorderDimensions() {
    if (!this->editor || !this->editor->map)
        return QJSValue();
    return Scripting::dimensions(this->editor->map->getBorderWidth(), this->editor->map->getBorderHeight());
}

int MainWindow::getBorderWidth() {
    if (!this->editor || !this->editor->map)
        return 0;
    return this->editor->map->getBorderWidth();
}

int MainWindow::getBorderHeight() {
    if (!this->editor || !this->editor->map)
        return 0;
    return this->editor->map->getBorderHeight();
}

void MainWindow::setBorderDimensions(int width, int height) {
    if (!this->editor || !this->editor->map || !projectConfig.getUseCustomBorderSize())
        return;
    if (width < 1 || height < 1 || width > MAX_BORDER_WIDTH || height > MAX_BORDER_HEIGHT)
        return;
    this->editor->map->setBorderDimensions(width, height);
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

void MainWindow::setBorderWidth(int width) {
    if (!this->editor || !this->editor->map || !projectConfig.getUseCustomBorderSize())
        return;
    if (width < 1 || width > MAX_BORDER_WIDTH)
        return;
    this->editor->map->setBorderDimensions(width, this->editor->map->getBorderHeight());
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

void MainWindow::setBorderHeight(int height) {
    if (!this->editor || !this->editor->map || !projectConfig.getUseCustomBorderSize())
        return;
    if (height < 1 || height > MAX_BORDER_HEIGHT)
        return;
    this->editor->map->setBorderDimensions(this->editor->map->getBorderWidth(), height);
    this->tryCommitMapChanges(true);
    this->onMapNeedsRedrawing();
}

//======================
// Editing map tilesets
//======================

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

void MainWindow::setPrimaryTilesetPalette(int paletteIndex, QList<QList<int>> colors, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    this->setTilesetPalette(this->editor->map->layout->tileset_primary, paletteIndex, colors);
    if (forceRedraw) {
        this->refreshAfterPaletteChange(this->editor->map->layout->tileset_primary);
    }
}

void MainWindow::setPrimaryTilesetPalettes(QList<QList<QList<int>>> palettes, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalette(this->editor->map->layout->tileset_primary, i, palettes[i]);
    }
    if (forceRedraw) {
        this->refreshAfterPaletteChange(this->editor->map->layout->tileset_primary);
    }
}

void MainWindow::setSecondaryTilesetPalette(int paletteIndex, QList<QList<int>> colors, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    this->setTilesetPalette(this->editor->map->layout->tileset_secondary, paletteIndex, colors);
    if (forceRedraw) {
        this->refreshAfterPaletteChange(this->editor->map->layout->tileset_secondary);
    }
}

void MainWindow::setSecondaryTilesetPalettes(QList<QList<QList<int>>> palettes, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalette(this->editor->map->layout->tileset_secondary, i, palettes[i]);
    }
    if (forceRedraw) {
        this->refreshAfterPaletteChange(this->editor->map->layout->tileset_secondary);
    }
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

void MainWindow::setPrimaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    this->setTilesetPalettePreview(this->editor->map->layout->tileset_primary, paletteIndex, colors);
    if (forceRedraw) {
        this->refreshAfterPalettePreviewChange();
    }
}

void MainWindow::setPrimaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalettePreview(this->editor->map->layout->tileset_primary, i, palettes[i]);
    }
    if (forceRedraw) {
        this->refreshAfterPalettePreviewChange();
    }
}

void MainWindow::setSecondaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    this->setTilesetPalettePreview(this->editor->map->layout->tileset_secondary, paletteIndex, colors);
    if (forceRedraw) {
        this->refreshAfterPalettePreviewChange();
    }
}

void MainWindow::setSecondaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes, bool forceRedraw) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return;
    for (int i = 0; i < palettes.size(); i++) {
        this->setTilesetPalettePreview(this->editor->map->layout->tileset_secondary, i, palettes[i]);
    }
    if (forceRedraw) {
        this->refreshAfterPalettePreviewChange();
    }
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

int MainWindow::getNumSecondaryTilesetMetatiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return 0;
    return this->editor->map->layout->tileset_secondary->metatiles.length();
}

int MainWindow::getNumPrimaryTilesetTiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_primary)
        return 0;
    return this->editor->map->layout->tileset_primary->tiles.length();
}

int MainWindow::getNumSecondaryTilesetTiles() {
    if (!this->editor || !this->editor->map || !this->editor->map->layout || !this->editor->map->layout->tileset_secondary)
        return 0;
    return this->editor->map->layout->tileset_secondary->tiles.length();
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

void MainWindow::saveMetatilesByMetatileId(int metatileId) {
    Tileset * tileset = Tileset::getMetatileTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project && tileset)
        this->editor->project->saveTilesetMetatiles(tileset);
}

void MainWindow::saveMetatileAttributesByMetatileId(int metatileId) {
    Tileset * tileset = Tileset::getMetatileTileset(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
    if (this->editor->project && tileset)
        this->editor->project->saveTilesetMetatileAttributes(tileset);

    // If the tileset editor is open it needs to be refreshed with the new changes.
    // Rather than do a full refresh (which is costly) we tell the editor it will need
    // to reload the metatile from the project next time it's displayed.
    // If it's currently being displayed, trigger this reload immediately.
    if (this->tilesetEditor) {
        this->tilesetEditor->queueMetatileReload(metatileId);
        if (this->tilesetEditor->getSelectedMetatileId() == metatileId)
            this->tilesetEditor->onSelectedMetatileChanged(metatileId);
    }
}

Metatile * MainWindow::getMetatile(int metatileId) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return nullptr;
    return Tileset::getMetatile(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
}

QString MainWindow::getMetatileLabel(int metatileId) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return QString();
    return Tileset::getMetatileLabel(metatileId, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
}

void MainWindow::setMetatileLabel(int metatileId, QString label) {
    if (!this->editor || !this->editor->map || !this->editor->map->layout)
        return;

    // If the Tileset Editor is opened on this metatile we need to update the text box
    if (this->tilesetEditor && this->tilesetEditor->getSelectedMetatileId() == metatileId){
        this->tilesetEditor->setMetatileLabel(label);
        return;
    }

    if (!Tileset::setMetatileLabel(metatileId, label, this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary)) {
        logError("Failed to set metatile label. Must be a valid metatile id and a label containing only letters, numbers, and underscores.");
        return;
    }

    if (this->editor->project)
        this->editor->project->saveTilesetMetatileLabels(this->editor->map->layout->tileset_primary, this->editor->map->layout->tileset_secondary);
}

int MainWindow::getMetatileLayerType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->layerType();
}

void MainWindow::setMetatileLayerType(int metatileId, int layerType) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;
    metatile->setLayerType(layerType);
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileEncounterType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->encounterType();
}

void MainWindow::setMetatileEncounterType(int metatileId, int encounterType) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;
    metatile->setEncounterType(encounterType);
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileTerrainType(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->terrainType();
}

void MainWindow::setMetatileTerrainType(int metatileId, int terrainType) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;
    metatile->setTerrainType(terrainType);
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileBehavior(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->behavior();
}

void MainWindow::setMetatileBehavior(int metatileId, int behavior) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;
    metatile->setBehavior(behavior);
    this->saveMetatileAttributesByMetatileId(metatileId);
}

QString MainWindow::getMetatileBehaviorName(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile || !this->editor->project)
        return QString();
    return this->editor->project->metatileBehaviorMapInverse.value(metatile->behavior(), QString());
}

void MainWindow::setMetatileBehaviorName(int metatileId, QString behavior) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile || !this->editor->project)
        return;
    if (!this->editor->project->metatileBehaviorMap.contains(behavior)) {
        logError(QString("Unknown metatile behavior '%1'").arg(behavior));
        return;
    }
    metatile->setBehavior(this->editor->project->metatileBehaviorMap.value(behavior));
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::getMetatileAttributes(int metatileId) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return -1;
    return metatile->getAttributes();
}

void MainWindow::setMetatileAttributes(int metatileId, int attributes) {
    Metatile * metatile = this->getMetatile(metatileId);
    if (!metatile)
        return;
    metatile->setAttributes(attributes);
    this->saveMetatileAttributesByMetatileId(metatileId);
}

int MainWindow::calculateTileBounds(int * tileStart, int * tileEnd) {
    int maxNumTiles = projectConfig.getNumTilesInMetatile();
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
    this->tilesetNeedsRedraw = true;
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
    this->tilesetNeedsRedraw = true;
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

//=====================
// Editing map header
//=====================

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
    return this->editor->map->requiresFlash;
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
    return this->editor->map->show_location;
}

void MainWindow::setShowLocationName(bool show) {
    if (!this->ui)
        return;
    this->ui->checkBox_ShowLocation->setChecked(show);
}

bool MainWindow::getAllowRunning() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->editor->map->allowRunning;
}

void MainWindow::setAllowRunning(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowRunning->setChecked(allow);
}

bool MainWindow::getAllowBiking() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->editor->map->allowBiking;
}

void MainWindow::setAllowBiking(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowBiking->setChecked(allow);
}

bool MainWindow::getAllowEscaping() {
    if (!this->editor || !this->editor->map)
        return false;
    return this->editor->map->allowEscaping;
}

void MainWindow::setAllowEscaping(bool allow) {
    if (!this->ui)
        return;
    this->ui->checkBox_AllowEscaping->setChecked(allow);
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

