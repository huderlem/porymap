#include "newtilesetdialog.h"
#include "ui_newtilesetdialog.h"
#include "project.h"
#include "imageexport.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewTilesetDialog::NewTilesetDialog(Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTilesetDialog),
    symbolPrefix(projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    this->project = project;

    ui->checkBox_CheckerboardFill->setChecked(porymapConfig.tilesetCheckerboardFill);
    ui->label_SymbolNameDisplay->setText(this->symbolPrefix);
    ui->comboBox_Type->setMinimumContentsLength(12);

    //only allow characters valid for a symbol
    static const QRegularExpression expression("[A-Za-z_]+[\\w]*");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression, this);
    ui->lineEdit_FriendlyName->setValidator(validator);

    connect(ui->lineEdit_FriendlyName, &QLineEdit::textChanged, this, &NewTilesetDialog::onFriendlyNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewTilesetDialog::dialogButtonClicked);

    adjustSize();
}

NewTilesetDialog::~NewTilesetDialog()
{
    porymapConfig.tilesetCheckerboardFill = ui->checkBox_CheckerboardFill->isChecked();
    delete ui;
}

void NewTilesetDialog::onFriendlyNameChanged(const QString &friendlyName) {
    // When the tileset name is changed, update this label to display the full symbol name.
    ui->label_SymbolNameDisplay->setText(this->symbolPrefix + friendlyName);

    validateName(true);
}

bool NewTilesetDialog::validateName(bool allowEmpty) {
    const QString friendlyName = ui->lineEdit_FriendlyName->text();
    const QString symbolName = ui->label_SymbolNameDisplay->text();

    QString errorText;
    if (friendlyName.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_FriendlyName->text());
    } else if (!this->project->isIdentifierUnique(symbolName)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_SymbolName->text()).arg(symbolName);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    ui->lineEdit_FriendlyName->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
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
    Tileset *tileset = this->project->createNewTileset(ui->lineEdit_FriendlyName->text(), secondary, ui->checkBox_CheckerboardFill->isChecked());
    if (!tileset) {
        ui->label_GenericError->setText(QString("Failed to create tileset. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);

    QDialog::accept();
}
