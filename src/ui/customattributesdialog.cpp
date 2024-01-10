#include "customattributesdialog.h"
#include "ui_customattributesdialog.h"

#include <QMessageBox>

// Preserve type selection for later dialogs
static int typeIndex = 0;

CustomAttributesDialog::CustomAttributesDialog(CustomAttributesTable *parent) :
    QDialog(parent),
    ui(new Ui::CustomAttributesDialog),
    table(parent)
{
    ui->setupUi(this);

    // Type combo box
    static const QStringList types = {"String", "Number", "Boolean"};
    ui->comboBox_Type->addItems(types);
    ui->comboBox_Type->setEditable(false);
    connect(ui->comboBox_Type, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        // When the value type is changed, update the value input widget
        if (index >= 0 && index < types.length()){
            ui->stackedWidget_Value->setCurrentIndex(index);
            typeIndex = index;
        }
    });
    ui->comboBox_Type->setCurrentIndex(typeIndex);

    // Clear the red highlight that may be set if user tries to finish with an invalid key name
    connect(ui->lineEdit_Name, &QLineEdit::textChanged, [this](QString) {
        this->setNameEditHighlight(false);
    });

    // Button box
    connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton *button) {
        auto buttonRole = ui->buttonBox->buttonRole(button);
        if (buttonRole == QDialogButtonBox::AcceptRole && this->verifyName()) {
            this->addNewAttribute();
            this->done(QDialog::Accepted);
        } else if (buttonRole == QDialogButtonBox::RejectRole) {
            this->done(QDialog::Rejected);
        }
    });
}

CustomAttributesDialog::~CustomAttributesDialog() {
    delete ui;
}

void CustomAttributesDialog::setNameEditHighlight(bool highlight) {
    ui->lineEdit_Name->setStyleSheet(highlight ? "QLineEdit { background-color: rgba(255, 0, 0, 25%) }" : "");
}

bool CustomAttributesDialog::verifyName() {
    const QString name = ui->lineEdit_Name->text();

    if (name.isEmpty()) {
        this->setNameEditHighlight(true);
        return false;
    }

    // Invalidate name if it would collide with an expected JSON field name
    if (this->table->restrictedKeyNames.contains(name)) {
        const QString msg = QString("The name '%1' is reserved, please choose a different name.").arg(name);
        QMessageBox::critical(this, "Error", msg);
        this->setNameEditHighlight(true);
        return false;
    }

    // Warn user if key name would overwrite an existing custom attribute
    if (this->table->getAttributes().keys().contains(name)) {
        const QString msg = QString("Overwrite value for existing attribute '%1'?").arg(name);
        if (QMessageBox::warning(this, "Warning", msg, QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
            return false;
    }

    return true;
}

void CustomAttributesDialog::addNewAttribute() {
    QJsonValue value;
    const QString type = ui->comboBox_Type->currentText();
    if (type == "String") {
        value = QJsonValue(ui->lineEdit_Value->text());
    } else if (type == "Number") {
        value = QJsonValue(ui->spinBox_Value->value());
    } else if (type == "Boolean") {
        value = QJsonValue(ui->checkBox_Value->isChecked());
    }

    const QString key = ui->lineEdit_Name->text();
    this->table->addNewAttribute(key, value);

    if (ui->checkBox_Default->isChecked())
        this->table->setDefaultAttribute(key, value);
}
