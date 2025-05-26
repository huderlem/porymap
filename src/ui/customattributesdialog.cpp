#include "customattributesdialog.h"
#include "ui_customattributesdialog.h"
#include "utility.h"

#include <QMessageBox>

static int curInputType = 0;

CustomAttributesDialog::CustomAttributesDialog(CustomAttributesTable *table) :
    QDialog(table),
    ui(new Ui::CustomAttributesDialog),
    m_table(table)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    // Type combo box
    ui->comboBox_Type->addItems({"String", "Number", "Boolean"});
    ui->comboBox_Type->setEditable(false);

    // When the value type is changed, update the value input widget
    connect(ui->comboBox_Type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CustomAttributesDialog::setInputType);
    ui->comboBox_Type->setCurrentIndex(curInputType);

    ui->spinBox_Value->setMinimum(INT_MIN);
    ui->spinBox_Value->setMaximum(INT_MAX);

    connect(ui->lineEdit_Name, &QLineEdit::textChanged, this, &CustomAttributesDialog::onNameChanged);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &CustomAttributesDialog::clickedButton);

    adjustSize();
}

CustomAttributesDialog::~CustomAttributesDialog() {
    delete ui;
}

void CustomAttributesDialog::setInputType(int inputType) {
    if (inputType < 0 || inputType >= ui->stackedWidget_Value->count())
        return;

    ui->stackedWidget_Value->setCurrentIndex(inputType);

    // Preserve input widget for later dialogs
    curInputType = inputType;
}

void CustomAttributesDialog::onNameChanged(const QString &) {
    validateName(true);
}

bool CustomAttributesDialog::validateName(bool allowEmpty) {
    const QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_Name->text());
    } else if (m_table->restrictedKeys().contains(name)) {
        errorText = QString("The name '%1' is reserved, please choose a different name.").arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    Util::setErrorStylesheet(ui->lineEdit_Name, !isValid);
    return isValid;
}

QVariant CustomAttributesDialog::getValue() const {
    QVariant value;
    auto widget = ui->stackedWidget_Value->currentWidget();
    if (widget == ui->page_String) {
        value = QVariant(ui->lineEdit_Value->text());
    } else if (widget == ui->page_Number) {
        value = QVariant(ui->spinBox_Value->value());
    } else if (widget == ui->page_Boolean) {
        value = QVariant(ui->checkBox_Value->isChecked());
    }
    return value;
}

void CustomAttributesDialog::addNewAttribute() {
    m_table->addNewAttribute(ui->lineEdit_Name->text(), QJsonValue::fromVariant(getValue()));
}

void CustomAttributesDialog::clickedButton(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole && validateName()) {
        const QString key = ui->lineEdit_Name->text();
        if (m_table->keys().contains(key)) {
            // Warn user if key name would overwrite an existing custom attribute
            const QString msg = QString("Overwrite value for existing attribute '%1'?").arg(key);
            if (QMessageBox::warning(this, "Warning", msg, QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel){
                return;
            }
        }
        addNewAttribute();
        done(QDialog::Accepted);
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        done(QDialog::Rejected);
    }
}
