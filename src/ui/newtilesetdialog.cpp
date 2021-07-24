#include "newtilesetdialog.h"
#include "ui_newtilesetdialog.h"
#include <QFileDialog>
#include "project.h"

NewTilesetDialog::NewTilesetDialog(Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTilesetDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());
    this->project = project;
    //only allow characters valid for a symbol
    QRegularExpression expression("[_A-Za-z0-9]+$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    this->ui->nameLineEdit->setValidator(validator);

    connect(this->ui->nameLineEdit, &QLineEdit::textChanged, this, &NewTilesetDialog::NameOrSecondaryChanged);
    connect(this->ui->typeComboBox, &QComboBox::currentTextChanged, this, &NewTilesetDialog::SecondaryChanged);
    //connect(this->ui->toolButton, &QToolButton::clicked, this, &NewTilesetDialog::ChangeFilePath);
    this->SecondaryChanged();
}

NewTilesetDialog::~NewTilesetDialog()
{
    delete ui;
}

void NewTilesetDialog::SecondaryChanged(){
    this->isSecondary = (this->ui->typeComboBox->currentIndex() == 1);
    NameOrSecondaryChanged();
}

void NewTilesetDialog::NameOrSecondaryChanged() {
    this->friendlyName = this->ui->nameLineEdit->text();
    this->fullSymbolName = "gTileset_" + this->friendlyName;
    this->ui->symbolNameLineEdit->setText(this->fullSymbolName);
    this->path = QString("/data/tilesets/") + (this->isSecondary ? "secondary/" : "primary/") + this->friendlyName.toLower();
    this->ui->pathLineEdit->setText(this->path);
}
