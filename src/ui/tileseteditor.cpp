#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "log.h"
#include "imageproviders.h"
#include "advancemapparser.h"
#include "paletteutil.h"
#include "imageexport.h"
#include "config.h"
#include "shortcut.h"
#include "filedialog.h"
#include "validator.h"
#include "eventfilters.h"
#include "utility.h"
#include "message.h"
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QImageReader>

TilesetEditor::TilesetEditor(Project *project, Layout *layout, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor),
    project(project),
    layout(layout),
    hasUnsavedChanges(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    setTilesets(this->layout->tileset_primary_label, this->layout->tileset_secondary_label);

    connect(ui->checkBox_xFlip, &QCheckBox::toggled, this, &TilesetEditor::setXFlip);
    connect(ui->checkBox_yFlip, &QCheckBox::toggled, this, &TilesetEditor::setYFlip);

    this->tileXFlip = ui->checkBox_xFlip->isChecked();
    this->tileYFlip = ui->checkBox_yFlip->isChecked();
    this->paletteId = ui->spinBox_paletteSelector->value();

    connect(ui->actionSave_Tileset, &QAction::triggered, this, &TilesetEditor::save);

    connect(ui->actionImport_Primary_Tiles_Image,   &QAction::triggered, [this] { importTilesetTiles(this->primaryTileset); });
    connect(ui->actionImport_Secondary_Tiles_Image, &QAction::triggered, [this] { importTilesetTiles(this->secondaryTileset); });

    connect(ui->actionImport_Primary_AdvanceMap_Metatiles,   &QAction::triggered, [this] { importAdvanceMapMetatiles(this->primaryTileset); });
    connect(ui->actionImport_Secondary_AdvanceMap_Metatiles, &QAction::triggered, [this] { importAdvanceMapMetatiles(this->secondaryTileset); });

    connect(ui->actionExport_Primary_Tiles_Image,   &QAction::triggered, [this] { exportTilesImage(this->primaryTileset); });
    connect(ui->actionExport_Secondary_Tiles_Image, &QAction::triggered, [this] { exportTilesImage(this->secondaryTileset); });

    connect(ui->actionExport_Primary_Porytiles_Layer_Images,   &QAction::triggered, [this] { exportPorytilesLayerImages(this->primaryTileset); });
    connect(ui->actionExport_Secondary_Porytiles_Layer_Images, &QAction::triggered, [this] { exportPorytilesLayerImages(this->secondaryTileset); });

    connect(ui->actionExport_Metatiles_Image, &QAction::triggered, [this] { exportMetatilesImage(); });

    ui->actionShow_Tileset_Divider->setChecked(porymapConfig.showTilesetEditorDivider);
    ui->actionShow_Raw_Metatile_Attributes->setChecked(porymapConfig.showTilesetEditorRawAttributes);

    ui->spinBox_paletteSelector->setMinimum(0);
    ui->spinBox_paletteSelector->setMaximum(Project::getNumPalettesTotal() - 1);

    auto validator = new IdentifierValidator(this);
    validator->setAllowEmpty(true);
    ui->lineEdit_metatileLabel->setValidator(validator);

    ActiveWindowFilter *filter = new ActiveWindowFilter(this);
    connect(filter, &ActiveWindowFilter::activated, this, &TilesetEditor::onWindowActivated);
    this->installEventFilter(filter);

    initAttributesUi();
    initMetatileSelector();
    initMetatileLayersItem();
    initTileSelector();
    initSelectedTileItem();
    initShortcuts();
    this->metatileSelector->select(0);
    restoreWindowState();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
    delete metatileSelector;
    delete tileSelector;
    delete metatileLayersItem;
    delete paletteEditor;
    delete primaryTileset;
    delete secondaryTileset;
    delete metatilesScene;
    delete tilesScene;
    delete selectedTilePixmapItem;
    delete selectedTileScene;
    delete metatileLayersScene;
    delete copiedMetatile;
    delete metatileImageExportSettings;
    this->metatileHistory.clear();
}

void TilesetEditor::update(Layout *layout, QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    this->updateLayout(layout);
    this->updateTilesets(primaryTilesetLabel, secondaryTilesetLabel);
}

void TilesetEditor::updateLayout(Layout *layout) {
    this->layout = layout;
    this->metatileSelector->layout = layout;
}

void TilesetEditor::updateTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    if (this->hasUnsavedChanges) {
        auto result = SaveChangesMessage::show(QStringLiteral("Tileset"), false, this);
        if (result == QMessageBox::Yes) {
            this->save();
        }
    }
    this->setTilesets(primaryTilesetLabel, secondaryTilesetLabel);
    this->refresh();
}

bool TilesetEditor::selectMetatile(uint16_t metatileId) {
    if (!Tileset::metatileIsValid(metatileId, this->primaryTileset, this->secondaryTileset) || this->lockSelection)
        return false;
    this->metatileSelector->select(metatileId);
    this->redrawMetatileSelector();
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

void TilesetEditor::initAttributesUi() {
    connect(ui->comboBox_metatileBehaviors, &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitMetatileBehavior);
    connect(ui->comboBox_encounterType,     &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitEncounterType);
    connect(ui->comboBox_terrainType,       &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitTerrainType);
    connect(ui->comboBox_layerType,         &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitLayerType);

    // Behavior
    if (projectConfig.metatileBehaviorMask) {
        for (auto i = project->metatileBehaviorMapInverse.constBegin(); i != project->metatileBehaviorMapInverse.constEnd(); i++) {
            this->ui->comboBox_metatileBehaviors->addItem(i.value(), i.key());
        }
        this->ui->comboBox_metatileBehaviors->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_metatileBehaviors->setVisible(false);
        this->ui->label_metatileBehavior->setVisible(false);
    }

    // Terrain Type
    if (projectConfig.metatileTerrainTypeMask) {
        for (auto i = project->terrainTypeToName.constBegin(); i != project->terrainTypeToName.constEnd(); i++) {
            this->ui->comboBox_terrainType->addItem(i.value(), i.key());
        }
        this->ui->comboBox_terrainType->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_terrainType->setVisible(false);
        this->ui->label_terrainType->setVisible(false);
    }

    // Encounter Type
    if (projectConfig.metatileEncounterTypeMask) {
        for (auto i = project->encounterTypeToName.constBegin(); i != project->encounterTypeToName.constEnd(); i++) {
            this->ui->comboBox_encounterType->addItem(i.value(), i.key());
        }
        this->ui->comboBox_encounterType->setMinimumContentsLength(0);
    } else {
        this->ui->comboBox_encounterType->setVisible(false);
        this->ui->label_encounterType->setVisible(false);
    }

    // Layer Type
    if (!projectConfig.tripleLayerMetatilesEnabled) {
        this->ui->comboBox_layerType->addItem("Normal - Middle/Top",     Metatile::LayerType::Normal);
        this->ui->comboBox_layerType->addItem("Covered - Bottom/Middle", Metatile::LayerType::Covered);
        this->ui->comboBox_layerType->addItem("Split - Bottom/Top",      Metatile::LayerType::Split);
        this->ui->comboBox_layerType->setEditable(false);
        this->ui->comboBox_layerType->setMinimumContentsLength(0);
        if (!projectConfig.metatileLayerTypeMask) {
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

    // Raw attributes value
    ui->spinBox_rawAttributesValue->setMaximum(Metatile::getMaxAttributesMask());
    setRawAttributesVisible(ui->actionShow_Raw_Metatile_Attributes->isChecked());
    connect(ui->spinBox_rawAttributesValue, &UIntHexSpinBox::editingFinished, this, &TilesetEditor::onRawAttributesEdited);
    connect(ui->actionShow_Raw_Metatile_Attributes, &QAction::toggled, this, &TilesetEditor::setRawAttributesVisible);

    this->ui->frame_Properties->adjustSize();
}

void TilesetEditor::setRawAttributesVisible(bool visible) {
    porymapConfig.showTilesetEditorRawAttributes = visible;
    ui->label_rawAttributesValue->setVisible(visible);
    ui->spinBox_rawAttributesValue->setVisible(visible);
}

void TilesetEditor::initMetatileSelector()
{
    this->metatileSelector = new TilesetEditorMetatileSelector(projectConfig.metatileSelectorWidth, this->primaryTileset, this->secondaryTileset, this->layout);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileChanged,
            this, &TilesetEditor::onHoveredMetatileChanged);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileCleared,
            this, &TilesetEditor::onHoveredMetatileCleared);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::selectedMetatileChanged,
            this, &TilesetEditor::onSelectedMetatileChanged);

    bool showGrid = porymapConfig.showTilesetEditorMetatileGrid;
    this->ui->actionMetatile_Grid->setChecked(showGrid);
    this->metatileSelector->showGrid = showGrid;
    this->metatileSelector->showDivider = this->ui->actionShow_Tileset_Divider->isChecked();

    this->metatilesScene = new QGraphicsScene;
    this->metatilesScene->addItem(this->metatileSelector);
    this->metatileSelector->draw();

    this->ui->graphicsView_Metatiles->setScene(this->metatilesScene);
    this->ui->graphicsView_Metatiles->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    this->ui->horizontalSlider_MetatilesZoom->setValue(porymapConfig.tilesetEditorMetatilesZoom);
}

void TilesetEditor::initMetatileLayersItem() {
    Metatile *metatile = Tileset::getMetatile(this->getSelectedMetatileId(), this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, &MetatileLayersItem::tileChanged,
            this, &TilesetEditor::onMetatileLayerTileChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::selectedTilesChanged,
            this, &TilesetEditor::onMetatileLayerSelectionChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileChanged, [this](const Tile &tile) {
        onHoveredTileChanged(tile);
    });
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileCleared,
            this, &TilesetEditor::onHoveredTileCleared);

    bool showGrid = porymapConfig.showTilesetEditorLayerGrid;
    this->ui->actionLayer_Grid->setChecked(showGrid);
    this->metatileLayersItem->showGrid = showGrid;

    this->metatileLayersScene = new QGraphicsScene;
    this->metatileLayersScene->addItem(this->metatileLayersItem);
    this->ui->graphicsView_metatileLayers->setScene(this->metatileLayersScene);
}

void TilesetEditor::initTileSelector()
{
    this->tileSelector = new TilesetEditorTileSelector(this->primaryTileset, this->secondaryTileset, projectConfig.getNumLayersInMetatile());
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileChanged, [this](uint16_t tileId) {
        onHoveredTileChanged(tileId);
    });
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileCleared,
            this, &TilesetEditor::onHoveredTileCleared);
    connect(this->tileSelector, &TilesetEditorTileSelector::selectedTilesChanged,
            this, &TilesetEditor::onSelectedTilesChanged);

    this->tileSelector->showDivider = this->ui->actionShow_Tileset_Divider->isChecked();

    this->tilesScene = new QGraphicsScene;
    this->tilesScene->addItem(this->tileSelector);
    this->tileSelector->select(0);
    this->tileSelector->draw();

    this->ui->graphicsView_Tiles->setScene(this->tilesScene);
    this->ui->graphicsView_Tiles->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    this->ui->horizontalSlider_TilesZoom->setValue(porymapConfig.tilesetEditorTilesZoom);
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
    this->ui->splitter->restoreState(geometry.value("tileset_editor_splitter_state"));
}

void TilesetEditor::onWindowActivated() {
    // User may have made layout edits since window was last focused, so update counts
    if (this->metatileSelector) {
        if (this->metatileSelector->selectorShowUnused || this->metatileSelector->selectorShowCounts) {
            countMetatileUsage();
            this->metatileSelector->draw();
        }
    }
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

    this->redrawTileSelector();
    this->redrawMetatileSelector();
    this->drawSelectedTiles();
}

void TilesetEditor::drawSelectedTiles() {
    if (!this->selectedTileScene) {
        return;
    }

    const int imgTileWidth = 16;
    const int imgTileHeight = 16;
    this->selectedTileScene->clear();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QImage selectionImage(imgTileWidth * dimensions.x(), imgTileHeight * dimensions.y(), QImage::Format_RGBA8888);
    QPainter painter(&selectionImage);
    int tileIndex = 0;
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            auto tile = tiles.at(tileIndex);
            QImage tileImage = getPalettedTileImage(tile.tileId, this->primaryTileset, this->secondaryTileset, tile.palette, true).scaled(imgTileWidth, imgTileHeight);
            tile.flip(&tileImage);
            tileIndex++;
            painter.drawImage(i * imgTileWidth, j * imgTileHeight, tileImage);
        }
    }

    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(selectionImage));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);

    QSize size(this->selectedTilePixmapItem->pixmap().width(), this->selectedTilePixmapItem->pixmap().height());
    this->ui->graphicsView_selectedTile->setSceneRect(0, 0, size.width(), size.height());
    this->ui->graphicsView_selectedTile->setFixedSize(size.width() + 2, size.height() + 2);
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
        Metatile *updatedMetatile = Tileset::getMetatile(metatileId, this->layout->tileset_primary, this->layout->tileset_secondary);
        if (updatedMetatile) *this->metatile = *updatedMetatile;
    }

    this->metatileLayersItem->setMetatile(metatile);
    this->metatileLayersItem->draw();
    this->ui->graphicsView_metatileLayers->setFixedSize(this->metatileLayersItem->pixmap().width() + 2, this->metatileLayersItem->pixmap().height() + 2);

    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, this->primaryTileset, this->secondaryTileset);
    this->ui->lineEdit_metatileLabel->setText(labels.owned);
    this->ui->lineEdit_metatileLabel->setPlaceholderText(labels.shared);

    refreshMetatileAttributes();
}

void TilesetEditor::queueMetatileReload(uint16_t metatileId) {
    this->metatileReloadQueue.insert(metatileId);
}

void TilesetEditor::onHoveredTileChanged(const Tile &tile) {
    this->ui->statusbar->showMessage(QString("Tile: %1, Palette: %2%3%4")
                                        .arg(Util::toHexString(tile.tileId, 3))
                                        .arg(QString::number(tile.palette))
                                        .arg(tile.xflip ? ", X-flipped" : "")
                                        .arg(tile.yflip ? ", Y-flipped" : "")
                                    );
}

void TilesetEditor::onHoveredTileChanged(uint16_t tileId) {
    this->ui->statusbar->showMessage(QString("Tile: %1").arg(Util::toHexString(tileId, 3)));
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
                if (this->tileSelector->showUnused) {
                    this->tileSelector->usedTiles[tile.tileId] += 1;
                    this->tileSelector->usedTiles[prevMetatile->tiles[tileIndex].tileId] -= 1;
                }
            }
            selectedTileIndex++;
        }
    }

    this->metatileSelector->drawSelectedMetatile();
    this->metatileLayersItem->draw();
    this->tileSelector->draw();
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

    this->tileSelector->setExternalSelection(width, height, tiles, tileIdxs);
    if (width == 1 && height == 1) {
        ui->spinBox_paletteSelector->setValue(tiles[0].palette);
        this->tileSelector->highlight(static_cast<uint16_t>(tiles[0].tileId));
        this->redrawTileSelector();
    }
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

void TilesetEditor::setXFlip(bool enabled)
{
    this->tileXFlip = enabled;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
}

void TilesetEditor::setYFlip(bool enabled)
{
    this->tileYFlip = enabled;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
    this->metatileLayersItem->clearLastModifiedCoords();
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

uint32_t TilesetEditor::attributeNameToValue(Metatile::Attr attribute, const QString &text, bool *ok) {
    if (ok) *ok = true;
    if (attribute == Metatile::Attr::Behavior) {
        auto it = project->metatileBehaviorMap.constFind(text);
        if (it != project->metatileBehaviorMap.constEnd())
            return it.value();
    } else if (attribute == Metatile::Attr::EncounterType) {
        for (auto i = project->encounterTypeToName.constBegin(); i != project->encounterTypeToName.constEnd(); i++) {
            if (i.value() == text) return i.key();
        }
    } else if (attribute == Metatile::Attr::TerrainType) {
        for (auto i = project->terrainTypeToName.constBegin(); i != project->terrainTypeToName.constEnd(); i++) {
            if (i.value() == text) return i.key();
        }
    } else if (attribute == Metatile::Attr::LayerType) {
        // The layer type text is not editable, it uses special display names. Just get the index of the display name.
        int i = ui->comboBox_layerType->findText(text);
        if (i >= 0) return i;
    }
    return text.toUInt(ok, 0);
}

void TilesetEditor::commitAttributeFromComboBox(Metatile::Attr attribute, NoScrollComboBox *combo) {
    if (!this->metatile)
        return;

    bool ok;
    uint32_t newValue = this->attributeNameToValue(attribute, combo->currentText(), &ok);
    if (ok && newValue != this->metatile->getAttribute(attribute)) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setAttribute(attribute, newValue);
        this->commitMetatileChange(prevMetatile);

        // When an attribute changes we also need to update the raw value display.
        const QSignalBlocker b_RawAttributesValue(ui->spinBox_rawAttributesValue);
        ui->spinBox_rawAttributesValue->setValue(this->metatile->getAttributes());
    }

    // Update the text in the combo box to reflect the final value.
    // The text may change if the input text was invalid, the value was too large to fit, or if a number was entered that we know an identifier for.
    const QSignalBlocker b(combo);
    combo->setHexItem(this->metatile->getAttribute(attribute));
}

void TilesetEditor::onRawAttributesEdited() {
    uint32_t newAttributes = ui->spinBox_rawAttributesValue->value();
     if (newAttributes != this->metatile->getAttributes()) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setAttributes(newAttributes);
        this->commitMetatileChange(prevMetatile);
    }
    refreshMetatileAttributes();
}

void TilesetEditor::refreshMetatileAttributes() {
    if (!this->metatile) return;

    const QSignalBlocker b_MetatileBehaviors(ui->comboBox_metatileBehaviors);
    const QSignalBlocker b_EncounterType(ui->comboBox_encounterType);
    const QSignalBlocker b_TerrainType(ui->comboBox_terrainType);
    const QSignalBlocker b_LayerType(ui->comboBox_layerType);
    const QSignalBlocker b_RawAttributesValue(ui->spinBox_rawAttributesValue);
    ui->comboBox_metatileBehaviors->setHexItem(this->metatile->behavior());
    ui->comboBox_encounterType->setHexItem(this->metatile->encounterType());
    ui->comboBox_terrainType->setHexItem(this->metatile->terrainType());
    ui->comboBox_layerType->setHexItem(this->metatile->layerType());
    ui->spinBox_rawAttributesValue->setValue(this->metatile->getAttributes());

    this->metatileSelector->drawSelectedMetatile();
}

void TilesetEditor::commitMetatileBehavior() {
    commitAttributeFromComboBox(Metatile::Attr::Behavior, ui->comboBox_metatileBehaviors);
}

void TilesetEditor::commitEncounterType() {
    commitAttributeFromComboBox(Metatile::Attr::EncounterType, ui->comboBox_encounterType);
}

void TilesetEditor::commitTerrainType() {
    commitAttributeFromComboBox(Metatile::Attr::TerrainType, ui->comboBox_terrainType);
};

void TilesetEditor::commitLayerType() {
    commitAttributeFromComboBox(Metatile::Attr::LayerType, ui->comboBox_layerType);
    this->metatileSelector->drawSelectedMetatile(); // Changing the layer type can affect how fully transparent metatiles appear
}

bool TilesetEditor::save() {
    // Need this temporary flag to stop selection resetting after saving.
    // This is a workaround; redrawing the map's metatile selector shouldn't emit the same signal as when it's selected.
    this->lockSelection = true;

    bool success = this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    emit this->tilesetsSaved(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor) {
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    }
    this->ui->statusbar->showMessage(success ? QStringLiteral("Saved primary and secondary Tilesets!")
                                             : QStringLiteral("Failed to save tilesets! See log for details."), 5000);
    if (success) {
        this->hasUnsavedChanges = false;
    }
    this->lockSelection = false;
    return success;
}

void TilesetEditor::importTilesetTiles(Tileset *tileset) {
    bool primary = !tileset->is_secondary;
    QString descriptor = primary ? "primary" : "secondary";
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = FileDialog::getOpenFileName(this, QString("Import %1 Tileset Tiles Image").arg(descriptorCaps), "", "Image Files (*.png *.bmp *.jpg *.dib)");
    if (filepath.isEmpty()) {
        return;
    }

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
    if (image.width() == 0 || image.height() == 0 || image.width() % Tile::pixelWidth() != 0 || image.height() % Tile::pixelHeight() != 0) {
        ErrorMessage::show(QStringLiteral("Failed to import tiles."),
                           QString("The image dimensions (%1x%2) are invalid. The dimensions must be a multiple of %3x%4 pixels.")
                                  .arg(image.width())
                                  .arg(image.height())
                                  .arg(Tile::pixelWidth())
                                  .arg(Tile::pixelHeight()),
                            this);
        return;
    }

    // Validate total number of tiles in image.
    int numTilesWide = image.width() / Tile::pixelWidth();
    int numTilesHigh = image.height() / Tile::pixelHeight();
    int totalTiles = numTilesHigh * numTilesWide;
    int maxAllowedTiles = primary ? Project::getNumTilesPrimary() : Project::getNumTilesTotal() - Project::getNumTilesPrimary();
    if (totalTiles > maxAllowedTiles) {
        ErrorMessage::show(QStringLiteral("Failed to import tiles."),
                           QString("The maximum number of tiles allowed in the %1 tileset is %2, but the provided image contains %3 total tiles.")
                                  .arg(descriptor)
                                  .arg(maxAllowedTiles)
                                  .arg(totalTiles),
                           this);
        return;
    }

    // Ask user to provide a palette for the un-indexed image.
    if (image.colorCount() == 0) {
        auto msgBox = new QuestionMessage(QStringLiteral("Select a palette file for this image?"), this);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->setInformativeText(QStringLiteral("The provided image is not indexed. "
                                                  "An indexed image will be generated using the provided image and palette."));
        if (msgBox->exec() != QMessageBox::Yes)
            return;

        QString filepath = FileDialog::getOpenFileName(this, "Select Palette for Tiles Image", "", "Palette Files (*.pal *.act *tpl *gpl)");
        if (filepath.isEmpty()) {
            return;
        }

        bool error = false;
        QList<QRgb> palette = PaletteUtil::parse(filepath, &error);
        if (error) {
            RecentErrorMessage::show(QStringLiteral("Failed to import palette."), this);
            return;
        }

        QVector<QRgb> colorTable = palette.toVector();
        image = image.convertToFormat(QImage::Format::Format_Indexed8, colorTable);
    }

    if (!tileset->loadTilesImage(&image)) {
        RecentErrorMessage::show(QStringLiteral("Failed to import tiles."), this);
        return;
    }
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::closeEvent(QCloseEvent *event)
{
    if (this->hasUnsavedChanges) {
        auto result = SaveChangesMessage::show(QStringLiteral("Tileset"), this);
        if (result == QMessageBox::Yes) {
            if (this->save()) {
                event->accept();
            } else {
                event->ignore();
            }
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
            this->saveState(),
            this->ui->splitter->saveState()
        );
    }
}

void TilesetEditor::on_actionChange_Metatiles_Count_triggered()
{
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Change Number of Metatiles");
    dialog.setWindowModality(Qt::WindowModal);

    QFormLayout form(&dialog);

    QSpinBox *primarySpinBox = new QSpinBox();
    QSpinBox *secondarySpinBox = new QSpinBox();
    primarySpinBox->setMinimum(1);
    secondarySpinBox->setMinimum(1);
    primarySpinBox->setMaximum(Project::getNumMetatilesPrimary());
    secondarySpinBox->setMaximum(Project::getNumMetatilesSecondary());
    primarySpinBox->setValue(this->primaryTileset->numMetatiles());
    secondarySpinBox->setValue(this->secondaryTileset->numMetatiles());
    form.addRow(new QLabel("Primary Tileset"), primarySpinBox);
    form.addRow(new QLabel("Secondary Tileset"), secondarySpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        this->primaryTileset->resizeMetatiles(primarySpinBox->value());
        this->secondaryTileset->resizeMetatiles(secondarySpinBox->value());
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

    // Update tile usage if any tiles changed
    if (this->tileSelector && this->tileSelector->showUnused) {
        int numTiles = projectConfig.getNumTilesInMetatile();
        for (int i = 0; i < numTiles; i++) {
            if (src->tiles[i].tileId != dest->tiles[i].tileId) {
                this->tileSelector->usedTiles[src->tiles[i].tileId] += 1;
                this->tileSelector->usedTiles[dest->tiles[i].tileId] -= 1;
            }
        }
        this->tileSelector->draw();
    }

    this->metatile = dest;
    *this->metatile = *src;
    this->metatileSelector->select(metatileId);
    this->metatileSelector->drawMetatile(metatileId);
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

void TilesetEditor::exportTilesImage(Tileset *tileset) {
    bool primary = !tileset->is_secondary;
    QString defaultFilepath = QString("%1/%2_Tiles_Pal%3.png").arg(FileDialog::getDirectory()).arg(tileset->name).arg(this->paletteId);
    QString filepath = FileDialog::getSaveFileName(this, QString("Export %1 Tiles Image").arg(primary ? "Primary" : "Secondary"), defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        QImage image = primary ? this->tileSelector->buildPrimaryTilesIndexedImage() : this->tileSelector->buildSecondaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

// There are many more options for exporting metatile images than tile images, so we open a separate dialog to ask the user for settings.
void TilesetEditor::exportMetatilesImage() {
    if (!this->metatileImageExportSettings) {
        this->metatileImageExportSettings = new MetatileImageExporter::Settings;
    }
    auto dialog = new MetatileImageExporter(this, this->primaryTileset, this->secondaryTileset, this->metatileImageExportSettings);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void TilesetEditor::exportPorytilesLayerImages(Tileset *tileset) {
    QString dir = FileDialog::getExistingDirectory(this, QStringLiteral("Choose Folder to Export Images"));
    if (dir.isEmpty()) {
        return;
    }

    MetatileImageExporter layerExporter(this, this->primaryTileset, this->secondaryTileset);
    MetatileImageExporter::Settings settings = {};
    settings.usePrimaryTileset = !tileset->is_secondary;
    settings.useSecondaryTileset = tileset->is_secondary;

    QMap<QString,QImage> images;
    QStringList pathCollisions;
    for (int i = 0; i < 3; i++) {
        settings.layerOrder.clear();
        settings.layerOrder[i] = true;
        layerExporter.applySettings(settings);

        QString filename = layerExporter.getDefaultFileName();
        QString path = QString("%1/%2").arg(dir).arg(filename);
        if (QFileInfo::exists(path)) {
            pathCollisions.append(filename);
        }
        images[path] = layerExporter.getImage();
    }

    if (!pathCollisions.isEmpty()) {
        QString message = QString("The following files will be overwritten, are you sure you want to export?\n\n%1").arg(pathCollisions.join("\n"));
        auto reply = QuestionMessage::show(message, this);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    for (auto it = images.constBegin(); it != images.constEnd(); it++) {
        QString path = it.key();
        if (!it.value().save(path)) {
            logError(QString("Failed to save Porytiles layer image '%1'.").arg(path));
        }
    }
}

void TilesetEditor::importAdvanceMapMetatiles(Tileset *tileset) {
    bool primary = !tileset->is_secondary;
    QString descriptorCaps = primary ? "Primary" : "Secondary";

    QString filepath = FileDialog::getOpenFileName(this, QString("Import %1 Tileset Metatiles from Advance Map 1.92").arg(descriptorCaps), "", "Advance Map 1.92 Metatile Files (*.bvd)");
    if (filepath.isEmpty()) {
        return;
    }

    bool error = false;
    QList<Metatile*> metatiles = AdvanceMapParser::parseMetatiles(filepath, &error, primary);
    if (error) {
        RecentErrorMessage::show(QStringLiteral("Failed to import metatiles from Advance Map 1.92 .bvd file."), this);
        qDeleteAll(metatiles);
        return;
    }

    // TODO: This is crude because it makes a history entry for every newly-imported metatile.
    //       Revisit this when tiles and num metatiles are added to tileset editory history.
    uint16_t metatileIdBase = tileset->firstMetatileId();
    for (int i = 0; i < metatiles.length(); i++) {
        if (i >= tileset->numMetatiles()) {
            break;
        }

        uint16_t metatileId = static_cast<uint16_t>(metatileIdBase + i);
        QString prevLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
        Metatile *prevMetatile = new Metatile(*tileset->metatileAt(i));
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileId,
                                                              prevMetatile, new Metatile(*metatiles.at(i)),
                                                              prevLabel, prevLabel);
        metatileHistory.push(commit);
    }

    tileset->setMetatiles(metatiles);
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
    porymapConfig.showTilesetEditorMetatileGrid = checked;
}

void TilesetEditor::on_actionLayer_Grid_triggered(bool checked) {
    this->metatileLayersItem->showGrid = checked;
    this->metatileLayersItem->draw();
    porymapConfig.showTilesetEditorLayerGrid = checked;
}

void TilesetEditor::on_actionShow_Tileset_Divider_triggered(bool checked) {
    this->metatileSelector->showDivider = checked;
    this->metatileSelector->draw();

    this->tileSelector->showDivider = checked;
    this->tileSelector->draw();

    porymapConfig.showTilesetEditorDivider = checked;
}

void TilesetEditor::countMetatileUsage() {
    // do not double count
    this->metatileSelector->usedMetatiles.fill(0);

    for (const auto &layoutId : this->project->layoutIds()) {
        Layout *layout = this->project->getLayout(layoutId);
        bool usesPrimary = (layout->tileset_primary_label == this->primaryTileset->name);
        bool usesSecondary = (layout->tileset_secondary_label == this->secondaryTileset->name);

        if (usesPrimary || usesSecondary) {
            if (!this->project->loadLayout(layoutId))
                continue;

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

    for (const auto &layoutId : this->project->layoutIds()) {
        Layout *layout = this->project->getLayout(layoutId);
        if (layout->tileset_primary_label == this->primaryTileset->name
         || layout->tileset_secondary_label == this->secondaryTileset->name) {
            // need to check metatiles
            this->project->loadLayoutTilesets(layout);
            if (layout->tileset_primary && layout->tileset_secondary) {
                primaryTilesets.insert(layout->tileset_primary);
                secondaryTilesets.insert(layout->tileset_secondary);
            }
        }
    }

    // check primary tilesets that are used with this secondary tileset for
    // reference to secondary tiles in primary metatiles
    for (const auto &tileset : primaryTilesets) {
        for (const auto &metatile : tileset->metatiles()) {
            for (const auto &tile : metatile->tiles) {
                if (tile.tileId >= Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // do the opposite for primary tiles in secondary metatiles
    for (Tileset *tileset : secondaryTilesets) {
        for (const auto &metatile : tileset->metatiles()) {
            for (const auto &tile : metatile->tiles) {
                if (tile.tileId < Project::getNumTilesPrimary())
                    this->tileSelector->usedTiles[tile.tileId]++;
            }
        }
    }

    // check this primary tileset metatiles
    for (const auto &metatile : this->primaryTileset->metatiles()) {
        for (const auto &tile : metatile->tiles) {
            this->tileSelector->usedTiles[tile.tileId]++;
        }
    }

    // and the secondary metatiles
    for (const auto &metatile : this->secondaryTileset->metatiles()) {
        for (const auto &tile : metatile->tiles) {
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

void TilesetEditor::on_horizontalSlider_MetatilesZoom_valueChanged(int value) {
    porymapConfig.tilesetEditorMetatilesZoom = value;
    this->redrawMetatileSelector();
}

void TilesetEditor::redrawMetatileSelector() {
    QSize size(this->metatileSelector->pixmap().width(), this->metatileSelector->pixmap().height());
    this->ui->graphicsView_Metatiles->setSceneRect(0, 0, size.width(), size.height());

    double scale = pow(3.0, static_cast<double>(porymapConfig.tilesetEditorMetatilesZoom - 30) / 30.0);
    QTransform transform;
    transform.scale(scale, scale);
    size *= scale;

    this->ui->graphicsView_Metatiles->setTransform(transform);
    this->ui->graphicsView_Metatiles->setFixedSize(size.width() + 2, size.height() + 2);

    QPoint pos = this->metatileSelector->getMetatileIdCoordsOnWidget(this->getSelectedMetatileId());
    pos *= scale;

    this->ui->scrollAreaWidgetContents_Metatiles->adjustSize();
    auto viewport = this->ui->scrollArea_Metatiles->viewport();
    this->ui->scrollArea_Metatiles->ensureVisible(pos.x(), pos.y(), viewport->width() / 2, viewport->height() / 2);
}

void TilesetEditor::on_horizontalSlider_TilesZoom_valueChanged(int value) {
    porymapConfig.tilesetEditorTilesZoom = value;
    this->redrawTileSelector();
}

void TilesetEditor::redrawTileSelector() {
    QSize size(this->tileSelector->pixmap().width(), this->tileSelector->pixmap().height());
    this->ui->graphicsView_Tiles->setSceneRect(0, 0, size.width(), size.height());

    double scale = pow(3.0, static_cast<double>(porymapConfig.tilesetEditorTilesZoom - 30) / 30.0);
    QTransform transform;
    transform.scale(scale, scale);
    size *= scale;

    this->ui->graphicsView_Tiles->setTransform(transform);
    this->ui->graphicsView_Tiles->setFixedSize(size.width() + 2, size.height() + 2);

    this->ui->scrollAreaWidgetContents_Tiles->adjustSize();

    auto tiles = this->tileSelector->getSelectedTiles();
    if (!tiles.isEmpty()) {
        QPoint pos = this->tileSelector->getTileCoordsOnWidget(tiles[0].tileId);
        pos *= scale;
        auto viewport = this->ui->scrollArea_Tiles->viewport();
        this->ui->scrollArea_Tiles->ensureVisible(pos.x(), pos.y(), viewport->width() / 2, viewport->height() / 2);
    }
}
