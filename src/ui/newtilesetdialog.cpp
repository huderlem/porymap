#include "newtilesetdialog.h"
#include "ui_newtilesetdialog.h"
#include "project.h"
#include "imageexport.h"
#include "validator.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewTilesetDialog::NewTilesetDialog(Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTilesetDialog),
    symbolPrefix(projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix))
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;

    ui->checkBox_CheckerboardFill->setChecked(porymapConfig.tilesetCheckerboardFill);
    ui->comboBox_Type->setMinimumContentsLength(12 + this->symbolPrefix.length());
    ui->lineEdit_Name->setValidator(new IdentifierValidator(this->symbolPrefix, this));
    ui->lineEdit_Name->setText(this->symbolPrefix);

    connect(ui->lineEdit_Name, &QLineEdit::textChanged, this, &NewTilesetDialog::onNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewTilesetDialog::dialogButtonClicked);

    adjustSize();
}

NewTilesetDialog::~NewTilesetDialog()
{
    porymapConfig.tilesetCheckerboardFill = ui->checkBox_CheckerboardFill->isChecked();
    delete ui;
}

void NewTilesetDialog::onNameChanged(const QString &) {
    validateName(true);
}

bool NewTilesetDialog::validateName(bool allowEmpty) {
    const QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty() || name == symbolPrefix) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_Name->text());
    } else if (!this->project->isIdentifierUnique(name)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_Name->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    ui->lineEdit_Name->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewTilesetDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewTilesetDialog::accept() {
    if (!validateName())
        return;

    bool secondary = ui->comboBox_Type->currentIndex() == 1;
    Tileset *tileset = this->project->createNewTileset(ui->lineEdit_Name->text(), secondary, ui->checkBox_CheckerboardFill->isChecked());
    if (!tileset) {
        ui->label_GenericError->setText(QString("Failed to create tileset. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);

    QDialog::accept();
    emit applied(tileset);
}
