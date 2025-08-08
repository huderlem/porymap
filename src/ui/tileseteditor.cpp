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

    ui->spinBox_paletteSelector->setRange(0, Project::getNumPalettesTotal() - 1);

    auto validator = new IdentifierValidator(this);
    validator->setAllowEmpty(true);
    ui->lineEdit_MetatileLabel->setValidator(validator);

    ui->actionShow_Tileset_Divider->setChecked(porymapConfig.showTilesetEditorDivider);
    ui->actionShow_Raw_Metatile_Attributes->setChecked(porymapConfig.showTilesetEditorRawAttributes);

    ActiveWindowFilter *filter = new ActiveWindowFilter(this);
    connect(filter, &ActiveWindowFilter::activated, this, &TilesetEditor::onWindowActivated);
    this->installEventFilter(filter);

    setTilesets(this->layout->tileset_primary_label, this->layout->tileset_secondary_label);

    connect(ui->checkBox_xFlip, &QCheckBox::toggled, this, &TilesetEditor::refreshTileFlips);
    connect(ui->checkBox_yFlip, &QCheckBox::toggled, this, &TilesetEditor::refreshTileFlips);

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

    connect(ui->spinBox_paletteSelector, QOverload<int>::of(&QSpinBox::valueChanged), this, &TilesetEditor::refreshPaletteId);

    connect(ui->actionLayer_Arrangement_Horizontal, &QAction::triggered, [this] { setMetatileLayerOrientation(Qt::Horizontal); });
    connect(ui->actionLayer_Arrangement_Vertical,   &QAction::triggered, [this] { setMetatileLayerOrientation(Qt::Vertical); });

    connect(ui->lineEdit_MetatileLabel, &QLineEdit::editingFinished, this, &TilesetEditor::commitMetatileLabel);

    initAttributesUi();
    initMetatileSelector();
    initMetatileLayersItem();
    initTileSelector();
    initSelectedTileItem();
    initShortcuts();
    setMetatileLayerOrientation(porymapConfig.tilesetEditorLayerOrientation);
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
    delete this->primaryTileset;
    delete this->secondaryTileset;
    this->primaryTileset = new Tileset(*primaryTileset);
    this->secondaryTileset = new Tileset(*secondaryTileset);
    if (this->paletteEditor) this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    initMetatileHistory();
}

void TilesetEditor::initAttributesUi() {
    connect(ui->comboBox_MetatileBehaviors, &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitMetatileBehavior);
    connect(ui->comboBox_EncounterType,     &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitEncounterType);
    connect(ui->comboBox_TerrainType,       &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitTerrainType);
    connect(ui->comboBox_LayerType,         &NoScrollComboBox::editingFinished, this, &TilesetEditor::commitLayerType);

    // Behavior
    if (projectConfig.metatileBehaviorMask) {
        for (auto i = project->metatileBehaviorMapInverse.constBegin(); i != project->metatileBehaviorMapInverse.constEnd(); i++) {
            this->ui->comboBox_MetatileBehaviors->addItem(i.value(), i.key());
        }
        this->ui->comboBox_MetatileBehaviors->setMinimumContentsLength(0);
    } else {
        this->ui->frame_MetatileBehavior->setVisible(false);
    }

    // Terrain Type
    if (projectConfig.metatileTerrainTypeMask) {
        for (auto i = project->terrainTypeToName.constBegin(); i != project->terrainTypeToName.constEnd(); i++) {
            this->ui->comboBox_TerrainType->addItem(i.value(), i.key());
        }
        this->ui->comboBox_TerrainType->setMinimumContentsLength(0);
    } else {
        this->ui->frame_TerrainType->setVisible(false);
    }

    // Encounter Type
    if (projectConfig.metatileEncounterTypeMask) {
        for (auto i = project->encounterTypeToName.constBegin(); i != project->encounterTypeToName.constEnd(); i++) {
            this->ui->comboBox_EncounterType->addItem(i.value(), i.key());
        }
        this->ui->comboBox_EncounterType->setMinimumContentsLength(0);
    } else {
        this->ui->frame_EncounterType->setVisible(false);
    }

    // Layer Type
    if (!projectConfig.tripleLayerMetatilesEnabled) {
        this->ui->comboBox_LayerType->addItem("Normal - Middle/Top",     Metatile::LayerType::Normal);
        this->ui->comboBox_LayerType->addItem("Covered - Bottom/Middle", Metatile::LayerType::Covered);
        this->ui->comboBox_LayerType->addItem("Split - Bottom/Top",      Metatile::LayerType::Split);
        this->ui->comboBox_LayerType->setEditable(false);
        this->ui->comboBox_LayerType->setMinimumContentsLength(0);
        if (!projectConfig.metatileLayerTypeMask) {
            // User doesn't have triple layer metatiles, but has no layer type attribute.
            // Porymap is still using the layer type value to render metatiles, and with
            // no mask set every metatile will be "Middle/Top", so just display the combo
            // box but prevent the user from changing the value.
            this->ui->comboBox_LayerType->setEnabled(false);
        }
    } else {
        this->ui->frame_LayerType->setVisible(false);
        this->ui->label_BottomTop->setText("Bottom/Middle/Top");
    }

    // Raw attributes value
    ui->spinBox_RawAttributesValue->setMaximum(Metatile::getMaxAttributesMask());
    setRawAttributesVisible(ui->actionShow_Raw_Metatile_Attributes->isChecked());
    connect(ui->spinBox_RawAttributesValue, &UIntHexSpinBox::editingFinished, this, &TilesetEditor::onRawAttributesEdited);
    connect(ui->actionShow_Raw_Metatile_Attributes, &QAction::toggled, this, &TilesetEditor::setRawAttributesVisible);
}

void TilesetEditor::setRawAttributesVisible(bool visible) {
    porymapConfig.showTilesetEditorRawAttributes = visible;
    ui->frame_RawAttributesValue->setVisible(visible);
    rebuildMetatilePropertiesFrame();
}

void TilesetEditor::initMetatileSelector()
{
    this->metatileSelector = new TilesetEditorMetatileSelector(projectConfig.metatileSelectorWidth, this->primaryTileset, this->secondaryTileset, this->layout);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileChanged,  this, &TilesetEditor::onHoveredMetatileChanged);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::hoveredMetatileCleared,  this, &TilesetEditor::onHoveredMetatileCleared);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::selectedMetatileChanged, this, &TilesetEditor::onSelectedMetatileChanged);
    connect(this->metatileSelector, &TilesetEditorMetatileSelector::swapRequested, this, &TilesetEditor::commitMetatileSwap);
    connect(ui->actionSwap_Metatiles, &QAction::toggled, this->metatileSelector, &TilesetEditorMetatileSelector::setSwapMode);

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

void TilesetEditor::setMetatileLayerOrientation(Qt::Orientation orientation) {
    // Sync settings
    bool horizontal = (orientation == Qt::Horizontal);
    porymapConfig.tilesetEditorLayerOrientation = orientation;
    const QSignalBlocker b_Horizontal(ui->actionLayer_Arrangement_Horizontal);
    const QSignalBlocker b_Vertical(ui->actionLayer_Arrangement_Vertical);
    ui->actionLayer_Arrangement_Horizontal->setChecked(horizontal);
    ui->actionLayer_Arrangement_Vertical->setChecked(!horizontal);

    this->metatileLayersItem->setOrientation(orientation);

    int numTilesWide = Metatile::tileWidth();
    int numTilesTall = Metatile::tileHeight();
    int numLayers = projectConfig.getNumLayersInMetatile();
    if (horizontal) {
        numTilesWide *= numLayers;
    } else {
        numTilesTall *= numLayers;
    }
    this->tileSelector->setMaxSelectionSize(numTilesWide, numTilesTall);

    const int scale = 2;
    int w = Tile::pixelWidth() * numTilesWide * scale + 2;
    int h = Tile::pixelHeight() * numTilesTall * scale + 2;
    ui->graphicsView_selectedTile->setFixedSize(w, h);
    ui->graphicsView_MetatileLayers->setFixedSize(w, h);

    drawSelectedTiles();

    // If the layers are laid out vertically then the orientation is obvious, no need to label them.
    // This also lets us give the vertical space of the label over to the layer view.
    ui->label_BottomTop->setVisible(horizontal);

    rebuildMetatilePropertiesFrame();
}

// We rearrange the metatile properties panel depending on the orientation and size of the metatile layer view.
// If triple layer metatiles are in-use then layer type field is hidden, so there's an awkward amount of space
// next to the layer view, especially in the vertical orientation.
// We shift 1-2 widgets up to fill this space next to the layer view. This gets a little complicated because which
// widgets are available to move changes depending on the user's settings.
void TilesetEditor::rebuildMetatilePropertiesFrame() {
    if (porymapConfig.tilesetEditorLayerOrientation == Qt::Horizontal) {
        this->numLayerViewRows = 1;
    } else {
        this->numLayerViewRows = projectConfig.tripleLayerMetatilesEnabled ? 4 : 2;
    }

    for (const auto &frame : ui->gridLayout_MetatileProperties->findChildren<QFrame*>()) {
        ui->gridLayout_MetatileProperties->removeWidget(frame);
    }
    ui->gridLayout_MetatileProperties->addWidget(ui->frame_Layers, 0, 0, this->numLayerViewRows, 1);

    int row = 0;
    addWidgetToMetatileProperties(ui->frame_LayerType, &row, 2);
    if (porymapConfig.tilesetEditorLayerOrientation == Qt::Horizontal) {
        // When the layer view's orientation is horizontal we only allow the
        // layer type selector to share the row with the layer view.
        row = this->numLayerViewRows;
    }
    addWidgetToMetatileProperties(ui->frame_MetatileBehavior,   &row, 2);
    addWidgetToMetatileProperties(ui->frame_EncounterType,      &row, 2);
    addWidgetToMetatileProperties(ui->frame_TerrainType,        &row, 2);
    addWidgetToMetatileProperties(ui->frame_RawAttributesValue, &row, 2);
    addWidgetToMetatileProperties(ui->frame_MetatileLabel,      &row, 2);
}

void TilesetEditor::addWidgetToMetatileProperties(QWidget *w, int *row, int rowSpan) {
    if (w->isVisibleTo(ui->frame_Properties)) {
        int col = (*row < this->numLayerViewRows) ? 1 : 0; // Shift widget over if it shares the row with the layer view
        ui->gridLayout_MetatileProperties->addWidget(w, *row, col, rowSpan, -1);
        *row += rowSpan;
    }
}

void TilesetEditor::initMetatileLayersItem() {
    Metatile *metatile = Tileset::getMetatile(this->getSelectedMetatileId(), this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, &MetatileLayersItem::tileChanged, [this](const QPoint &pos) { paintSelectedLayerTiles(pos); });
    connect(this->metatileLayersItem, &MetatileLayersItem::paletteChanged, [this](const QPoint &pos) { paintSelectedLayerTiles(pos, true); });
    connect(this->metatileLayersItem, &MetatileLayersItem::selectedTilesChanged, this, &TilesetEditor::onMetatileLayerSelectionChanged);
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileChanged, [this](const Tile &tile) { showTileStatus(tile); });
    connect(this->metatileLayersItem, &MetatileLayersItem::hoveredTileCleared, this, &TilesetEditor::onHoveredTileCleared);

    bool showGrid = porymapConfig.showTilesetEditorLayerGrid;
    this->ui->actionLayer_Grid->setChecked(showGrid);
    this->metatileLayersItem->showGrid = showGrid;

    this->metatileLayersScene = new QGraphicsScene;
    this->metatileLayersScene->addItem(this->metatileLayersItem);
    this->ui->graphicsView_MetatileLayers->setScene(this->metatileLayersScene);
}

void TilesetEditor::initTileSelector() {
    this->tileSelector = new TilesetEditorTileSelector(this->primaryTileset, this->secondaryTileset);
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileChanged, [this](uint16_t tileId) {
        showTileStatus(tileId);
    });
    connect(this->tileSelector, &TilesetEditorTileSelector::hoveredTileCleared, this, &TilesetEditor::onHoveredTileCleared);
    connect(this->tileSelector, &TilesetEditorTileSelector::selectedTilesChanged, this, &TilesetEditor::drawSelectedTiles);

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
    QSize dimensions = this->tileSelector->getSelectionDimensions();
    QImage selectionImage(imgTileWidth * dimensions.width(), imgTileHeight * dimensions.height(), QImage::Format_RGBA8888);
    QPainter painter(&selectionImage);
    int tileIndex = 0;
    for (int y = 0; y < dimensions.height(); y++) {
        for (int x = 0; x < dimensions.width(); x++) {
            auto tile = tiles.value(tileIndex++);
            QImage tileImage = getPalettedTileImage(tile.tileId, this->primaryTileset, this->secondaryTileset, tile.palette, true).scaled(imgTileWidth, imgTileHeight);
            tile.flip(&tileImage);
            painter.drawImage(x * imgTileWidth, y * imgTileHeight, tileImage);
        }
    }

    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(selectionImage));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);

    QSize size(this->selectedTilePixmapItem->pixmap().width(), this->selectedTilePixmapItem->pixmap().height());
    this->ui->graphicsView_selectedTile->setSceneRect(0, 0, size.width(), size.height());
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
    if (!this->metatile) return;

    // The scripting API allows users to change metatiles in the project, and these changes are saved to disk.
    // The Tileset Editor (if open) needs to reflect these changes when the metatile is next displayed.
    if (this->metatileReloadQueue.contains(metatileId)) {
        this->metatileReloadQueue.remove(metatileId);
        Metatile *updatedMetatile = Tileset::getMetatile(metatileId, this->layout->tileset_primary, this->layout->tileset_secondary);
        if (updatedMetatile) *this->metatile = *updatedMetatile;
    }

    this->metatileLayersItem->setMetatile(metatile);

    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, this->primaryTileset, this->secondaryTileset);
    this->ui->lineEdit_MetatileLabel->setText(labels.owned);
    this->ui->lineEdit_MetatileLabel->setPlaceholderText(labels.shared);

    refreshMetatileAttributes();
}

void TilesetEditor::queueMetatileReload(uint16_t metatileId) {
    this->metatileReloadQueue.insert(metatileId);
}

void TilesetEditor::updateLayerTileStatus() {
    if (this->metatileLayersItem->hasCursor()) {
        showTileStatus(this->metatileLayersItem->tileUnderCursor());
    }
}

void TilesetEditor::showTileStatus(const Tile &tile) {
    this->ui->statusbar->showMessage(QString("Tile: %1, Palette: %2%3%4")
                                        .arg(Util::toHexString(tile.tileId, 3))
                                        .arg(QString::number(tile.palette))
                                        .arg(tile.xflip ? ", X-flipped" : "")
                                        .arg(tile.yflip ? ", Y-flipped" : "")
                                    );
}

void TilesetEditor::showTileStatus(uint16_t tileId) {
    this->ui->statusbar->showMessage(QString("Tile: %1").arg(Util::toHexString(tileId, 3)));
}

void TilesetEditor::onHoveredTileCleared() {
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::paintSelectedLayerTiles(const QPoint &pos, bool paletteOnly) {
    if (!this->metatile) return;

    bool changed = false;
    Metatile *prevMetatile = new Metatile(*this->metatile);
    QSize dimensions = this->tileSelector->getSelectionDimensions();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    int srcTileIndex = 0;
    int maxTileIndex = projectConfig.getNumTilesInMetatile();
    for (int y = 0; y < dimensions.height(); y++) {
        for (int x = 0; x < dimensions.width(); x++) {
            int destTileIndex = this->metatileLayersItem->posToTileIndex(pos.x() + x, pos.y() + y);
            if (destTileIndex < maxTileIndex) {
                Tile &destTile = this->metatile->tiles[destTileIndex];
                const Tile srcTile = tiles.value(srcTileIndex++);
                if (paletteOnly) {
                    if (srcTile.palette == destTile.palette)
                        continue; // Ignore no-ops for edit history
                    destTile.palette = srcTile.palette;
                } else {
                    if (srcTile == destTile)
                        continue; // Ignore no-ops for edit history

                    // Update tile usage count
                    if (this->tileSelector->showUnused && destTile.tileId != srcTile.tileId) {
                        this->tileSelector->usedTiles[srcTile.tileId] += 1;
                        this->tileSelector->usedTiles[destTile.tileId] -= 1;
                    }
                    destTile = srcTile;
                }
                changed = true;
            }
        }
    }
    if (!changed) {
        delete prevMetatile;
        return;
    }

    this->metatileSelector->drawSelectedMetatile();
    this->metatileLayersItem->draw();
    updateLayerTileStatus();
    this->tileSelector->draw();
    this->commitMetatileChange(prevMetatile);
}

void TilesetEditor::onMetatileLayerSelectionChanged(const QPoint &selectionOrigin, const QSize &size) {
    QList<Tile> tiles;
    for (int y = 0; y < size.height(); y++) {
        for (int x = 0; x < size.width(); x++) {
            int tileIndex = this->metatileLayersItem->posToTileIndex(selectionOrigin.x() + x, selectionOrigin.y() + y);
            tiles.append(this->metatile ? this->metatile->tiles.value(tileIndex) : Tile());
        }
    }

    this->tileSelector->setExternalSelection(size.width(), size.height(), tiles);
    if (size == QSize(1,1)) {
        setPaletteId(tiles[0].palette);
        this->tileSelector->highlight(tiles[0].tileId);
        this->redrawTileSelector();
    }
}

void TilesetEditor::setPaletteId(int paletteId) {
    ui->spinBox_paletteSelector->setValue(paletteId);
}

int TilesetEditor::paletteId() const {
    return ui->spinBox_paletteSelector->value();
}

void TilesetEditor::refreshPaletteId() {
    this->tileSelector->setPaletteId(paletteId());
    this->drawSelectedTiles();
    if (this->paletteEditor) {
        this->paletteEditor->setPaletteId(paletteId());
    }
}

void TilesetEditor::refreshTileFlips() {
    this->tileSelector->setTileFlips(ui->checkBox_xFlip->isChecked(), ui->checkBox_yFlip->isChecked());
    this->drawSelectedTiles();
}

void TilesetEditor::setMetatileLabel(QString label)
{
    this->ui->lineEdit_MetatileLabel->setText(label);
    commitMetatileLabel();
}

void TilesetEditor::commitMetatileLabel() {
    if (!this->metatile) return;

    // Only commit if the field has changed.
    uint16_t metatileId = this->getSelectedMetatileId();
    QString oldLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    QString newLabel = this->ui->lineEdit_MetatileLabel->text();
    if (oldLabel != newLabel) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        Tileset::setMetatileLabel(metatileId, newLabel, this->primaryTileset, this->secondaryTileset);
        this->commitMetatileAndLabelChange(prevMetatile, oldLabel);
    }
}

void TilesetEditor::commitMetatileAndLabelChange(Metatile * prevMetatile, QString prevLabel) {
    if (!this->metatile) return;

    commit(new MetatileHistoryItem(this->getSelectedMetatileId(),
                                    prevMetatile, new Metatile(*this->metatile),
                                    prevLabel, this->ui->lineEdit_MetatileLabel->text()));
}

void TilesetEditor::commitMetatileChange(Metatile * prevMetatile)
{
    this->commitMetatileAndLabelChange(prevMetatile, this->ui->lineEdit_MetatileLabel->text());
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
        int i = ui->comboBox_LayerType->findText(text);
        if (i >= 0) return i;
    }
    return text.toUInt(ok, 0);
}

void TilesetEditor::commitAttributeFromComboBox(Metatile::Attr attribute, NoScrollComboBox *combo) {
    if (!this->metatile) return;

    bool ok;
    uint32_t newValue = this->attributeNameToValue(attribute, combo->currentText(), &ok);
    if (ok && newValue != this->metatile->getAttribute(attribute)) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setAttribute(attribute, newValue);
        this->commitMetatileChange(prevMetatile);

        // When an attribute changes we also need to update the raw value display.
        const QSignalBlocker b_RawAttributesValue(ui->spinBox_RawAttributesValue);
        ui->spinBox_RawAttributesValue->setValue(this->metatile->getAttributes());
    }

    // Update the text in the combo box to reflect the final value.
    // The text may change if the input text was invalid, the value was too large to fit, or if a number was entered that we know an identifier for.
    const QSignalBlocker b(combo);
    combo->setHexItem(this->metatile->getAttribute(attribute));
}

void TilesetEditor::onRawAttributesEdited() {
    if (!this->metatile) return;

    uint32_t newAttributes = ui->spinBox_RawAttributesValue->value();
     if (newAttributes != this->metatile->getAttributes()) {
        Metatile *prevMetatile = new Metatile(*this->metatile);
        this->metatile->setAttributes(newAttributes);
        this->commitMetatileChange(prevMetatile);
    }
    refreshMetatileAttributes();
}

void TilesetEditor::refreshMetatileAttributes() {
    if (!this->metatile) return;

    const QSignalBlocker b_MetatileBehaviors(ui->comboBox_MetatileBehaviors);
    const QSignalBlocker b_EncounterType(ui->comboBox_EncounterType);
    const QSignalBlocker b_TerrainType(ui->comboBox_TerrainType);
    const QSignalBlocker b_LayerType(ui->comboBox_LayerType);
    const QSignalBlocker b_RawAttributesValue(ui->spinBox_RawAttributesValue);
    ui->comboBox_MetatileBehaviors->setHexItem(this->metatile->behavior());
    ui->comboBox_EncounterType->setHexItem(this->metatile->encounterType());
    ui->comboBox_TerrainType->setHexItem(this->metatile->terrainType());
    ui->comboBox_LayerType->setHexItem(this->metatile->layerType());
    ui->spinBox_RawAttributesValue->setValue(this->metatile->getAttributes());

    this->metatileSelector->drawSelectedMetatile();
}

void TilesetEditor::commitMetatileBehavior() {
    commitAttributeFromComboBox(Metatile::Attr::Behavior, ui->comboBox_MetatileBehaviors);
}

void TilesetEditor::commitEncounterType() {
    commitAttributeFromComboBox(Metatile::Attr::EncounterType, ui->comboBox_EncounterType);
}

void TilesetEditor::commitTerrainType() {
    commitAttributeFromComboBox(Metatile::Attr::TerrainType, ui->comboBox_TerrainType);
};

void TilesetEditor::commitLayerType() {
    commitAttributeFromComboBox(Metatile::Attr::LayerType, ui->comboBox_LayerType);
    this->metatileSelector->drawSelectedMetatile(); // Changing the layer type can affect how fully transparent metatiles appear
}

bool TilesetEditor::save() {
    // Need this temporary flag to stop selection resetting after saving.
    // This is a workaround; redrawing the map's metatile selector shouldn't emit the same signal as when it's selected.
    this->lockSelection = true;

    bool success = this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    applyMetatileSwapsToLayouts();
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
    int maxAllowedTiles = primary ? Project::getNumTilesPrimary() : Project::getNumTilesSecondary();
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
    // If focus is still on any input widgets, a user may have made changes
    // but the widget hasn't had a chance to fire the 'editingFinished' signal.
    // Make sure they lose focus before we close so that changes aren't missed.
    setFocus();

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

        // Our selected metatile ID may have become invalid. Make sure it's in-bounds.
        uint16_t metatileId = this->metatileSelector->getSelectedMetatileId();
        Tileset *tileset = Tileset::getMetatileTileset(metatileId, this->primaryTileset, this->secondaryTileset);
        if (tileset && !tileset->containsMetatileId(metatileId)) {
            this->metatileSelector->select(qBound(tileset->firstMetatileId(), metatileId, tileset->lastMetatileId()));
        }

        refresh();
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_actionChange_Palettes_triggered()
{
    if (!this->paletteEditor) {
        this->paletteEditor = new PaletteEditor(this->project, this->primaryTileset,
                                                this->secondaryTileset, this->paletteId(), this);
        connect(this->paletteEditor, &PaletteEditor::changedPaletteColor, this, &TilesetEditor::onPaletteEditorChangedPaletteColor);
        connect(this->paletteEditor, &PaletteEditor::changedPalette, this, &TilesetEditor::setPaletteId);
        connect(this->paletteEditor, &PaletteEditor::metatileSelected, this, &TilesetEditor::selectMetatile);
    }
    Util::show(this->paletteEditor);
}

void TilesetEditor::onPaletteEditorChangedPaletteColor() {
    this->refresh();
    this->hasUnsavedChanges = true;
}

bool TilesetEditor::replaceMetatile(uint16_t metatileId, const Metatile &src, QString newLabel) {
    Metatile * dest = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    QString oldLabel = Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    if (!dest || (*dest == src && oldLabel == newLabel))
        return false;

    Tileset::setMetatileLabel(metatileId, newLabel, this->primaryTileset, this->secondaryTileset);
    if (metatileId == this->getSelectedMetatileId())
        this->ui->lineEdit_MetatileLabel->setText(newLabel);

    // Update tile usage if any tiles changed
    if (this->tileSelector && this->tileSelector->showUnused) {
        int numTiles = projectConfig.getNumTilesInMetatile();
        for (int i = 0; i < numTiles; i++) {
            if (src.tiles[i].tileId != dest->tiles[i].tileId) {
                this->tileSelector->usedTiles[src.tiles[i].tileId] += 1;
                this->tileSelector->usedTiles[dest->tiles[i].tileId] -= 1;
            }
        }
        this->tileSelector->draw();
    }

    this->metatile = dest;
    *this->metatile = src;
    this->metatileSelector->select(metatileId);
    this->metatileSelector->drawMetatile(metatileId);
    this->metatileLayersItem->draw();
    updateLayerTileStatus();
    return true;
}

void TilesetEditor::initMetatileHistory() {
    this->metatileHistory.clear();
    updateEditHistoryActions();
    this->hasUnsavedChanges = false;
}

void TilesetEditor::commit(MetatileHistoryItem *item) {
    this->metatileHistory.push(item);
    updateEditHistoryActions();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::updateEditHistoryActions() {
    ui->actionUndo->setEnabled(this->metatileHistory.canUndo());
    ui->actionRedo->setEnabled(this->metatileHistory.canRedo());
}

void TilesetEditor::on_actionUndo_triggered() {
    MetatileHistoryItem *commit = this->metatileHistory.current();
    if (!commit) return;
    this->metatileHistory.back();

    if (commit->isSwap) {
        swapMetatiles(commit->swapMetatileId, commit->metatileId);
    } else if (commit->prevMetatile) {
        replaceMetatile(commit->metatileId, *commit->prevMetatile, commit->prevLabel);
    };
    updateEditHistoryActions();
}

void TilesetEditor::on_actionRedo_triggered() {
    MetatileHistoryItem *commit = this->metatileHistory.next();
    if (!commit) return;

    if (commit->isSwap) {
        swapMetatiles(commit->metatileId, commit->swapMetatileId);
    } else if (commit->newMetatile) {
        replaceMetatile(commit->metatileId, *commit->newMetatile, commit->newLabel);
    }
    updateEditHistoryActions();
}

void TilesetEditor::on_actionCut_triggered()
{
    this->copyMetatile(true);
    this->pasteMetatile(Metatile(projectConfig.getNumTilesInMetatile()), "");
}

void TilesetEditor::on_actionCopy_triggered()
{
    this->copyMetatile(false);
}

void TilesetEditor::on_actionPaste_triggered()
{
    if (this->copiedMetatile) {
        this->pasteMetatile(*this->copiedMetatile, this->copiedMetatileLabel);
    }
}

void TilesetEditor::copyMetatile(bool cut) {
    uint16_t metatileId = this->getSelectedMetatileId();
    Metatile * toCopy = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    if (!toCopy) return;

    if (!this->copiedMetatile)
        this->copiedMetatile = new Metatile(*toCopy);
    else
        *this->copiedMetatile = *toCopy;

    ui->actionPaste->setEnabled(true);

    // Don't try to copy the label unless it's a cut, these should be unique to each metatile.
    this->copiedMetatileLabel = cut ? Tileset::getOwnedMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset) : QString();
}

void TilesetEditor::pasteMetatile(const Metatile &toPaste, QString newLabel) {
    if (!this->metatile) return;

    Metatile *prevMetatile = new Metatile(*this->metatile);
    QString prevLabel = this->ui->lineEdit_MetatileLabel->text();
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
    QString defaultFilepath = QString("%1/%2_Tiles_Pal%3.png").arg(FileDialog::getDirectory()).arg(tileset->name).arg(this->paletteId());
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
        commit(new MetatileHistoryItem(metatileId,
                                       prevMetatile, new Metatile(*metatiles.at(i)),
                                       prevLabel, prevLabel));
    }

    tileset->setMetatiles(metatiles);
    this->refresh();
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
    this->tileSelector->usedTiles.resize(Project::getNumTilesTotal());
    this->tileSelector->usedTiles.fill(0);

    auto countTilesetTileUsage = [this](Tileset *searchTileset) {
        // Count usage of our search tileset's tiles (in itself, and in any tilesets it gets paired with).
        QSet<QString> tilesetNames = this->project->getPairedTilesetLabels(searchTileset);
        QSet<Tileset*> tilesets;

        // For the currently-loaded tilesets, make sure we use the Tileset Editor's versions
        // (which may contain unsaved changes) and not the versions from the project.
        tilesetNames.remove(this->primaryTileset->name);
        tilesetNames.remove(this->secondaryTileset->name);
        tilesets.insert(this->primaryTileset);
        tilesets.insert(this->secondaryTileset);

        for (const auto &tilesetName : tilesetNames) {
            Tileset *tileset = this->project->getTileset(tilesetName);
            if (tileset) tilesets.insert(tileset);
        }
        for (const auto &tileset : tilesets) {
            for (const auto &metatile : tileset->metatiles()) {
                for (const auto &tile : metatile->tiles) {
                    if (searchTileset->containsTileId(tile.tileId)) {
                        this->tileSelector->usedTiles[tile.tileId]++;
                    }
                }
            }
        }
    };

    countTilesetTileUsage(this->primaryTileset);
    countTilesetTileUsage(this->secondaryTileset);
}

void TilesetEditor::on_copyButton_MetatileLabel_clicked() {
    uint16_t metatileId = this->getSelectedMetatileId();
    QString label = Tileset::getMetatileLabel(metatileId, this->primaryTileset, this->secondaryTileset);
    if (label.isEmpty()) return;
    Tileset * tileset = Tileset::getMetatileLabelTileset(metatileId, this->primaryTileset, this->secondaryTileset);
    if (tileset)
        label.prepend(tileset->getMetatileLabelPrefix());
    QGuiApplication::clipboard()->setText(label);
    QToolTip::showText(this->ui->copyButton_MetatileLabel->mapToGlobal(QPoint(0, 0)), "Copied!");
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

void TilesetEditor::commitMetatileSwap(uint16_t metatileIdA, uint16_t metatileIdB) {
    if (swapMetatiles(metatileIdA, metatileIdB)) {
        commit(new MetatileHistoryItem(metatileIdA, metatileIdB));
    }
}

bool TilesetEditor::swapMetatiles(uint16_t metatileIdA, uint16_t metatileIdB) {
    this->metatileSelector->clearSwapSelection();

    QList<Metatile*> metatiles;
    for (const auto &metatileId : {metatileIdA, metatileIdB}) {
        Metatile *metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
        if (metatile) {
            metatiles.append(metatile);
        } else {
            logError(QString("Failed to load metatile %1 for swap.").arg(Metatile::getMetatileIdString(metatileId)));
        }
    }
    if (metatiles.length() < 2)
        return false;

    // Swap the metatile data in the tileset
    Metatile tempMetatile = *metatiles.at(0);
    QString tempLabel = Tileset::getOwnedMetatileLabel(metatileIdA, this->primaryTileset, this->secondaryTileset);
    replaceMetatile(metatileIdA, *metatiles.at(1), Tileset::getOwnedMetatileLabel(metatileIdB, this->primaryTileset, this->secondaryTileset));
    replaceMetatile(metatileIdB, tempMetatile, tempLabel);

    // Record this swap so that we can update the layouts later.
    // If this is the inverse of the most recent swap (e.g. from Undo), we instead remove that swap to save time.
    if (!this->metatileIdSwaps.isEmpty()) {
        auto recentSwapPair = this->metatileIdSwaps.constLast();
        if (recentSwapPair.first == metatileIdB && recentSwapPair.second == metatileIdA) {
            this->metatileIdSwaps.removeLast();
            return true;
        }
    }
    this->metatileIdSwaps.append(QPair<uint16_t,uint16_t>(metatileIdA, metatileIdB));
    return true;
}

// If any metatiles swapped positions, apply the swap to all relevant layouts.
// We only do this once changes in the Tileset Editor are saved.
void TilesetEditor::applyMetatileSwapsToLayouts() {
    if (this->metatileIdSwaps.isEmpty())
        return;

    QProgressDialog progress("", "", 0, this->metatileIdSwaps.length(), this);
    progress.setAutoClose(true);
    progress.setWindowModality(Qt::WindowModal);
    progress.setModal(true);
    progress.setMinimumDuration(1000);
    progress.setValue(progress.minimum());

    for (const auto &swapPair : this->metatileIdSwaps) {
        progress.setLabelText(QString("Swapping metatiles %1 and %2 in map layouts...")
                                        .arg(Metatile::getMetatileIdString(swapPair.first))
                                        .arg(Metatile::getMetatileIdString(swapPair.second)));
        applyMetatileSwapToLayouts(swapPair.first, swapPair.second);
        progress.setValue(progress.value() + 1);
    }
    this->metatileIdSwaps.clear();
}

void TilesetEditor::applyMetatileSwapToLayouts(uint16_t metatileIdA, uint16_t metatileIdB) {
    struct TilesetPair {
        Tileset* primary = nullptr;
        Tileset* secondary = nullptr;
    };
    TilesetPair tilesets;

    // Get which tilesets our swapped metatiles belong to.
    auto addSourceTileset = [this](uint16_t metatileId, TilesetPair *tilesets) {
        if (this->primaryTileset->containsMetatileId(metatileId)) {
            tilesets->primary = this->primaryTileset;
        } else if (this->secondaryTileset->containsMetatileId(metatileId)) {
            tilesets->secondary = this->secondaryTileset;
        } else {
            // Invalid metatile, shouldn't happen
            this->metatileSelector->removeFromSwapSelection(metatileId);
        }
    };
    addSourceTileset(metatileIdA, &tilesets);
    addSourceTileset(metatileIdB, &tilesets);
    if (!tilesets.primary && !tilesets.secondary) {
        return;
    }

    // In each layout that uses the appropriate tileset(s), swap the two metatiles.
    QSet<QString> layoutIds = this->project->getTilesetLayoutIds(tilesets.primary, tilesets.secondary);
    for (const auto &layoutId : layoutIds) {
        Layout *layout = this->project->loadLayout(layoutId);
        if (!layout) continue;
        // Perform swap(s) in layout's main data.
        for (int y = 0; y < layout->height; y++)
        for (int x = 0; x < layout->width; x++) {
            uint16_t metatileId = layout->getMetatileId(x, y);
            if (metatileId == metatileIdA) {
                layout->setMetatileId(x, y, metatileIdB);
            } else if (metatileId == metatileIdB) {
                layout->setMetatileId(x, y, metatileIdA);
            } else continue;
            layout->hasUnsavedDataChanges = true;
        }
        // Perform swap(s) in layout's border data.
        for (auto &borderBlock : layout->border) {
            if (borderBlock.metatileId() == metatileIdA) {
                borderBlock.setMetatileId(metatileIdB);
            } else if (borderBlock.metatileId() == metatileIdB) {
                borderBlock.setMetatileId(metatileIdA);
            } else continue;
            layout->hasUnsavedDataChanges = true;
        }
    }
}

void TilesetEditor::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && ui->actionSwap_Metatiles->isChecked()) {
        ui->actionSwap_Metatiles->setChecked(false);
    } else {
        QMainWindow::keyPressEvent(event);
    }
}
