#include "newlayoutdialog.h"
#include "maplayout.h"
#include "ui_newlayoutdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

Layout::Settings NewLayoutDialog::settings = {};
bool NewLayoutDialog::initializedSettings = false;

NewLayoutDialog::NewLayoutDialog(Project *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewLayoutDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    ui->label_GenericError->setVisible(false);
    this->project = project;

    Layout::Settings newSettings = project->getNewLayoutSettings();
    if (!initializedSettings) {
        // The first time this dialog is opened we initialize all the default settings.
        settings = newSettings;
        initializedSettings = true;
    } else {
        // On subsequent openings we only initialize the settings that should be unique,
        // preserving all other settings from the last time the dialog was open.
        settings.name = newSettings.name;
        settings.id = newSettings.id;
    }
    ui->newLayoutForm->initUi(project);

    // Identifiers can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_LayoutID->setValidator(validator);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewLayoutDialog::dialogButtonClicked);

    refresh();
    adjustSize();
}

// Creating new layout from an existing layout (e.g. via AdvanceMap import, or duplicating from map list).
NewLayoutDialog::NewLayoutDialog(Project *project, const Layout *layoutToCopy, QWidget *parent) :
    NewLayoutDialog(project, parent)
{
    if (!layoutToCopy)
        return;

    this->importedLayout = layoutToCopy->copy();
    if (!this->importedLayout->name.isEmpty()) {
        // If the layout we're duplicating has a name and ID we'll initialize the name/ID fields
        // using that name and add a suffix to make it unique.
        // Layouts imported with AdvanceMap won't have a name/ID.
        int i = 2;
        do {
            settings.name = QString("%1_%2").arg(this->importedLayout->name).arg(i);
            settings.id = QString("%1_%2").arg(this->importedLayout->id).arg(i);
            i++;
        } while (!this->project->isIdentifierUnique(settings.name) || !this->project->isIdentifierUnique(settings.id));
    }
    refresh();
}

NewLayoutDialog::~NewLayoutDialog()
{
    saveSettings();
    delete this->importedLayout;
    delete ui;
}

void NewLayoutDialog::refresh() {
    if (this->importedLayout) {
        // If we're importing a layout then some settings will be enforced.
        ui->newLayoutForm->setSettings(this->importedLayout->settings());
        ui->newLayoutForm->setDisabled(true);
    } else {
        ui->newLayoutForm->setSettings(settings);
        ui->newLayoutForm->setDisabled(false);
    }

    ui->lineEdit_Name->setText(settings.name);
    ui->lineEdit_LayoutID->setText(settings.id);
}

void NewLayoutDialog::saveSettings() {
    settings = ui->newLayoutForm->settings();
    settings.id = ui->lineEdit_LayoutID->text();
    settings.name = ui->lineEdit_Name->text();
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
        settings = this->project->getNewLayoutSettings();
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

    Layout *layout = this->project->createNewLayout(settings, this->importedLayout);
    if (!layout) {
        ui->label_GenericError->setText(QString("Failed to create layout. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);

    // TODO: See if we can get away with emitting this from Project so that we don't need to connect
    //       to this signal every time we create the dialog.
    emit applied(layout->id);
    QDialog::accept();
}
