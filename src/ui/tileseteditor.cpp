#include "tileseteditor.h"
#include "ui_tileseteditor.h"

TilesetEditor::TilesetEditor(QWidget *parent, Project *project) :
    QMainWindow(parent),
    ui(new Ui::TilesetEditor)
{
    ui->setupUi(this);
    this->project = project;

    displayPrimaryTilesetTiles();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
}

void TilesetEditor::displayPrimaryTilesetTiles()
{
    Tileset *primaryTileset = this->project->getTileset(this->primaryTilesetLabel);
//    scene_metatiles = new QGraphicsScene;
//    metatiles_item = new MetatilesPixmapItem(map);
//    metatiles_item->draw();
//    scene_metatiles->addItem(metatiles_item);
}
