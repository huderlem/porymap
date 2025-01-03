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
    ui->comboBox_Type->addItems({"String", "Number", "Boolean"});
    ui->comboBox_Type->setEditable(false);
    connect(ui->comboBox_Type, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        // When the value type is changed, update the value input widget
        if (index >= 0 && index < ui->stackedWidget_Value->count()){
            ui->stackedWidget_Value->setCurrentIndex(index);
            typeIndex = index;
        }
    });
    ui->comboBox_Type->setCurrentIndex(typeIndex);

    // Clear the red highlight that may be set if user tries to finish with an invalid key name
    connect(ui->lineEdit_Name, &QLineEdit::textChanged, [this](QString) {
        this->setNameEditHighlight(false);
    });

    // Exclude delimiters used in the config
    static const QRegularExpression expression("[^:,=/]*");
    ui->lineEdit_Name->setValidator(new QRegularExpressionValidator(expression));

    // Button box
    connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton *button) {
        auto buttonRole = ui->buttonBox->buttonRole(button);
        if (buttonRole == QDialogButtonBox::AcceptRole && this->verifyInput()) {
            this->addNewAttribute();
            this->done(QDialog::Accepted);
        } else if (buttonRole == QDialogButtonBox::RejectRole) {
            this->done(QDialog::Rejected);
        }
    });

    ui->spinBox_Value->setMinimum(INT_MIN);
    ui->spinBox_Value->setMaximum(INT_MAX);
}

CustomAttributesDialog::~CustomAttributesDialog() {
    delete ui;
}

void CustomAttributesDialog::setNameEditHighlight(bool highlight) {
    ui->lineEdit_Name->setStyleSheet(highlight ? "QLineEdit { background-color: rgba(255, 0, 0, 25%) }" : "");
}

bool CustomAttributesDialog::verifyInput() {
    const QString name = ui->lineEdit_Name->text();

    if (name.isEmpty()) {
        this->setNameEditHighlight(true);
        return false;
    }

    // Invalidate name if it would collide with an expected JSON field name
    if (this->table->restrictedKeys().contains(name)) {
        const QString msg = QString("The name '%1' is reserved, please choose a different name.").arg(name);
        QMessageBox::critical(this, "Error", msg);
        this->setNameEditHighlight(true);
        return false;
    }

    // Warn user if changing the default value of a "Default" custom attribute
    if (this->table->defaultKeys().contains(name) && ui->checkBox_Default->isChecked()) {
        const QString msg = QString("'%1' is already a default attribute. Replace its default value with '%2'?")
                                    .arg(name)
                                    .arg(this->getValue().toString());
        if (QMessageBox::warning(this, "Warning", msg, QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
            return false;
    }
    // Warn user if key name would overwrite an existing custom attribute
    else if (this->table->keys().contains(name)) {
        const QString msg = QString("Overwrite value for existing attribute '%1'?").arg(name);
        if (QMessageBox::warning(this, "Warning", msg, QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Cancel)
            return false;
    }

    return true;
}

// For initializing the dialog with a state other than the default
void CustomAttributesDialog::setInput(const QString &key, QJsonValue value, bool setDefault) {
    ui->lineEdit_Name->setText(key);
    ui->checkBox_Default->setChecked(setDefault);

    auto type = value.type();
    if (type == QJsonValue::Type::String) {
        ui->lineEdit_Value->setText(value.toString());
        ui->stackedWidget_Value->setCurrentWidget(ui->page_String);
    } else if (type == QJsonValue::Type::Double) {
        ui->spinBox_Value->setValue(value.toInt());
        ui->stackedWidget_Value->setCurrentWidget(ui->page_Number);
    } else if (type == QJsonValue::Type::Bool) {
        ui->checkBox_Value->setChecked(value.toBool());
        ui->stackedWidget_Value->setCurrentWidget(ui->page_Boolean);
    }
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
    this->table->addNewAttribute(ui->lineEdit_Name->text(), QJsonValue::fromVariant(this->getValue()), ui->checkBox_Default->isChecked());
}
