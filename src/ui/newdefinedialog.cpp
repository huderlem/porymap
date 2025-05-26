#include "newdefinedialog.h"
#include "ui_newdefinedialog.h"
#include "validator.h"
#include "utility.h"

NewDefineDialog::NewDefineDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewDefineDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    ui->lineEdit_Name->setValidator(new IdentifierValidator(this));

    connect(ui->lineEdit_Name, &QLineEdit::textChanged, this, &NewDefineDialog::onNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewDefineDialog::dialogButtonClicked);

    adjustSize();
}

NewDefineDialog::~NewDefineDialog()
{
    delete ui;
}

void NewDefineDialog::onNameChanged(const QString &) {
    validateName(true);
}

bool NewDefineDialog::validateName(bool allowEmpty) {
    const QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty() && !allowEmpty) {
        errorText = QString("%1 cannot be empty.").arg(ui->label_Name->text());
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    Util::setErrorStylesheet(ui->lineEdit_Name, !isValid);
    return isValid;
}

void NewDefineDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewDefineDialog::accept() {
    if (!validateName())
        return;
    emit createdDefine(ui->lineEdit_Name->text(), ui->lineEdit_Value->text());
    QDialog::accept();
}
