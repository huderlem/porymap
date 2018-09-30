#include "tileseteditor.h"
#include "ui_tileseteditor.h"

TilesetEditor::TilesetEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TilesetEditor)
{
    ui->setupUi(this);
}

TilesetEditor::~TilesetEditor()
{
    delete ui;
}
