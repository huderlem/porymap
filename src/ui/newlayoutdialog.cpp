#include "newlayoutdialog.h"
#include "maplayout.h"
#include "ui_newlayoutdialog.h"
#include "config.h"
#include "validator.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewLayoutDialog::NewLayoutDialog(Project *project, QWidget *parent) :
    NewLayoutDialog(project, nullptr, parent)
{}

NewLayoutDialog::NewLayoutDialog(Project *project, const Layout *layoutToCopy, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewLayoutDialog),
    layoutToCopy(layoutToCopy)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;
    Layout::Settings *settings = &project->newLayoutSettings;

    // Note: 'layoutToCopy' will have an empty name if it's an import from AdvanceMap
    if (this->layoutToCopy && !this->layoutToCopy->name.isEmpty()) {
        settings->name = project->toUniqueIdentifier(this->layoutToCopy->name);
    } else {
        settings->name = project->getNewLayoutName();
    }
    // Generate a unique Layout constant
    settings->id = project->toUniqueIdentifier(Layout::layoutConstantFromName(settings->name));

    ui->newLayoutForm->initUi(project);

    auto validator = new IdentifierValidator(this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_LayoutID->setValidator(validator);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewLayoutDialog::dialogButtonClicked);

    refresh();
    adjustSize();
}

NewLayoutDialog::~NewLayoutDialog()
{
    saveSettings();
    delete ui;
}

void NewLayoutDialog::refresh() {
    const Layout::Settings *settings = &this->project->newLayoutSettings;

    if (this->layoutToCopy) {
        // If we're importing a layout then some settings will be enforced.
        ui->newLayoutForm->setSettings(this->layoutToCopy->settings());
        ui->newLayoutForm->setDisabled(true);
    } else {
        ui->newLayoutForm->setSettings(*settings);
        ui->newLayoutForm->setDisabled(false);
    }

    ui->lineEdit_Name->setText(settings->name);
    ui->lineEdit_LayoutID->setText(settings->id);
}

void NewLayoutDialog::saveSettings() {
    Layout::Settings *settings = &this->project->newLayoutSettings;

    *settings = ui->newLayoutForm->settings();
    settings->id = ui->lineEdit_LayoutID->text();
    settings->name = ui->lineEdit_Name->text();
}

bool NewLayoutDialog::validateLayoutID(bool allowEmpty) {
    QString id = ui->lineEdit_LayoutID->text();

    QString errorText;
    if (id.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_LayoutID->text());
    } else if (!this->project->isIdentifierUnique(id)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_LayoutID->text()).arg(id);
    }

    bool isValid = errorText.isEmpty();
    ui->label_LayoutIDError->setText(errorText);
    ui->label_LayoutIDError->setVisible(!isValid);
    ui->lineEdit_LayoutID->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewLayoutDialog::on_lineEdit_LayoutID_textChanged(const QString &) {
    validateLayoutID(true);
}

bool NewLayoutDialog::validateName(bool allowEmpty) {
    QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (name.isEmpty()) {
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

void NewLayoutDialog::on_lineEdit_Name_textChanged(const QString &text) {
    validateName(true);
    
    // Changing the layout name updates the layout ID field to match.
    ui->lineEdit_LayoutID->setText(Layout::layoutConstantFromName(text));
}

void NewLayoutDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        this->project->initNewLayoutSettings();
        refresh();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewLayoutDialog::accept() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!ui->newLayoutForm->validate()) success = false;
    if (!validateLayoutID()) success = false;
    if (!validateName()) success = false;
    if (!success)
        return;

    // Update settings from UI
    saveSettings();

    Layout *layout = this->project->createNewLayout(this->project->newLayoutSettings, this->layoutToCopy);
    if (!layout) {
        ui->label_GenericError->setText(QString("Failed to create layout. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);

    emit applied(layout->id);
    QDialog::accept();
}
