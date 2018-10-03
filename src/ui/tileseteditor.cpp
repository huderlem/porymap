#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include "imageproviders.h"
#include <QDebug>

TilesetEditor::TilesetEditor(Project *project, QString primaryTilesetLabel, QString secondaryTilesetLabel, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor)
{
    ui->setupUi(this);
    this->project = project;

    this->tileXFlip = ui->checkBox_xFlip->isChecked();
    this->tileYFlip = ui->checkBox_yFlip->isChecked();
    this->paletteId = ui->spinBox_paletteSelector->value();

    Tileset *primaryTileset = project->getTileset(primaryTilesetLabel);
    Tileset *secondaryTileset = project->getTileset(secondaryTilesetLabel);
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

TilesetEditor::~TilesetEditor()
{
    delete ui;
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
    connect(this->tileSelector, SIGNAL(selectedTileChanged(uint16_t)),
            this, SLOT(onSelectedTileChanged(uint16_t)));

    this->tilesScene = new QGraphicsScene;
    this->tilesScene->addItem(this->tileSelector);
    this->tileSelector->select(0);
    this->tileSelector->draw();

    this->ui->graphicsView_Tiles->setScene(this->tilesScene);
    this->ui->graphicsView_Tiles->setFixedSize(this->tileSelector->pixmap().width() + 2, this->tileSelector->pixmap().height() + 2);
}

void TilesetEditor::initSelectedTileItem() {
    this->selectedTileScene = new QGraphicsScene;
    this->drawSelectedTile();
    this->ui->graphicsView_selectedTile->setScene(this->selectedTileScene);
}

void TilesetEditor::drawSelectedTile() {
    if (!this->selectedTileScene) {
        return;
    }

    this->selectedTileScene->clear();
    QImage tileImage = getColoredTileImage(this->tileSelector->getSelectedTile(), this->primaryTileset, this->secondaryTileset, this->paletteId)
            .mirrored(this->tileXFlip, this->tileYFlip);
    this->selectedTilePixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(tileImage).scaled(32, 32));
    this->selectedTileScene->addItem(this->selectedTilePixmapItem);
}

void TilesetEditor::initMetatileLayersItem() {
    Metatile *metatile = Tileset::getMetatile(this->metatileSelector->getSelectedMetatile(), this->primaryTileset, this->secondaryTileset);
    this->metatileLayersItem = new MetatileLayersItem(metatile, this->primaryTileset, this->secondaryTileset);
    connect(this->metatileLayersItem, SIGNAL(tileChanged(int)),
            this, SLOT(onMetatileLayerTileChanged(int)));

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

void TilesetEditor::onSelectedTileChanged(uint16_t) {    
    this->drawSelectedTile();
}

void TilesetEditor::onMetatileLayerTileChanged(int tileIndex) {
    Tile tile = this->metatile->tiles->at(tileIndex);
    tile.tile = this->tileSelector->getSelectedTile();
    tile.xflip = this->tileXFlip;
    tile.yflip = this->tileYFlip;
    tile.palette = this->paletteId;
    (*this->metatile->tiles)[tileIndex] = tile;
    this->metatileSelector->draw();
    this->metatileLayersItem->draw();
}

void TilesetEditor::on_spinBox_paletteSelector_valueChanged(int paletteId)
{
    this->paletteId = paletteId;
    this->tileSelector->setPaletteId(paletteId);
    this->drawSelectedTile();
}

void TilesetEditor::on_checkBox_xFlip_stateChanged(int checked)
{
    this->tileXFlip = checked;
    this->drawSelectedTile();
}

void TilesetEditor::on_checkBox_yFlip_stateChanged(int checked)
{
    this->tileYFlip = checked;
    this->drawSelectedTile();
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
}
