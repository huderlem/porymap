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

    initMetatilesSelector();
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
}

void TilesetEditor::initMetatilesSelector()
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
