#include "newmapgroupdialog.h"
#include "ui_newmapgroupdialog.h"
#include "project.h"
#include "validator.h"

NewMapGroupDialog::NewMapGroupDialog(Project* project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMapGroupDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;

    ui->lineEdit_Name->setValidator(new IdentifierValidator(this));
    ui->lineEdit_Name->setText(Project::getMapGroupPrefix());

    connect(ui->lineEdit_Name, &QLineEdit::textChanged, this, &NewMapGroupDialog::onNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapGroupDialog::dialogButtonClicked);

    adjustSize();
}

NewMapGroupDialog::~NewMapGroupDialog()
{
    delete ui;
}

void NewMapGroupDialog::onNameChanged(const QString &) {
    validateName(true);
}

bool NewMapGroupDialog::validateName(bool allowEmpty) {
    const QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_Name->text());
    } else if (!this->project->isIdentifierUnique(name)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_Name->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    Util::setErrorStylesheet(ui->lineEdit_Name, !isValid);
    return isValid;
}

void NewMapGroupDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewMapGroupDialog::accept() {
    if (!validateName())
        return;

    this->project->addNewMapGroup(ui->lineEdit_Name->text());

    QDialog::accept();
}
