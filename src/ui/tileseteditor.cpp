#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "imageproviders.h"
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QDialogButtonBox>

TilesetEditor::TilesetEditor(Project *project, QString primaryTilesetLabel, QString secondaryTilesetLabel, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor)
{
    this->init(project, primaryTilesetLabel, secondaryTilesetLabel);
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
}

void TilesetEditor::init(Project *project, QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    ui->setupUi(this);
    this->project = project;

    this->hasUnsavedChanges = false;
    this->tileXFlip = ui->checkBox_xFlip->isChecked();
    this->tileYFlip = ui->checkBox_yFlip->isChecked();
    this->paletteId = ui->spinBox_paletteSelector->value();

    Tileset *primaryTileset = project->getTileset(primaryTilesetLabel);
    Tileset *secondaryTileset = project->getTileset(secondaryTilesetLabel);
    if (this->primaryTileset) delete this->primaryTileset;
    if (this->secondaryTileset) delete this->secondaryTileset;
    this->primaryTileset = primaryTileset->copy();
    this->secondaryTileset = secondaryTileset->copy();

    QList<QString> sortedBehaviors;
    for (int num : project->metatileBehaviorMapInverse.keys()) {
        this->ui->comboBox_metatileBehaviors->addItem(project->metatileBehaviorMapInverse[num], num);
    }
    this->ui->comboBox_layerType->addItem("Normal - Middle/Top", 0);
    this->ui->comboBox_layerType->addItem("Covered - Bottom/Middle", 1);
    this->ui->comboBox_layerType->addItem("Split - Bottom/Top", 2);
    this->ui->spinBox_paletteSelector->setMinimum(0);
    this->ui->spinBox_paletteSelector->setMaximum(Project::getNumPalettesTotal() - 1);

    this->initMetatileSelector();
    this->initMetatileLayersItem();
    this->initTileSelector();
    this->initSelectedTileItem();
    this->metatileSelector->select(0);
}

void TilesetEditor::setTilesets(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    delete this->primaryTileset;
    delete this->secondaryTileset;
    Tileset *primaryTileset = project->getTileset(primaryTilesetLabel);
    Tileset *secondaryTileset = project->getTileset(secondaryTilesetLabel);
    this->primaryTileset = primaryTileset->copy();
    this->secondaryTileset = secondaryTileset->copy();
    this->refresh();
}

void TilesetEditor::refresh() {
    this->metatileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->tileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->select(this->metatileSelector->getSelectedMetatile());
    this->drawSelectedTiles();

    this->ui->graphicsView_Tiles->setSceneRect(0, 0, this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setSceneRect(0, 0, this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width(), this->selectedTilePixmapItem->pixmap().height());
}

void TilesetEditor::initMetatileSelector()
{
    this->metatileSelector = new TilesetEditorMetatileSelector(this->primaryTileset, this->secondaryTileset);
    connect(this->metatileSelector, SIGNAL(hoveredMetatileChanged(uint16_t)),
            this, SLOT(onHoveredMetatileChanged(uint16_t)));
    connect(this->metatileSelector, SIGNAL(hoveredMetatileCleared()),
            this, SLOT(onHoveredMetatileCleared()));
    connect(this->metatileSelector, SIGNAL(selectedMetatileChanged(uint16_t)),
            this, SLOT(onSelectedMetatileChanged(uint16_t)));

    this->metatilesScene = new QGraphicsScene;
    this->metatilesScene->addItem(this->metatileSelector);
    this->metatileSelector->draw();

    this->ui->graphicsView_Metatiles->setScene(this->metatilesScene);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
}

void TilesetEditor::initTileSelector()
{
    this->tileSelector = new TilesetEditorTileSelector(this->primaryTileset, this->secondaryTileset);
    connect(this->tileSelector, SIGNAL(hoveredTileChanged(uint16_t)),
            this, SLOT(onHoveredTileChanged(uint16_t)));
    connect(this->tileSelector, SIGNAL(hoveredTileCleared()),
            this, SLOT(onHoveredTileCleared()));
    connect(this->tileSelector, SIGNAL(selectedTilesChanged()),
            this, SLOT(onSelectedTilesChanged()));

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
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width(), this->selectedTilePixmapItem->pixmap().height());
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
            QImage tileImage = getColoredTileImage(tiles.at(tileIndex).tile, this->primaryTileset, this->secondaryTileset, tiles.at(tileIndex).palette)
                    .mirrored(tiles.at(tileIndex).xflip, tiles.at(tileIndex).yflip)
                    .scaled(16, 16);
            tileIndex++;
            painter.drawImage(i * 16, j * 16, tileImage);
        }
    }

    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(selectionImage));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width(), this->selectedTilePixmapItem->pixmap().height());
}

void TilesetEditor::initMetatileLayersItem() {
    Metatile *metatile = Tileset::getMetatile(this->metatileSelector->getSelectedMetatile(), this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, SIGNAL(tileChanged(int, int)),
            this, SLOT(onMetatileLayerTileChanged(int, int)));
    connect(this->metatileLayersItem, SIGNAL(selectedTilesChanged(QPoint, int, int)),
            this, SLOT(onMetatileLayerSelectionChanged(QPoint, int, int)));

    this->metatileLayersScene = new QGraphicsScene;
    this->metatileLayersScene->addItem(this->metatileLayersItem);
    this->ui->graphicsView_metatileLayers->setScene(this->metatileLayersScene);
}

void TilesetEditor::onHoveredMetatileChanged(uint16_t metatileId) {
    QString message = QString("Metatile: 0x%1")
                        .arg(QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper());
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredMetatileCleared() {
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedMetatileChanged(uint16_t metatileId) {
    this->metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem->setMetatile(metatile);
    this->metatileLayersItem->draw();
    this->ui->comboBox_metatileBehaviors->setCurrentIndex(this->ui->comboBox_metatileBehaviors->findData(this->metatile->behavior));
    this->ui->comboBox_layerType->setCurrentIndex(this->ui->comboBox_layerType->findData(this->metatile->layerType));
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
    int maxTileIndex = x < 2 ? 3 : 7;
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    int selectedTileIndex = 0;
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex <= maxTileIndex) {
                Tile tile = this->metatile->tiles->at(tileIndex);
                tile.tile = tiles.at(selectedTileIndex).tile;
                tile.xflip = tiles.at(selectedTileIndex).xflip;
                tile.yflip = tiles.at(selectedTileIndex).yflip;
                tile.palette = tiles.at(selectedTileIndex).palette;
                (*this->metatile->tiles)[tileIndex] = tile;
            }
            selectedTileIndex++;
        }
    }

    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::onMetatileLayerSelectionChanged(QPoint selectionOrigin, int width, int height) {
    QList<Tile> tiles;
    int x = selectionOrigin.x();
    int y = selectionOrigin.y();
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < 8) {
                tiles.append(this->metatile->tiles->at(tileIndex));
            }
        }
    }
    this->tileSelector->setExternalSelection(width, height, tiles);
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
}

void TilesetEditor::on_checkBox_xFlip_stateChanged(int checked)
{
    this->tileXFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
}

void TilesetEditor::on_checkBox_yFlip_stateChanged(int checked)
{
    this->tileYFlip = checked;
    this->tileSelector->setTileFlips(this->tileXFlip, this->tileYFlip);
    this->drawSelectedTiles();
}

void TilesetEditor::on_comboBox_metatileBehaviors_currentIndexChanged(const QString &metatileBehavior)
{
    if (this->metatile) {
        this->metatile->behavior = static_cast<uint8_t>(project->metatileBehaviorMap[metatileBehavior]);
    }
}

void TilesetEditor::on_comboBox_layerType_currentIndexChanged(int layerType)
{
    if (this->metatile) {
        this->metatile->layerType = static_cast<uint8_t>(layerType);
    }
}

void TilesetEditor::on_actionSave_Tileset_triggered()
{
    this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    emit this->tilesetsSaved(this->primaryTileset->name, this->secondaryTileset->name);
    this->ui->statusbar->showMessage(QString("Saved primary and secondary Tilesets!"), 5000);
    this->hasUnsavedChanges = false;
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
                this->project->root,
                "Image Files (*.png)");
    if (filepath.isEmpty()) {
        return;
    }

    qDebug() << QString("Importing %1 tileset tiles '%2'").arg(descriptor).arg(filepath);

    // Validate image dimensions.
    QImage image = QImage(filepath);
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

    // Validate image is properly indexed to 16 colors.
    if (image.colorCount() != 16) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(QString("The image must be indexed and contain 16 total colors. The provided image has %1 indexed colors.")
                                  .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Validate total number of tiles in image.
    int numTilesWide = image.width() / 16;
    int numTilesHigh = image.height() / 16;
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

    this->project->loadTilesetTiles(tileset, image);
    this->project->loadTilesetMetatiles(tileset);
    this->refresh();
    this->hasUnsavedChanges = true;
}

void TilesetEditor::closeEvent(QCloseEvent *event)
{
    bool close = true;
    if (this->hasUnsavedChanges) {
        QMessageBox::StandardButton result = QMessageBox::question(this, "porymap",
                                                                    "Discard unsaved Tileset changes?",
                                                                    QMessageBox::No | QMessageBox::Yes,
                                                                    QMessageBox::Yes);
        close = result == QMessageBox::Yes;
    }

    if (close) {
        event->accept();
        emit closed();
    } else {
        event->ignore();
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
    primarySpinBox->setValue(this->primaryTileset->metatiles->length());
    secondarySpinBox->setValue(this->secondaryTileset->metatiles->length());
    form.addRow(new QLabel("Primary Tileset"), primarySpinBox);
    form.addRow(new QLabel("Secondary Tileset"), secondarySpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        int numPrimaryMetatiles = primarySpinBox->value();
        int numSecondaryMetatiles = secondarySpinBox->value();
        while (this->primaryTileset->metatiles->length() > numPrimaryMetatiles) {
            Metatile *metatile = this->primaryTileset->metatiles->takeLast();
            delete metatile;
        }
        while (this->primaryTileset->metatiles->length() < numPrimaryMetatiles) {
            Tile tile;
            tile.palette = 0;
            tile.tile = 0;
            tile.xflip = false;
            tile.yflip = false;
            Metatile *metatile = new Metatile;
            metatile->behavior = 0;
            metatile->layerType = 0;
            for (int i = 0; i < 8; i++) {
                metatile->tiles->append(tile);
            }
            this->primaryTileset->metatiles->append(metatile);
        }
        while (this->secondaryTileset->metatiles->length() > numSecondaryMetatiles) {
            Metatile *metatile = this->secondaryTileset->metatiles->takeLast();
            delete metatile;
        }
        while (this->secondaryTileset->metatiles->length() < numSecondaryMetatiles) {
            Tile tile;
            tile.palette = 0;
            tile.tile = 0;
            tile.xflip = 0;
            tile.yflip = 0;
            Metatile *metatile = new Metatile;
            metatile->behavior = 0;
            metatile->layerType = 0;
            for (int i = 0; i < 8; i++) {
                metatile->tiles->append(tile);
            }
            this->secondaryTileset->metatiles->append(metatile);
        }

        this->refresh();
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::onPaletteEditorClosed() {
    if (this->paletteEditor) {
        delete this->paletteEditor;
        this->paletteEditor = nullptr;
    }
}

void TilesetEditor::on_actionChange_Palettes_triggered()
{
    if (!this->paletteEditor) {
        this->paletteEditor = new PaletteEditor(this->project, this->primaryTileset, this->secondaryTileset, this);
        connect(this->paletteEditor, SIGNAL(closed()), this, SLOT(onPaletteEditorClosed()));
        connect(this->paletteEditor, SIGNAL(changedPaletteColor()), this, SLOT(onPaletteEditorChangedPaletteColor()));
        connect(this->paletteEditor, SIGNAL(changedPalette(int)), this, SLOT(onPaletteEditorChangedPalette(int)));
    }

    if (!this->paletteEditor->isVisible()) {
        this->paletteEditor->show();
    } else if (this->paletteEditor->isMinimized()) {
        this->paletteEditor->showNormal();
    } else {
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
