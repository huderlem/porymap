#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "log.h"
#include "imageproviders.h"
#include "metatileparser.h"
#include "paletteutil.h"
#include "imageexport.h"
#include "config.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QImageReader>

TilesetEditor::TilesetEditor(Project *project, QString primaryTilesetLabel, QString secondaryTilesetLabel, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor)
{
    this->init(project, primaryTilesetLabel, secondaryTilesetLabel);
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

    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        this->ui->comboBox_encounterType->setVisible(true);
        this->ui->label_encounterType->setVisible(true);
        this->ui->comboBox_encounterType->addItem("None", 0);
        this->ui->comboBox_encounterType->addItem("Land", 1);
        this->ui->comboBox_encounterType->addItem("Water", 2);
        this->ui->comboBox_terrainType->setVisible(true);
        this->ui->label_terrainType->setVisible(true);
        this->ui->comboBox_terrainType->addItem("Normal", 0);
        this->ui->comboBox_terrainType->addItem("Grass", 1);
        this->ui->comboBox_terrainType->addItem("Water", 2);
        this->ui->comboBox_terrainType->addItem("Waterfall", 3);
    } else {
        this->ui->comboBox_encounterType->setVisible(false);
        this->ui->label_encounterType->setVisible(false);
        this->ui->comboBox_terrainType->setVisible(false);
        this->ui->label_terrainType->setVisible(false);
    }

    //only allow characters valid for a symbol
    QRegExp expression("[_A-Za-z0-9]*$");
    QRegExpValidator *validator = new QRegExpValidator(expression);
    this->ui->lineEdit_metatileLabel->setValidator(validator);

    this->initMetatileSelector();
    this->initMetatileLayersItem();
    this->initTileSelector();
    this->initSelectedTileItem();
    this->metatileSelector->select(0);

    MetatileHistoryItem *commit = new MetatileHistoryItem(0, nullptr, this->metatile->copy());
    metatileHistory.push(commit);
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
    this->metatileLayersItem->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->tileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->setTilesets(this->primaryTileset, this->secondaryTileset);
    this->metatileSelector->select(this->metatileSelector->getSelectedMetatile());
    this->drawSelectedTiles();

    this->ui->graphicsView_Tiles->setSceneRect(0, 0, this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setSceneRect(0, 0, this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
    this->ui->graphicsView_selectedTile->setFixedSize(this->selectedTilePixmapItem->pixmap().width() + 2, this->selectedTilePixmapItem->pixmap().height() + 2);
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
            QImage tileImage = getPalettedTileImage(tiles.at(tileIndex).tile, this->primaryTileset, this->secondaryTileset, tiles.at(tileIndex).palette, true)
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
    Metatile *metatile = Tileset::getMetatile(metatileId, this->primaryTileset, this->secondaryTileset);
    QString message;
    QString hexString = QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper();
    if (metatile && metatile->label.size() != 0) {
        message = QString("Metatile: 0x%1 \"%2\"").arg(hexString, metatile->label);
    } else {
        message = QString("Metatile: 0x%1").arg(hexString);
    }
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
    this->ui->lineEdit_metatileLabel->setText(this->metatile->label);
    this->ui->comboBox_layerType->setCurrentIndex(this->ui->comboBox_layerType->findData(this->metatile->layerType));
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        this->ui->comboBox_encounterType->setCurrentIndex(this->ui->comboBox_encounterType->findData(this->metatile->encounterType));
        this->ui->comboBox_terrainType->setCurrentIndex(this->ui->comboBox_terrainType->findData(this->metatile->terrainType));
    }
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
    const QList<QPoint> tileCoords = QList<QPoint>{
        QPoint(0, 0),
        QPoint(1, 0),
        QPoint(0, 1),
        QPoint(1, 1),
        QPoint(2, 0),
        QPoint(3, 0),
        QPoint(2, 1),
        QPoint(3, 1)
    };
    Metatile *prevMetatile = this->metatile->copy();
    QPoint dimensions = this->tileSelector->getSelectionDimensions();
    QList<Tile> tiles = this->tileSelector->getSelectedTiles();
    int selectedTileIndex = 0;
    for (int j = 0; j < dimensions.y(); j++) {
        for (int i = 0; i < dimensions.x(); i++) {
            int tileIndex = ((x + i) / 2 * 4) + ((y + j) * 2) + ((x + i) % 2);
            if (tileIndex < 8
             && tileCoords.at(tileIndex).x() >= x
             && tileCoords.at(tileIndex).y() >= y){
                Tile *tile = &(*this->metatile->tiles)[tileIndex];
                tile->tile = tiles.at(selectedTileIndex).tile;
                tile->xflip = tiles.at(selectedTileIndex).xflip;
                tile->yflip = tiles.at(selectedTileIndex).yflip;
                tile->palette = tiles.at(selectedTileIndex).palette;
            }
            selectedTileIndex++;
        }
    }

    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
    this->hasUnsavedChanges = true;

    MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
    metatileHistory.push(commit);
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

    if (width == 1 && height == 1) {
        this->tileSelector->select(static_cast<uint16_t>(tiles[0].tile));
        ui->spinBox_paletteSelector->setValue(tiles[0].palette);
        ui->checkBox_xFlip->setChecked(tiles[0].xflip);
        ui->checkBox_yFlip->setChecked(tiles[0].yflip);
        QPoint pos = tileSelector->getTileCoordsOnWidget(static_cast<uint16_t>(tiles[0].tile));
        ui->scrollArea_Tiles->ensureVisible(pos.x(), pos.y());
    }
    else {
        this->tileSelector->setExternalSelection(width, height, tiles);
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

void TilesetEditor::on_comboBox_metatileBehaviors_activated(const QString &metatileBehavior)
{
    if (this->metatile) {
        Metatile *prevMetatile = this->metatile->copy();
        this->metatile->behavior = static_cast<uint8_t>(project->metatileBehaviorMap[metatileBehavior]);
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
        metatileHistory.push(commit);
    }
}

void TilesetEditor::on_lineEdit_metatileLabel_editingFinished()
{
    saveMetatileLabel();
}

void TilesetEditor::saveMetatileLabel()
{
    // Only commit if the field has changed.
    if (this->metatile && this->metatile->label != this->ui->lineEdit_metatileLabel->text()) {
        Metatile *prevMetatile = this->metatile->copy();
        this->metatile->label = this->ui->lineEdit_metatileLabel->text();
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
        metatileHistory.push(commit);
    }
}

void TilesetEditor::on_comboBox_layerType_activated(int layerType)
{
    if (this->metatile) {
        Metatile *prevMetatile = this->metatile->copy();
        this->metatile->layerType = static_cast<uint8_t>(layerType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
        metatileHistory.push(commit);
    }
}

void TilesetEditor::on_comboBox_encounterType_activated(int encounterType)
{
    if (this->metatile) {
        Metatile *prevMetatile = this->metatile->copy();
        this->metatile->encounterType = static_cast<uint8_t>(encounterType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
        metatileHistory.push(commit);
    }
}

void TilesetEditor::on_comboBox_terrainType_activated(int terrainType)
{
    if (this->metatile) {
        Metatile *prevMetatile = this->metatile->copy();
        this->metatile->terrainType = static_cast<uint8_t>(terrainType);
        MetatileHistoryItem *commit = new MetatileHistoryItem(metatileSelector->getSelectedMetatile(), prevMetatile, this->metatile->copy());
        metatileHistory.push(commit);
    }
}

void TilesetEditor::on_actionSave_Tileset_triggered()
{
    saveMetatileLabel();

    this->project->saveTilesets(this->primaryTileset, this->secondaryTileset);
    emit this->tilesetsSaved(this->primaryTileset->name, this->secondaryTileset->name);
    if (this->paletteEditor) {
        this->paletteEditor->setTilesets(this->primaryTileset, this->secondaryTileset);
    }
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
                "Image Files (*.png *.bmp *.jpg *.dib)");
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
        msgBox.setInformativeText(QString("The provided image is not indexed. Please select a palette file to for the image. An indexed image will be generated using the provided image and palette.")
                                  .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Warning);
        msgBox.exec();

        QString filepath = QFileDialog::getOpenFileName(
            this,
            QString("Select Palette for Tiles Image").arg(descriptorCaps),
            this->project->root,
            "Palette Files (*.pal *.act *tpl *gpl)");
        if (filepath.isEmpty()) {
            return;
        }

        PaletteUtil parser;
        bool error = false;
        QList<QRgb> palette = parser.parse(filepath, &error);
        if (error) {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to import palette.");
            QString message = QString("The palette file could not be processed. View porymap.log for specific errors.");
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        } else if (palette.length() != 16) {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to import palette.");
            QString message = QString("The palette must have exactly 16 colors, but it has %1.").arg(palette.length());
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
    if (image.colorCount() != 16) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(QString("The image must be indexed and contain 16 total colors, or it must be un-indexed. The provided image has %1 indexed colors.")
                                  .arg(image.colorCount()));
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    this->project->loadTilesetTiles(tileset, image);
    this->refresh();
    this->hasUnsavedChanges = true;
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
            event->accept();
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
        }
    } else {
        event->accept();
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
            metatile->encounterType = 0;
            metatile->terrainType = 0;
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
            metatile->encounterType = 0;
            metatile->terrainType = 0;
            for (int i = 0; i < 8; i++) {
                metatile->tiles->append(tile);
            }
            this->secondaryTileset->metatiles->append(metatile);
        }

        this->refresh();
        this->hasUnsavedChanges = true;
    }
}

void TilesetEditor::on_actionChange_Palettes_triggered()
{
    if (!this->paletteEditor) {
        this->paletteEditor = new PaletteEditor(this->project, this->primaryTileset, this->secondaryTileset, this->paletteId, this);
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

void TilesetEditor::on_actionUndo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.current();
    if (!commit) return;
    Metatile *prev = commit->prevMetatile;
    if (!prev) return;
    this->metatileHistory.back();

    Metatile *temp = Tileset::getMetatile(commit->metatileId, this->primaryTileset, this->secondaryTileset);
    if (temp) {
        this->metatile = temp;
        this->metatile->copyInPlace(prev);
        this->metatileSelector->select(commit->metatileId);
        this->metatileSelector->draw();
        this->metatileLayersItem->draw();
        this->metatileLayersItem->clearLastModifiedCoords();
    }
}

void TilesetEditor::on_actionRedo_triggered()
{
    MetatileHistoryItem *commit = this->metatileHistory.next();
    if (!commit) return;
    Metatile *next = commit->newMetatile;
    if (!next) return;

    Metatile *temp = Tileset::getMetatile(commit->metatileId, this->primaryTileset, this->secondaryTileset);
    if (temp) {
        this->metatile = Tileset::getMetatile(commit->metatileId, this->primaryTileset, this->secondaryTileset);
        this->metatile->copyInPlace(next);
        this->metatileSelector->select(commit->metatileId);
        this->metatileSelector->draw();
        this->metatileLayersItem->draw();
        this->metatileLayersItem->clearLastModifiedCoords();
    }
}

void TilesetEditor::on_actionExport_Primary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->primaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->root).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Primary Tiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        QImage image = this->tileSelector->buildPrimaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
    }
}

void TilesetEditor::on_actionExport_Secondary_Tiles_Image_triggered()
{
    QString defaultName = QString("%1_Tiles_Pal%2").arg(this->secondaryTileset->name).arg(this->paletteId);
    QString defaultFilepath = QString("%1/%2.png").arg(this->project->root).arg(defaultName);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Secondary Tiles Image", defaultFilepath, "Image Files (*.png)");
    if (!filepath.isEmpty()) {
        QImage image = this->tileSelector->buildSecondaryTilesIndexedImage();
        exportIndexed4BPPPng(image, filepath);
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
                this->project->root,
                "Advance Map 1.92 Metatile Files (*.bvd)");
    if (filepath.isEmpty()) {
        return;
    }

    MetatileParser parser;
    bool error = false;
    QList<Metatile*> *metatiles = parser.parse(filepath, &error, primary);
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
    for (int i = 0; i < metatiles->length(); i++) {
        if (i >= tileset->metatiles->length()) {
            break;
        }

        Metatile *prevMetatile = tileset->metatiles->at(i)->copy();
        MetatileHistoryItem *commit = new MetatileHistoryItem(static_cast<uint16_t>(metatileIdBase + i), prevMetatile, metatiles->at(i)->copy());
        metatileHistory.push(commit);
    }

    tileset->metatiles = metatiles;
    this->refresh();
    this->hasUnsavedChanges = true;
}
