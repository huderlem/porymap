#include "newnamedialog.h"
#include "ui_newnamedialog.h"
#include "project.h"
#include "imageexport.h"
#include "validator.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewNameDialog::NewNameDialog(const QString &label, const QString &prefix, Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewNameDialog),
    namePrefix(prefix)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;

    if (!label.isEmpty())
        ui->label_Name->setText(label);

    ui->lineEdit_Name->setValidator(new IdentifierValidator(namePrefix, this));
    ui->lineEdit_Name->setText(namePrefix);

    connect(ui->lineEdit_Name, &QLineEdit::textChanged, this, &NewNameDialog::onNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewNameDialog::dialogButtonClicked);

    adjustSize();
}

NewNameDialog::~NewNameDialog()
{
    delete ui;
}

void NewNameDialog::onNameChanged(const QString &) {
    validateName(true);
}

bool NewNameDialog::validateName(bool allowEmpty) {
    const QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty() || name == namePrefix) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_Name->text());
    } else if (this->project && !this->project->isIdentifierUnique(name)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_Name->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    ui->lineEdit_Name->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewNameDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewNameDialog::accept() {
    if (!validateName())
        return;

    emit applied(ui->lineEdit_Name->text());
    QDialog::accept();
}
