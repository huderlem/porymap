#include "newlayoutdialog.h"
#include "maplayout.h"
#include "ui_newlayoutdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewLayoutDialog::NewLayoutDialog(QWidget *parent, Project *project) :
    QDialog(parent),
    ui(new Ui::NewLayoutDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    this->project = project;
    this->settings = &project->newMapSettings.layout;

    ui->lineEdit_Name->setText(project->getNewLayoutName());

    ui->newLayoutForm->initUi(project);
    ui->newLayoutForm->setSettings(*this->settings);

    // Names and IDs can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_LayoutID->setValidator(validator);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewLayoutDialog::dialogButtonClicked);
    adjustSize();
}

NewLayoutDialog::~NewLayoutDialog()
{
    saveSettings();
    delete this->importedLayout;
    delete ui;
}

// Creating new map from AdvanceMap import
// TODO: Re-use for a "Duplicate Layout" option?
void NewLayoutDialog::init(Layout *layoutToCopy) {
    if (this->importedLayout)
        delete this->importedLayout;

    this->importedLayout = new Layout();
    this->importedLayout->blockdata = layoutToCopy->blockdata;
    if (!layoutToCopy->border.isEmpty())
        this->importedLayout->border = layoutToCopy->border;

    useLayoutSettings(this->importedLayout);
}

void NewLayoutDialog::saveSettings() {
    *this->settings = ui->newLayoutForm->settings();
    this->settings->id = ui->lineEdit_LayoutID->text();
    this->settings->name = ui->lineEdit_Name->text();
}

void NewLayoutDialog::useLayoutSettings(Layout *layout) {
    if (!layout) return;
    this->settings->width = layout->width;
    this->settings->height = layout->height;
    this->settings->borderWidth = layout->border_width;
    this->settings->borderHeight = layout->border_height;
    this->settings->primaryTilesetLabel = layout->tileset_primary_label;
    this->settings->secondaryTilesetLabel = layout->tileset_secondary_label;
    ui->newLayoutForm->setSettings(*this->settings);

    // Don't allow changes to the layout settings
    ui->newLayoutForm->setDisabled(true);
}

bool NewLayoutDialog::validateLayoutID(bool allowEmpty) {
    QString id = ui->lineEdit_LayoutID->text();

    QString errorText;
    if (id.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_LayoutID->text());
    } else if (this->project->mapLayouts.contains(id)) {
        errorText = QString("%1 '%2' is already in use.").arg(ui->label_LayoutID->text()).arg(id);
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
    } else if (!this->project->isLayoutNameUnique(name)) {
        errorText = QString("%1 '%2' is already in use.").arg(ui->label_Name->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    ui->lineEdit_Name->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewLayoutDialog::on_lineEdit_Name_textChanged(const QString &text) {
    validateName(true);
    ui->lineEdit_LayoutID->setText(Layout::layoutConstantFromName(text));
}

void NewLayoutDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        this->project->initNewLayoutSettings(); // TODO: Don't allow this to change locked settings
        ui->newLayoutForm->setSettings(*this->settings);
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

    /*
    if (this->importedLayout) {
        // Copy layout data from imported layout
        layout->blockdata = this->importedLayout->blockdata;
        if (!this->importedLayout->border.isEmpty())
            layout->border = this->importedLayout->border;
    }
    */

    Layout *layout = this->project->createNewLayout(*this->settings);
    if (!layout)
        return;

    emit applied(layout->id);
    QDialog::accept();
}
