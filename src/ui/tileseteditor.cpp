#include "tileseteditor.h"
#include "ui_tileseteditor.h"
#include <QDebug>

TilesetEditor::TilesetEditor(Project *project, QString primaryTilesetLabel, QString secondaryTilesetLabel, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor)
{
    ui->setupUi(this);
    this->project = project;

    this->primaryTilesetLabel = primaryTilesetLabel;
    this->secondaryTilesetLabel = secondaryTilesetLabel;

    initMetatileSelector();
    initTileSelector();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
}

void TilesetEditor::initMetatileSelector()
{
    Tileset *primaryTileset = this->project->getTileset(this->primaryTilesetLabel);
    Tileset *secondaryTileset = this->project->getTileset(this->secondaryTilesetLabel);
    this->metatileSelector = new TilesetEditorMetatileSelector(primaryTileset, secondaryTileset);
    connect(this->metatileSelector, SIGNAL(hoveredMetatileChanged(uint16_t)),
            this, SLOT(onHoveredMetatileChanged(uint16_t)));
    connect(this->metatileSelector, SIGNAL(hoveredMetatileCleared()),
            this, SLOT(onHoveredMetatileCleared()));
    connect(this->metatileSelector, SIGNAL(selectedMetatileChanged(uint16_t)),
            this, SLOT(onSelectedMetatileChanged(uint16_t)));

    this->metatilesScene = new QGraphicsScene;
    this->metatilesScene->addItem(this->metatileSelector);
    this->metatileSelector->select(0);
    this->metatileSelector->draw();

    this->ui->graphicsView_Metatiles->setScene(this->metatilesScene);
    this->ui->graphicsView_Metatiles->setFixedSize(this->metatileSelector->pixmap().width() + 2, this->metatileSelector->pixmap().height() + 2);
}

void TilesetEditor::initTileSelector()
{
    Tileset *primaryTileset = this->project->getTileset(this->primaryTilesetLabel);
    Tileset *secondaryTileset = this->project->getTileset(this->secondaryTilesetLabel);
    this->tileSelector = new TilesetEditorTileSelector(primaryTileset, secondaryTileset);
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

void TilesetEditor::onHoveredMetatileChanged(uint16_t metatileId) {
    QString message = QString("Metatile: 0x%1")
                        .arg(QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper());
    this->ui->statusbar->showMessage(message);
}

void TilesetEditor::onHoveredMetatileCleared() {
    this->ui->statusbar->clearMessage();
}

void TilesetEditor::onSelectedMetatileChanged(uint16_t) {

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

}
