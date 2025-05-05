#include "newlocationdialog.h"
#include "ui_newlocationdialog.h"
#include "project.h"
#include "validator.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewLocationDialog::NewLocationDialog(Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewLocationDialog),
    namePrefix(projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix))
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;

    ui->lineEdit_IdName->setValidator(new IdentifierValidator(namePrefix, this));
    ui->lineEdit_IdName->setText(namePrefix);

    connect(ui->lineEdit_IdName, &QLineEdit::textChanged, this, &NewLocationDialog::onIdNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewLocationDialog::dialogButtonClicked);

    adjustSize();
}

NewLocationDialog::~NewLocationDialog()
{
    delete ui;
}

void NewLocationDialog::onIdNameChanged(const QString &idName) {
    validateIdName(true);

    // Extract a presumed display name from the ID name
    QString displayName = idName;
    if (displayName.startsWith(namePrefix))
        displayName.remove(0, namePrefix.length());
    displayName.replace("_", " ");
    ui->lineEdit_DisplayName->setText(displayName);
}

bool NewLocationDialog::validateIdName(bool allowEmpty) {
    const QString name = ui->lineEdit_IdName->text();

    QString errorText;
    if (name.isEmpty() || name == namePrefix) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_IdName->text());
    } else if (!this->project->isIdentifierUnique(name)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_IdName->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_IdNameError->setText(errorText);
    ui->label_IdNameError->setVisible(!isValid);
    ui->lineEdit_IdName->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewLocationDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewLocationDialog::accept() {
    if (!validateIdName())
        return;

    this->project->addNewMapsec(ui->lineEdit_IdName->text(), ui->lineEdit_DisplayName->text());

    QDialog::accept();
}
