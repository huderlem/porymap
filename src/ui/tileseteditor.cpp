#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "log.h"
#include "imageproviders.h"
#include "metatileparser.h"
#include "paletteutil.h"
#include "imageexport.h"
#include "config.h"
#include "shortcut.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QImageReader>

TilesetEditor::TilesetEditor(Project *project, Map *map, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor),
    project(project),
    map(map),
    hasUnsavedChanges(false)
{
    this->setTilesets(this->map->layout->tileset_primary_label, this->map->layout->tileset_secondary_label);
    this->initUi();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
    delete metatileSelector;
    delete tileSelector;
    delete metatileLayersItem;
    delete paletteEditor;
    delete metatile;
    delete primaryTileset;
    delete secondaryTileset;
    delete metatilesScene;
    delete tilesScene;
    delete selectedTilePixmapItem;
    delete selectedTileScene;
    delete metatileLayersScene;
    delete copiedMetatile;
}

void TilesetEditor::update(Map *map, QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    this->updateMap(map);
    this->updateTilesets(primaryTilesetLabel, secondaryTilesetLabel);
}

void TilesetEditor::updateMap(Map *map) {
    this->map = map;
    this->metatileSelector->map = map;
}

void TilesetEditor::updateTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    if (this->hasUnsavedChanges) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            "porymap",
            "Tileset has been modified, save changes?",
            QMessageBox::No | QMessageBox::Yes,
            QMessageBox::Yes);
        if (result == QMessageBox::Yes)
            this->on_actionSave_Tileset_triggered();
    }
    this->setTilesets(primaryTilesetLabel, secondaryTilesetLabel);
    this->refresh();
}

bool TilesetEditor::selectMetatile(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset) || this->lockSelection)
        return false;
    this->metatileSelector->select(metatileId);
    QPoint pos = this->metatileSelector->getMetatileIdCoordsOnWidget(metatileId);
    this->ui->scrollArea_Metatiles->ensureVisible(pos.x(), pos.y());
    return true;
}

uint16_t TilesetEditor::getSelectedMetatileId() {
    return this->metatileSelector->getSelectedMetatileId();
}

void TilesetEditor::setTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    this->metatileReloadQueue.clear();
    Tileset *primaryTileset = project->getTileset(primaryTilesetLabel);
    Tileset *secondaryTileset = project->getTileset(secondaryTilesetLabel);
    if (this->primaryTileset) delete this->primaryTileset;
    if (this->secondaryTileset) delete this->secondaryTileset;
    this->primaryTileset = new Tileset(*primaryTileset);
    this->secondaryTileset = new Tileset(*secondaryTileset);
    if (paletteEditor) paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->initMetatileHistory();
}

void TilesetEditor::initUi() {
    ui->setupUi(this);
    this->tileXFlip = ui->checkBox_xFlip->isChecked();
    this->tileYFlip = ui->checkBox_yFlip->isChecked();
    this->paletteId = ui->spinBox_paletteSelector->value();
    this->ui->spinBox_paletteSelector->setMinimum(0);
    this->ui->spinBox_paletteSelector->setMaximum(Project::getNumPalettesTotal() - 1);

    this->setAttributesUi();
    this->setMetatileLabelValidator();

    this->initMetatileSelector();
    this->initMetatileLayersItem();
    this->initTileSelector();
    this->initSelectedTileItem();
    this->initShortcuts();
    this->metatileSelector->select(0);
    this->restoreWindowState();
}

void TilesetEditor::setAttributesUi() {
    // Behavior
    if (projectConfig.getMetatileBehaviorMask()) {
        for (int num : project->metatileBehaviorMapInverse.keys()) {
            this->ui->comboBox_metatileBehaviors->addItem(project->metatileBehaviorMapInverse[num], num);
        }
        this->ui->comboBox_metatileBehaviors->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_metatileBehaviors->setVisible(false);
        this->ui->label_metatileBehavior->setVisible(false);
    }

    // Terrain Type
    if (projectConfig.getMetatileTerrainTypeMask()) {
        this->ui->comboBox_terrainType->addItem("Normal", TERRAIN_NONE);
        this->ui->comboBox_terrainType->addItem("Grass", TERRAIN_GRASS);
        this->ui->comboBox_terrainType->addItem("Water", TERRAIN_WATER);
        this->ui->comboBox_terrainType->addItem("Waterfall", TERRAIN_WATERFALL);
        this->ui->comboBox_terrainType->setEditable(false);
        this->ui->comboBox_terrainType->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_terrainType->setVisible(false);
        this->ui->label_terrainType->setVisible(false);
    }

    // Encounter Type
    if (projectConfig.getMetatileEncounterTypeMask()) {
        this->ui->comboBox_encounterType->addItem("None", ENCOUNTER_NONE);
        this->ui->comboBox_encounterType->addItem("Land", ENCOUNTER_LAND);
        this->ui->comboBox_encounterType->addItem("Water", ENCOUNTER_WATER);
        this->ui->comboBox_encounterType->setEditable(false);
        this->ui->comboBox_encounterType->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_encounterType->setVisible(false);
        this->ui->label_encounterType->setVisible(false);
    }

    // Layer Type
    if (!projectConfig.getTripleLayerMetatilesEnabled()) {
        this->ui->comboBox_layerType->addItem("Normal - Middle/Top", METATILE_LAYER_MIDDLE_TOP);
        this->ui->comboBox_layerType->addItem("Covered - Bottom/Middle", METATILE_LAYER_BOTTOM_MIDDLE);
        this->ui->comboBox_layerType->addItem("Split - Bottom/Top", METATILE_LAYER_BOTTOM_TOP);
        this->ui->comboBox_layerType->setEditable(false);
        this->ui->comboBox_layerType->setMinimumContentsLength(0);
        if (!projectConfig.getMetatileLayerTypeMask()) {
            // User doesn't have triple layer metatiles, but has no layer type attribute.
            // Porymap is still using the layer type value to render metatiles, and with
            // no mask set every metatile will be "Middle/Top", so just display the combo
            // box but prevent the user from changing the value.
            this->ui->comboBox_layerType->setEnabled(false);
        }
    } else {
        this->ui->comboBox_layerType->setVisible(false);
        this->ui->label_layerType->setVisible(false);
        this->ui->label_BottomTop->setText("Bottom/Middle/Top");
    }
}

void TilesetEditor::setMetatileLabelValidator() {
    //only allow characters valid for a symbol
    static const QRegularExpression expression("[_A-Za-z0-9]*$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    this->ui->lineEdit_metatileLabel->setValidator(validator);
}

void TilesetEditor::initMetatileSelector()
{
    this->metatileSelector = new TilesetEditorMetatileSelector(this->primaryTileset, this->secondaryTileset, this->map);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileChanged,
            this, &TilesetEditor::onHoveredMetatileChanged);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileCleared,
            this, &TilesetEditor::onHoveredMetatileCleared);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::selectedMetatileChanged,
            this, &TilesetEditor::onSelectedMetatileChanged);

    bool showGrid = porymapConfig.getShowTilesetEditorMetatileGrid();
    this->ui->actionMetatile_Grid->setChecked(showGrid);
    this->metatileSelector->showGrid = showGrid;

    this->metatilesScene = new QGraphicsScene;
    this->metatilesScene->addItem(this->metatileSelector);
    this->metatileSelector->draw();

    this->ui->graphicsView_Metatiles->setScene(this->metatilesScene);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
}

void TilesetEditor::initMetatileLayersItem() {
    Metatile *metatile = Tileset::getMetatile(this->getSelectedMetatileId(), this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, &MetatileLayersItem::tileChanged,
            this, &TilesetEditor::onMetatileLayerTileChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::selectedTilesChanged,
            this, &TilesetEditor::onMetatileLayerSelectionChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileChanged,
            this, &TilesetEditor::onHoveredTileChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileCleared,
            this, &TilesetEditor::onHoveredTileCleared);

    bool showGrid = porymapConfig.getShowTilesetEditorLayerGrid();
    this->ui->actionLayer_Grid->setChecked(showGrid);
    this->metatileLayersItem->showGrid = showGrid;

    this->metatileLayersScene = new QGraphicsScene;
    this->metatileLayersScene->addItem(this->metatileLayersItem);
    this->ui->graphicsView_metatileLayers->setScene(this->metatileLayersScene);
}

void TilesetEditor::initTileSelector()
{
    this->tileSelector = new TilesetEditorTileSelector(this->primaryTileset, this->secondaryTileset, projectConfig.getNumLayersInMetatile());
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileChanged,
            this, &TilesetEditor::onHoveredTileChanged);
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileCleared,
            this, &TilesetEditor::onHoveredTileCleared);
    connect(this->tileSelector, &TilesetEditorTileSelector::selectedTilesChanged,
            this, &TilesetEditor::onSelectedTilesChanged);

    this->tilesScene = new QGraphicsScene;
    this->tilesScene->addItem(this->tileSelector);
    this->tileSelector->select(0);
    this->tileSelector->draw();

    this->ui->graphicsView_Tiles->setScene(this->tilesScene);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
}

void TilesetEditor::initSelectedTileItem() {
    this->selectedTileScene = new QGraphicsScene;
    this->drawSelectedTiles();
    this->ui->graphicsView_selectedTile->setScene(this->selectedTileScene);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2, this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::initShortcuts() {
    initExtraShortcuts();

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();
}

void TilesetEditor::initExtraShortcuts() {
    ui->actionRedo->setShortcuts({ui->actionRedo->shortcut(), QKeySequence("Ctrl+Shift+Z")});

    auto *shortcut_xFlip = new Shortcut(QKeySequence(), ui->checkBox_xFlip, SLOT(toggle()));
    shortcut_xFlip->setObjectName("shortcut_xFlip");
    shortcut_xFlip->setWhatsThis("X Flip");

    auto *shortcut_yFlip = new Shortcut(QKeySequence(), ui->checkBox_yFlip, SLOT(toggle()));
    shortcut_yFlip->setObjectName("shortcut_yFlip");
    shortcut_yFlip->setWhatsThis("Y Flip");
}

QObjectList TilesetEditor::shortcutableObjects() const {
    QObjectList shortcutable_objects;

    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(shortcut));

    return shortcutable_objects;
}

void TilesetEditor::applyUserShortcuts() {
    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            action->setShortcuts(shortcutsConfig.userShortcuts(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcut->setKeys(shortcutsConfig.userShortcuts(shortcut));
}

void TilesetEditor::restoreWindowState() {
    logInfo("Restoring tileset editor geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getTilesetEditorGeometry();
    this->restoreGeometry(geometry.value("tileset_editor_geometry"));
    this->restoreState(geometry.value("tileset_editor_state"));
}

void TilesetEditor::initMetatileHistory() {
    metatileHistory.clear();
    MetatileHistoryItem *commit = new MetatileHistoryItem(0, nullptr, new Metatile(), QString(), QString());
    metatileHistory.push(commit);
    this->hasUnsavedChanges = false;
}

void TilesetEditor::reset() {
    this->setTilesets(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor)
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->refresh();
}

void TilesetEditor::refresh() {
    this->metatileLayersItem->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->tileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->select(this->getSelectedMetatileId());
    this->drawSelectedTiles();

    if (metatileSelector) {
        if (metatileSelector->selectorShowUnused || metatileSelector->selectorShowCounts) {
            countMetatileUsage();
            this->metatileSelector->draw();
        }
    }

    if (tileSelector) {
        if (tileSelector->showUnused) {
            countTileUsage();
            this->tileSelector->draw();
        }
    }

    this->ui->graphicsView_Tiles->setSceneRect(0, 0, this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setSceneRect(0, 0, this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2, this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::drawSelectedTiles() {
    if (!this->selectedTileScene) {
        return;
    }

    this->selectedTileScene->clear();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QImage selectionImage(16 * dimensions.x(), 16 * dimensions.y(), QImage::Format_RGBA8888);
    QPainter painter(&selectionImage);
    int tileIndex = 0;
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            QImage tileImage = getPalettedTileImage(tiles.at(tileIndex).tileId, this->primaryTileset, this->secondaryTileset, tiles.at(tileIndex).palette, true)
                    .mirrored(tiles.at(tileIndex).xflip, tiles.at(tileIndex).yflip)
                    .scaled(16, 16);
            tileIndex++;
            painter.drawImage(i * 16, j * 16, tileImage);
        }
    }

    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(selectionImage));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2, this->selectedTilePixmapItem->pixmap().height() + 2);
}

void TilesetEditor::onHoveredMetatileChanged(uint16_t metatileId) {
    QString label = Tileset::getMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    QString message = QString("Metatile: %1").arg(Metatile::getMetatileIdString(metatileId));
    if (label.size() != 0) {
        message += QString(" \"%1\"").arg(label);
    }
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredMetatileCleared() {
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedMetatileChanged(uint16_t metatileId) {
    this->metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);

    // The scripting API allows users to change metatiles in the project, and these changes are saved to disk.
    // The Tileset Editor (if open) needs to reflect these changes when the metatile is next displayed.
    if (this->metatileReloadQueue.contains(metatileId)) {
        this->metatileReloadQueue.remove(metatileId);
        Metatile *updatedMetatile = Tileset::getMetatile(metatileId, this->map->layout->tileset_primary, this->map->layout->tileset_secondary);
        if (updatedMetatile) *this->metatile = *updatedMetatile;
    }

    this->metatileLayersItem->setMetatile(metatile);
    this->metatileLayersItem->draw();
    this->ui->graphicsView_metatileLayers->setFixedSize(this->metatileLayersItem->pixmap().width() + 2, this->metatileLayersItem->pixmap().height() + 2);

    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, this->primaryTileset, this->secondaryTileset);
    this->ui->lineEdit_metatileLabel->setText(labels.owned);
    this->ui->lineEdit_metatileLabel->setPlaceholderText(labels.shared);

    this->ui->comboBox_metatileBehaviors->setHexItem(this->metatile->behavior());
    this->ui->comboBox_layerType->setHexItem(this->metatile->layerType());
    this->ui->comboBox_encounterType->setHexItem(this->metatile->encounterType());
    this->ui->comboBox_terrainType->setHexItem(this->metatile->terrainType());
}

void TilesetEditor::queueMetatileReload(uint16_t metatileId) {
    this->metatileReloadQueue.insert(metatileId);
}

void TilesetEditor::onHoveredTileChanged(uint16_t tile) {
    QString message = QString("Tile: 0x%1")
                        .arg(QString("%1").arg(tile, 3, 16, QChar('0')).toUpper());
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredTileCleared() {
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedTilesChanged() {
    this->drawSelectedTiles();
}

void TilesetEditor::onMetatileLayerTileChanged(int x, int y) {
    static const QList<QPoint> tileCoords = QList<QPoint>{
        QPoint(0, 0),
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(1, 1),
        QPoint(2, 0),
        QPoint(3, 0),
        QPoint(2, 1),
        QPoint(3, 1),
        QPoint(4, 0),
        QPoint(5, 0),
        QPoint(4, 1),
        QPoint(5, 1),
    };
    Metatile *prevMetatile = new Metatile(*this->metatile);
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    int selectedTileIndex = 0;
    int maxTileIndex = projectConfig.getNumTilesInMetatile();
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < maxTileIndex
             && tileCoords.at(tileIndex).x() >= x
             && tileCoords.at(tileIndex).y() >= y){
                Tile &tile = this->metatile->tiles[tileIndex];
                tile.tileId = tiles.at(selectedTileIndex).tileId;
                tile.xflip = tiles.at(selectedTileIndex).xflip;
                tile.yflip = tiles.at(selectedTileIndex).yflip;
                tile.palette = tiles.at(selectedTileIndex).palette;
            }
            selectedTileIndex++;
        }
    }

    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
    this->commitMetatileChange(prevMetatile);
}

void TilesetEditor::onMetatileLayerSelectionChanged(QPoint selectionOrigin, int width, int height) {
    QList<Tile> tiles;
    QList<int> tileIdxs;
    int x = selectionOrigin.x();
    int y = selectionOrigin.y();
    int maxTileIndex = projectConfig.getNumTilesInMetatile();
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < maxTileIndex) {
                tiles.append(this->metatile->tiles.at(tileIndex));
                tileIdxs.append(tileIndex);
            }
        }
    }

    if (width == 1 && height == 1) {
        this->tileSelector->highlight(static_cast<uint16_t>(tiles[0].tileId));
        ui->spinBox_paletteSelector->setValue(tiles[0].palette);
        QPoint pos = tileSelector->getTileCoordsOnWidget(static_cast<uint16_t>(tiles[0].tileId));
        ui->scrollArea_Tiles->ensureVisible(pos.x(), pos.y());
    }
    this->tileSelector->setExternalSelection(width, height, tiles, tileIdxs);
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_spinBox_paletteSelector_valueChanged(int paletteId)
{
    this->ui->spinBox_paletteSelector->blockSignals(true);
    this->ui->spinBox_paletteSelector->setValue(paletteId);
    this->ui->spinBox_paletteSelector->blockSignals(false);
    this->paletteId = paletteId;
    this->tileSelector->setPaletteId(paletteId);
    this->drawSelectedTiles();
    if (this->paletteEditor) {
        this->paletteEditor->setPaletteId(paletteId);
    }
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_checkBox_xFlip_stateChanged(int checked)
{
    this->tileXFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_checkBox_yFlip_stateChanged(int checked)
{
    this->tileYFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::on_comboBox_metatileBehaviors_currentTextChanged(const QString &metatileBehavior)
{
    if (this->metatile) {
        uint32_t behavior;
        if (project->metatileBehaviorMap.contains(metatileBehavior)) {
            behavior = project->metatileBehaviorMap[metatileBehavior];
        } else {
            // Check if user has entered a number value instead
            bool ok;
            behavior = metatileBehavior.toUInt(&ok, 0);
            if (!ok) return;
        }

        // This function can also be called when the user selects
        // a different metatile. Stop this from being considered a change.
        if (this->metatile->behavior() == behavior)
            return;

        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setBehavior(behavior);
        this->commitMetatileChange(prevMetatile);
    }
}

void TilesetEditor::setMetatileLabel(QString label)
{
    this->ui->lineEdit_metatileLabel->setText(label);
    commitMetatileLabel();
}

void TilesetEditor::on_lineEdit_metatileLabel_editingFinished()
{
    commitMetatileLabel();
}

void TilesetEditor::commitMetatileLabel()
{
    // Only commit if the field has changed.
    uint16_t metatileId = this->getSelectedMetatileId();
    QString oldLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    QString newLabel = this->ui->lineEdit_metatileLabel->text();
    if (oldLabel != newLabel) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        Tileset::setMetatileLabel(metatileId, newLabel, this->primaryTileset, this->secondaryTileset);
        this->commitMetatileAndLabelChange(prevMetatile, oldLabel);
    }
}

void TilesetEditor::commitMetatileAndLabelChange(Metatile * prevMetatile, QString prevLabel)
{
    metatileHistory.push(new MetatileHistoryItem(this->getSelectedMetatileId(),
                                                 prevMetatile, new Metatile(*this->metatile),
                                                 prevLabel, this->ui->lineEdit_metatileLabel->text()));
    this->hasUnsavedChanges = true;
}

void TilesetEditor::commitMetatileChange(Metatile * prevMetatile)
{
    this->commitMetatileAndLabelChange(prevMetatile, this->ui->lineEdit_metatileLabel->text());
}

void TilesetEditor::on_comboBox_layerType_activated(int layerType)
{
    if (this->metatile) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setLayerType(layerType);
        this->commitMetatileChange(prevMetatile);
        this->metatileSelector->draw(); // Changing the layer type can affect how fully transparent metatiles appear
    }
}

void TilesetEditor::on_comboBox_encounterType_activated(int encounterType)
{
    if (this->metatile) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setEncounterType(encounterType);
        this->commitMetatileChange(prevMetatile);
    }
}

void TilesetEditor::on_comboBox_terrainType_activated(int terrainType)
{
    if (this->metatile) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setTerrainType(terrainType);
        this->commitMetatileChange(prevMetatile);
    }
}

void TilesetEditor::on_actionSave_Tileset_triggered()
{
    // Need this temporary flag to stop selection resetting after saving.
    // This is a workaround; redrawing the map's metatile selector shouldn't emit the same signal as when it's selected.
    this->lockSelection = true;
    this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    emit this->tilesetsSaved(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor) {
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    }
    this->ui->statusbar->showMessage(QString("Saved primary and secondary Tilesets!"), 5000);
    this->hasUnsavedChanges = false;
    this->lockSelection = false;
}

void TilesetEditor::on_actionImport_Primary_Tiles_triggered()
{
    this->importTilesetTiles(this->primaryTileset, true);
}

void TilesetEditor::on_actionImport_Secondary_Tiles_triggered()
{
    this->importTilesetTiles(this->secondaryTileset, false);
}

void TilesetEditor::importTilesetTiles(Tileset *tileset, bool primary) {
    QString descriptor = primary ? "primary" : "secondary";
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Import %1 Tileset Tiles Image").arg(descriptorCaps),
                this->project->importExportPath,
                "Image Files (*.png *.bmp *.jpg *.dib)");
    if (filepath.isEmpty()) {
        return;
    }
    this->project->setImportExportPath(filepath);
    logInfo(QString("Importing %1 tileset tiles '%2'").arg(descriptor).arg(filepath));

    // Read image data from buffer so that the built-in QImage doesn't try to detect file format
    // purely from the extension name. Advance Map exports ".png" files that are actually BMP format, for example.
    QFile file(filepath);
    QImage image;
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray imageData = file.readAll();
        image = QImage::fromData(imageData);
    } else {
        logError(QString("Failed to open image file: '%1'").arg(filepath));
    }
    if (image.width() == 0 || image.height() == 0 || image.width() % 8 != 0 || image.height() % 8 != 0) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(QString("The image dimensions (%1 x %2) are invalid. Width and height must be multiples of 8 pixels.")
                                  .arg(image.width())
                                  .arg(image.height()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Validate total number of tiles in image.
    int numTilesWide = image.width() / 8;
    int numTilesHigh = image.height() / 8;
    int totalTiles = numTilesHigh * numTilesWide;
    int maxAllowedTiles = primary ? Project::getNumTilesPrimary() : Project::getNumTilesTotal() - Project::getNumTilesPrimary();
    if (totalTiles > maxAllowedTiles) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(QString("The maximum number of tiles allowed in the %1 tileset is %2, but the provided image contains %3 total tiles.")
                                  .arg(descriptor)
                                  .arg(maxAllowedTiles)
                                  .arg(totalTiles));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Ask user to provide a palette for the un-indexed image.
    if (image.colorCount() == 0) {
        QMessageBox msgBox(this);
        msgBox.setText("Select Palette for Tiles");
        msgBox.setInformativeText(QString("The provided image is not indexed. Please select a palette file for the image. An indexed image will be generated using the provided image and palette.")
                                  .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        msgBox.exec();

        QString filepath = QFileDialog::getOpenFileName(
            this,
            QString("Select Palette for Tiles Image").arg(descriptorCaps),
            this->project->importExportPath,
            "Palette Files (*.pal *.act *tpl *gpl)");
        if (filepath.isEmpty()) {
            return;
        }
        this->project->setImportExportPath(filepath);
        bool error = false;
        QList<QRgb> palette = PaletteUtil::parse(filepath, &error);
        if (error) {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to import palette.");
            QString message = QString("The palette file could not be processed. View porymap.log for specific errors.");
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }

        QVector<QRgb> colorTable = palette.toVector();
        image = image.convertToFormat(QImage::Format::Format_Indexed8, colorTable);
    }

    // Validate image is properly indexed to 16 colors.
    int colorCount = image.colorCount();
    if (colorCount > 16) {
        flattenTo4bppImage(&image);
    } else if (colorCount < 16) {
        QVector<QRgb> colorTable = image.colorTable();
        for (int i = colorTable.length(); i < 16; i++) {
            colorTable.append(Qt::black);
        }
        image.setColorTable(colorTable);
    }

    this->project->loadTilesetTiles(tileset, image);
    this->refresh();
    this->hasUnsavedChanges = true;
    tileset->hasUnsavedTilesImage = true;
}

void TilesetEditor::closeEvent(QCloseEvent *event)
{
    if (this->hasUnsavedChanges) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            "porymap",
            "Tileset has been modified, save changes?",
            QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Yes);

        if (result == QMessageBox::Yes) {
            this->on_actionSave_Tileset_triggered();
            event->accept();
        } else if (result == QMessageBox::No) {
            this->reset();
            event->accept();
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
        }
    } else {
        event->accept();
    }

    if (event->isAccepted()) {
        if (this->paletteEditor) this->paletteEditor->close();
        porymapConfig.setTilesetEditorGeometry(
            this->saveGeometry(),
            this->saveState()
        );
    }
}

void TilesetEditor::on_actionChange_Metatiles_Count_triggered()
{
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Change Number of Metatiles");
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);

    QSpinBox *primarySpinBox = new QSpinBox();
    QSpinBox *secondarySpinBox = new QSpinBox();
    primarySpinBox->setMinimum(1);
    secondarySpinBox->setMinimum(1);
    primarySpinBox->setMaximum(Project::getNumMetatilesPrimary());
    secondarySpinBox->setMaximum(Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary());
    primarySpinBox->setValue(this->primaryTileset->metatiles.length());
    secondarySpinBox->setValue(this->secondaryTileset->metatiles.length());
    form.addRow(new QLabel("Primary Tileset"), primarySpinBox);
    form.addRow(new QLabel("Secondary Tileset"), secondarySpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        int numPrimaryMetatiles = primarySpinBox->value();
        int numSecondaryMetatiles = secondarySpinBox->value();
        int numTiles = projectConfig.getNumTilesInMetatile();
        while (this->primaryTileset->metatiles.length() > numPrimaryMetatiles) {
            delete this->primaryTileset->metatiles.takeLast();
        }
        while (this->primaryTileset->metatiles.length() < numPrimaryMetatiles) {
            this->primaryTileset->metatiles.append(new Metatile(numTiles));
        }
        while (this->secondaryTileset->metatiles.length() > numSecondaryMetatiles) {
            delete this->secondaryTileset->metatiles.takeLast();
        }
        while (this->secondaryTileset->metatiles.length() < numSecondaryMetatiles) {
            this->secondaryTileset->metatiles.append(new Metatile(numTiles));
        }

        this->metatileSelector->updateSelectedMetatile();
        this->refresh();
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_actionChange_Palettes_triggered()
{
    if (!this->paletteEditor) {
        this->paletteEditor = new PaletteEditor(this->project, this->primaryTileset,
                                                this->secondaryTileset, this->paletteId, this);
        connect(this->paletteEditor, &PaletteEditor::changedPaletteColor,
                this, &TilesetEditor::onPaletteEditorChangedPaletteColor);
        connect(this->paletteEditor, &PaletteEditor::changedPalette,
                this, &TilesetEditor::onPaletteEditorChangedPalette);
    }

    if (!this->paletteEditor->isVisible()) {
        this->paletteEditor->show();
    } else if (this->paletteEditor->isMinimized()) {
        this->paletteEditor->showNormal();
    } else {
        this->paletteEditor->raise();
        this->paletteEditor->activateWindow();
    }
}

void TilesetEditor::onPaletteEditorChangedPaletteColor() {
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::onPaletteEditorChangedPalette(int paletteId) {
    this->on_spinBox_paletteSelector_valueChanged(paletteId);
}

bool TilesetEditor::replaceMetatile(uint16_t metatileId, const Metatile * src, QString newLabel)
{
    Metatile * dest = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    QString oldLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    if (!dest || !src || (*dest == *src && oldLabel == newLabel))
        return false;

    Tileset::setMetatileLabel(metatileId, newLabel, this->primaryTileset, this->secondaryTileset);
    if (metatileId == this->getSelectedMetatileId())
        this->ui->lineEdit_metatileLabel->setText(newLabel);

    this->metatile = dest;
    *this->metatile = *src;
    this->metatileSelector->select(metatileId);
    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
    this->metatileLayersItem->clearLastModifiedCoords();
    this->metatileLayersItem->clearLastHoveredCoords();
    return true;
}

void TilesetEditor::on_actionUndo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.current();
    if (!commit) return;
    Metatile *prev = commit->prevMetatile;
    if (!prev) return;
    this->metatileHistory.back();
    this->replaceMetatile(commit->metatileId, prev, commit->prevLabel);
}

void TilesetEditor::on_actionRedo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.next();
    if (!commit) return;
    this->replaceMetatile(commit->metatileId, commit->newMetatile, commit->newLabel);
}

void TilesetEditor::on_actionCut_triggered()
{
    Metatile * empty = new Metatile(projectConfig.getNumTilesInMetatile());
    this->copyMetatile(true);
    this->pasteMetatile(empty, "");
    delete empty;
}

void TilesetEditor::on_actionCopy_triggered()
{
    this->copyMetatile(false);
}

void TilesetEditor::on_actionPaste_triggered()
{
    this->pasteMetatile(this->copiedMetatile, this->copiedMetatileLabel);
}

void TilesetEditor::copyMetatile(bool cut) {
    uint16_t metatileId = this->getSelectedMetatileId();
    Metatile * toCopy = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    if (!toCopy) return;

    if (!this->copiedMetatile)
        this->copiedMetatile = new Metatile(*toCopy);
    else
        *this->copiedMetatile = *toCopy;

    // Don't try to copy the label unless it's a cut, these should be unique to each metatile.
    this->copiedMetatileLabel = cut ? Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset) : QString();
}

void TilesetEditor::pasteMetatile(const Metatile * toPaste, QString newLabel)
{
    Metatile *prevMetatile = new Metatile(*this->metatile);
    QString prevLabel = this->ui->lineEdit_metatileLabel->text();
    if (newLabel.isNull()) newLabel = prevLabel; // Don't change the label if one wasn't copied
    uint16_t metatileId = this->getSelectedMetatileId();
    if (!this->replaceMetatile(metatileId, toPaste, newLabel)) {
        delete prevMetatile;
        return;
    }

    this->commitMetatileAndLabelChange(prevMetatile, prevLabel);
}

void TilesetEditor::on_actionExport_Primary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->primaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->importExportPath).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Primary Tiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        this->project->setImportExportPath(filepath);
        QImage image = this->tileSelector->buildPrimaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

void TilesetEditor::on_actionExport_Secondary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->secondaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->importExportPath).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Secondary Tiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        this->project->setImportExportPath(filepath);
        QImage image = this->tileSelector->buildSecondaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

void TilesetEditor::on_actionExport_Primary_Metatiles_Image_triggered()
{
    QString defaultName = QString("%1_Metatiles").arg(this->primaryTileset->name);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->importExportPath).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Primary Metatiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        this->project->setImportExportPath(filepath);
        QImage image = this->metatileSelector->buildPrimaryMetatilesImage();
        image.save(filepath, "PNG");
    }
}

void TilesetEditor::on_actionExport_Secondary_Metatiles_Image_triggered()
{
    QString defaultName = QString("%1_Metatiles").arg(this->secondaryTileset->name);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->importExportPath).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Secondary Metatiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        this->project->setImportExportPath(filepath);
        QImage image = this->metatileSelector->buildSecondaryMetatilesImage();
        image.save(filepath, "PNG");
    }
}

void TilesetEditor::on_actionImport_Primary_Metatiles_triggered()
{
    this->importTilesetMetatiles(this->primaryTileset, true);
}

void TilesetEditor::on_actionImport_Secondary_Metatiles_triggered()
{
    this->importTilesetMetatiles(this->secondaryTileset, false);
}

void TilesetEditor::importTilesetMetatiles(Tileset *tileset, bool primary)
{
    QString descriptor = primary ? "primary" : "secondary";
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Import %1 Tileset Metatiles from Advance Map 1.92").arg(descriptorCaps),
                this->project->importExportPath,
                "Advance Map 1.92 Metatile Files (*.bvd)");
    if (filepath.isEmpty()) {
        return;
    }
    this->project->setImportExportPath(filepath);
    bool error = false;
    QList<Metatile*> metatiles = MetatileParser::parse(filepath, &error, primary);
    if (error) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import metatiles from Advance Map 1.92 .bvd file.");
        QString message = QString("The .bvd file could not be processed. View porymap.log for specific errors.");
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // TODO: This is crude because it makes a history entry for every newly-imported metatile.
    //       Revisit this when tiles and num metatiles are added to tileset editory history.
    int metatileIdBase = primary ? 0 : Project::getNumMetatilesPrimary();
    for (int i = 0; i < metatiles.length(); i++) {
        if (i >= tileset->metatiles.length()) {
            break;
        }

        uint16_t metatileId = static_cast<uint16_t>(metatileIdBase + i);
        QString prevLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
        Metatile *prevMetatile = new Metatile(*tileset->metatiles.at(i));
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileId,
                                                              prevMetatile, new Metatile(*metatiles.at(i)),
                                                              prevLabel, prevLabel);
        metatileHistory.push(commit);
    }

    tileset->metatiles = metatiles;
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::on_actionShow_Unused_toggled(bool checked) {
    this->metatileSelector->selectorShowUnused = checked;

    if (checked) countMetatileUsage();

    this->metatileSelector->draw();
}

void TilesetEditor::on_actionShow_Counts_toggled(bool checked) {
    this->metatileSelector->selectorShowCounts = checked;

    if (checked) countMetatileUsage();

    this->metatileSelector->draw();
}

void TilesetEditor::on_actionShow_UnusedTiles_toggled(bool checked) {
    this->tileSelector->showUnused = checked;

    if (checked) countTileUsage();

    this->tileSelector->draw();
}

void TilesetEditor::on_actionMetatile_Grid_triggered(bool checked) {
    this->metatileSelector->showGrid = checked;
    this->metatileSelector->draw();
    porymapConfig.setShowTilesetEditorMetatileGrid(checked);
}

void TilesetEditor::on_actionLayer_Grid_triggered(bool checked) {
    this->metatileLayersItem->showGrid = checked;
    this->metatileLayersItem->draw();
    porymapConfig.setShowTilesetEditorLayerGrid(checked);
}

void TilesetEditor::countMetatileUsage() {
    // do not double count
    metatileSelector->usedMetatiles.fill(0);

    for (auto layout : this->project->mapLayouts.values()) {
        bool usesPrimary = false;
        bool usesSecondary = false;

        if (layout->tileset_primary_label == this->primaryTileset->name) {
            usesPrimary = true;
        }

        if (layout->tileset_secondary_label == this->secondaryTileset->name) {
            usesSecondary = true;
        }

        if (usesPrimary || usesSecondary) {
            this->project->loadLayout(layout);

            // for each block in the layout, mark in the vector that it is used
            for (int i = 0; i < layout->blockdata.length(); i++) {
                uint16_t metatileId = layout->blockdata.at(i).metatileId();
                if (metatileId < this->project->getNumMetatilesPrimary()) {
                    if (usesPrimary) metatileSelector->usedMetatiles[metatileId]++;
                } else {
                    if (usesSecondary) metatileSelector->usedMetatiles[metatileId]++;
                }
            }

            for (int i = 0; i < layout->border.length(); i++) {
                uint16_t metatileId = layout->border.at(i).metatileId();
                if (metatileId < this->project->getNumMetatilesPrimary()) {
                    if (usesPrimary) metatileSelector->usedMetatiles[metatileId]++;
                } else {
                    if (usesSecondary) metatileSelector->usedMetatiles[metatileId]++;
                }
            }
        }
    }
}

void TilesetEditor::countTileUsage() {
    // check primary tiles
    this->tileSelector->usedTiles.resize(Project::getNumTilesTotal());
    this->tileSelector->usedTiles.fill(0);

    QSet<Tileset*> primaryTilesets;
    QSet<Tileset*> secondaryTilesets;

    for (auto layout : this->project->mapLayouts.values()) {
        if (layout->tileset_primary_label == this->primaryTileset->name
         || layout->tileset_secondary_label == this->secondaryTileset->name) {
            this->project->loadLayoutTilesets(layout);
            // need to check metatiles
            if (layout->tileset_primary && layout->tileset_secondary) {
                primaryTilesets.insert(layout->tileset_primary);
                secondaryTilesets.insert(layout->tileset_secondary);
            }
        }
    }

    // check primary tilesets that are used with this secondary tileset for
    // reference to secondary tiles in primary metatiles
    for (Tileset *tileset : primaryTilesets) {
        for (Metatile *metatile : tileset->metatiles) {
            for (Tile tile : metatile->tiles) {
                if (tile.tileId >= Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // do the opposite for primary tiles in secondary metatiles
    for (Tileset *tileset : secondaryTilesets) {
        for (Metatile *metatile : tileset->metatiles) {
            for (Tile tile : metatile->tiles) {
                if (tile.tileId < Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // check this primary tileset metatiles
    for (Metatile *metatile : this->primaryTileset->metatiles) {
        for (Tile tile : metatile->tiles) {
            this->tileSelector->usedTiles[tile.tileId]++;
        }
    }

    // and the secondary metatiles
    for (Metatile *metatile : this->secondaryTileset->metatiles) {
        for (Tile tile : metatile->tiles) {
            this->tileSelector->usedTiles[tile.tileId]++;
        }
    }
}

void TilesetEditor::on_copyButton_metatileLabel_clicked() {
    uint16_t metatileId = this->getSelectedMetatileId();
    QString label = Tileset::getMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    if (label.isEmpty()) return;
    Tileset * tileset = Tileset::getMetatileLabelTileset(metatileId, this->primaryTileset, this->secondaryTileset);
    if (tileset)
        label.prepend(tileset->getMetatileLabelPrefix());
    QGuiApplication::clipboard()->setText(label);
    QToolTip::showText(this->ui->copyButton_metatileLabel->mapToGlobal(QPoint(0, 0)), "Copied!");
}
