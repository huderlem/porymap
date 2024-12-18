#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"
#include "validator.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewMapDialog::NewMapDialog(Project *project, QWidget *parent) :
    NewMapDialog(project, nullptr, parent)
{}

NewMapDialog::NewMapDialog(Project *project, const Map *mapToCopy, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMapDialog),
    mapToCopy(mapToCopy)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;
    Project::NewMapSettings *settings = &project->newMapSettings;

    if (this->mapToCopy) {
        // Copy settings from the map we're duplicating
        if (this->mapToCopy->layout()){
            settings->layout = this->mapToCopy->layout()->settings();
        }
        settings->header = *this->mapToCopy->header();
        settings->group = project->mapNameToMapGroup(this->mapToCopy->name());
        settings->name = project->toUniqueIdentifier(this->mapToCopy->name());
        
    } else {
        // Not duplicating a map, get a generic new map name.
        // The rest of the settings are preserved in the project between sessions.
        settings->name = project->getNewMapName();
    }
    // Generate a unique Layout constant
    settings->layout.id = project->toUniqueIdentifier(Layout::layoutConstantFromName(settings->name));

    ui->newLayoutForm->initUi(project);
    ui->comboBox_Group->addItems(project->groupNames);
    ui->comboBox_LayoutID->addItems(project->layoutIds);

    auto validator = new IdentifierValidator(this);
    ui->lineEdit_Name->setValidator(validator);
    ui->comboBox_Group->setValidator(validator);
    ui->comboBox_LayoutID->setValidator(validator);

    // Create a collapsible section that has all the map header data.
    this->headerForm = new MapHeaderForm();
    this->headerForm->init(project);
    auto sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(this->headerForm);

    this->headerSection = new CollapsibleSection("Header Data", porymapConfig.newMapHeaderSectionExpanded, 150, this);
    this->headerSection->setContentLayout(sectionLayout);
    ui->layout_HeaderData->addWidget(this->headerSection);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapDialog::dialogButtonClicked);

    refresh();
}

// Adding new map to an existing map list folder. Initialize settings accordingly.
// Even if we initialize settings like this we'll allow users to change them afterwards,
// because nothing is expecting them to stay at these values (with exception to layouts).
NewMapDialog::NewMapDialog(Project *project, int mapListTab, const QString &mapListItem, QWidget *parent) :
    NewMapDialog(project, parent)
{
    switch (mapListTab)
    {
    case MapListTab::Groups:
        ui->comboBox_Group->setTextItem(mapListItem);
        break;
    case MapListTab::Areas:
        this->headerForm->setLocation(mapListItem);
        break;
    case MapListTab::Layouts:
        // We specifically lock the layout ID because otherwise the setting would be overwritten when
        // the user changes the map name (which will normally automatically update the layout ID to match).
        // For the Group/Area settings above we don't care if the user changes them afterwards.
        ui->comboBox_LayoutID->setTextItem(mapListItem);
        ui->comboBox_LayoutID->setDisabled(true);
        break;
    }
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete ui;
}

// Reload the UI from the last-saved settings.
void NewMapDialog::refresh() {
    const Project::NewMapSettings *settings = &this->project->newMapSettings;

    ui->lineEdit_Name->setText(settings->name);
    ui->comboBox_Group->setTextItem(settings->group);

    // If the layout combo box is disabled, it's because we're enforcing the setting. Leave it unchanged.
    if (ui->comboBox_LayoutID->isEnabled())
        ui->comboBox_LayoutID->setTextItem(settings->layout.id);

    ui->newLayoutForm->setSettings(settings->layout);
    ui->checkBox_CanFlyTo->setChecked(settings->canFlyTo);
    this->headerForm->setHeaderData(settings->header);
}

void NewMapDialog::saveSettings() {
    Project::NewMapSettings *settings = &this->project->newMapSettings;

    settings->name = ui->lineEdit_Name->text();
    settings->group = ui->comboBox_Group->currentText();
    settings->layout = ui->newLayoutForm->settings();
    settings->layout.id = ui->comboBox_LayoutID->currentText();
    settings->canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    settings->header = this->headerForm->headerData();

    // This dialog doesn't give users the option to give new layouts a name, we generate one using the map name.
    //  (an older iteration of this dialog gave users an option to name new layouts, but it's extra clutter for
    //   something the majority of users creating a map won't need. If they want to give a specific name to a layout
    //   they can create the layout first, then create a new map that uses that layout.)
    const Layout *layout = this->project->mapLayouts.value(settings->layout.id);
    if (!layout) {
        const QString newLayoutName = QString("%1%2").arg(settings->name).arg(Layout::defaultSuffix());
        settings->layout.name = this->project->toUniqueIdentifier(newLayoutName);
    } else {
        // Pre-existing layout. The layout name won't be read, but we'll make sure it's correct anyway.
        settings->layout.name = layout->name;
    }

    // Folders for new layouts created for new maps use the map name, rather than the layout name
    // (i.e., if you create "MyMap", you'll get a 'data/layouts/MyMap/', rather than 'data/layouts/MyMap_Layout/').
    // There's no real reason for this, aside from maintaining consistency with the default layout folder names that do this.
    settings->layout.folderName = settings->name;

    porymapConfig.newMapHeaderSectionExpanded = this->headerSection->isExpanded();
}

bool NewMapDialog::validateName(bool allowEmpty) {
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

void NewMapDialog::on_lineEdit_Name_textChanged(const QString &text) {
    validateName(true);

    // Changing the map name updates the layout ID field to match.
    if (ui->comboBox_LayoutID->isEnabled()) {
        ui->comboBox_LayoutID->setCurrentText(Layout::layoutConstantFromName(text));
    }
}

bool NewMapDialog::validateGroup(bool allowEmpty) {
    QString groupName = ui->comboBox_Group->currentText();

    QString errorText;
    if (groupName.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_Group->text());
    } else if (!this->project->groupNames.contains(groupName) && !this->project->isIdentifierUnique(groupName)) {
        errorText = QString("%1 must either be the name of an existing map group, or a unique identifier for a new map group.").arg(ui->label_Group->text());
    }

    bool isValid = errorText.isEmpty();
    ui->label_GroupError->setText(errorText);
    ui->label_GroupError->setVisible(!isValid);
    ui->comboBox_Group->lineEdit()->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_comboBox_Group_currentTextChanged(const QString &) {
    validateGroup(true);
}

bool NewMapDialog::validateLayoutID(bool allowEmpty) {
    QString layoutId = ui->comboBox_LayoutID->currentText();

    QString errorText;
    if (layoutId.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_LayoutID->text());
    } else if (!this->project->layoutIds.contains(layoutId) && !this->project->isIdentifierUnique(layoutId)) {
        errorText = QString("%1 must either be the ID for an existing layout, or a unique identifier for a new layout.").arg(ui->label_LayoutID->text());
    }

    bool isValid = errorText.isEmpty();
    ui->label_LayoutIDError->setText(errorText);
    ui->label_LayoutIDError->setVisible(!isValid);
    ui->comboBox_LayoutID->lineEdit()->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_comboBox_LayoutID_currentTextChanged(const QString &text) {
    validateLayoutID(true);

    // Changing the layout ID to an existing layout updates the layout settings to match.
    const Layout *layout = this->project->mapLayouts.value(text);
    if (layout) {
        ui->newLayoutForm->setSettings(layout->settings());
        ui->newLayoutForm->setDisabled(true);
    } else {
        ui->newLayoutForm->setDisabled(false);
    }
}

void NewMapDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        this->project->initNewMapSettings();
        refresh();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewMapDialog::accept() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!ui->newLayoutForm->validate()) success = false;
    if (!validateName()) success = false;
    if (!validateGroup()) success = false;
    if (!validateLayoutID()) success = false;
    if (!success)
        return;

    // Update settings from UI
    saveSettings();

    Map *map = this->project->createNewMap(this->project->newMapSettings, this->mapToCopy);
    if (!map) {
        ui->label_GenericError->setText(QString("Failed to create map. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);
    QDialog::accept();
}
