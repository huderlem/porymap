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
    static const QRegularExpression expression("[_A-Za-z0-9]+$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    this->ui->nameLineEdit->setValidator(validator);

    bool checkerboard = porymapConfig.getTilesetCheckerboardFill();
    this->ui->fillCheckBox->setChecked(checkerboard);
    this->checkerboardFill = checkerboard;

    connect(this->ui->nameLineEdit, &QLineEdit::textChanged, this, &NewTilesetDialog::NameOrSecondaryChanged);
    connect(this->ui->typeComboBox, &QComboBox::currentTextChanged, this, &NewTilesetDialog::SecondaryChanged);
    connect(this->ui->fillCheckBox, &QCheckBox::stateChanged, this, &NewTilesetDialog::FillChanged);
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
    this->fullSymbolName = projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix) + this->friendlyName;
    this->ui->symbolNameLineEdit->setText(this->fullSymbolName);
    this->path = Tileset::getExpectedDir(this->fullSymbolName, this->isSecondary);
    this->ui->pathLineEdit->setText(this->path);
}

void NewTilesetDialog::FillChanged() {
    this->checkerboardFill = this->ui->fillCheckBox->isChecked();
    porymapConfig.setTilesetCheckerboardFill(this->checkerboardFill);
}
