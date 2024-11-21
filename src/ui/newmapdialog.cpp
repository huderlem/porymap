#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

Project::NewMapSettings NewMapDialog::settings = {};
bool NewMapDialog::initializedSettings = false;

NewMapDialog::NewMapDialog(Project *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewMapDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    ui->label_GenericError->setVisible(false);
    this->project = project;

    Project::NewMapSettings newSettings = project->getNewMapSettings();
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
    
    ui->comboBox_Group->addItems(project->groupNames);
    ui->comboBox_LayoutID->addItems(project->layoutIds);

    // Identifiers can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_MapID->setValidator(validator);
    ui->comboBox_Group->setValidator(validator);
    ui->comboBox_LayoutID->setValidator(validator);

    // Create a collapsible section that has all the map header data.
    this->headerForm = new MapHeaderForm();
    this->headerForm->init(project);
    this->headerForm->setHeader(&settings.header);
    auto sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(this->headerForm);

    this->headerSection = new CollapsibleSection("Header Data", porymapConfig.newMapHeaderSectionExpanded, 150, this);
    this->headerSection->setContentLayout(sectionLayout);
    ui->layout_HeaderData->addWidget(this->headerSection);
    ui->layout_HeaderData->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapDialog::dialogButtonClicked);

    refresh();
    adjustSize(); // TODO: Save geometry?
}

// Adding new map to existing map list folder.
NewMapDialog::NewMapDialog(Project *project, int mapListTab, const QString &mapListItem, QWidget *parent) :
    NewMapDialog(project, parent)
{
    switch (mapListTab)
    {
    case MapListTab::Groups:
        settings.group = mapListItem;
        ui->label_Group->setDisabled(true);
        ui->comboBox_Group->setDisabled(true);
        ui->comboBox_Group->setTextItem(settings.group);
        break;
    case MapListTab::Areas:
        settings.header.setLocation(mapListItem);
        this->headerForm->setLocationDisabled(true);
        // Header UI is kept in sync automatically by MapHeaderForm
        break;
    case MapListTab::Layouts:
        settings.layout.id = mapListItem;
        ui->label_LayoutID->setDisabled(true);
        ui->comboBox_LayoutID->setDisabled(true);
        ui->comboBox_LayoutID->setTextItem(settings.layout.id);
        break;
    }
}

// TODO: Use for a "Duplicate Map" option
NewMapDialog::NewMapDialog(Project *project, const Map *mapToCopy, QWidget *parent) :
    NewMapDialog(project, parent)
{
    /*
    if (this->importedMap)
        delete this->importedMap;

    this->importedMap = new Map(mapToCopy);
    useLayoutSettings(this->importedMap->layout());
    */
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete this->importedMap;
    delete ui;
}

// Sync UI with settings. If any UI elements are disabled (because their settings are being enforced)
// then we don't update them using the settings here.
void NewMapDialog::refresh() {
    ui->lineEdit_Name->setText(settings.name);
    ui->lineEdit_MapID->setText(settings.id);
    
    ui->comboBox_Group->setTextItem(settings.group);
    ui->checkBox_CanFlyTo->setChecked(settings.canFlyTo);
    ui->comboBox_LayoutID->setTextItem(settings.layout.id);
    ui->newLayoutForm->setSettings(settings.layout);
    // Header UI is kept in sync automatically by MapHeaderForm
}

void NewMapDialog::saveSettings() {
    settings.name = ui->lineEdit_Name->text();
    settings.id = ui->lineEdit_MapID->text();
    settings.group = ui->comboBox_Group->currentText();
    settings.canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    settings.layout = ui->newLayoutForm->settings();
    settings.layout.id = ui->comboBox_LayoutID->currentText();
    // We don't provide full control for naming new layouts here (just via the ID).
    // If a user wants to explicitly name a layout they can create it individually before creating the map.
    settings.layout.name = Layout::layoutNameFromMapName(settings.name);
    settings.header = this->headerForm->headerData();
    porymapConfig.newMapHeaderSectionExpanded = this->headerSection->isExpanded();
}

void NewMapDialog::useLayoutSettings(const Layout *layout) {
    if (layout) {
        ui->newLayoutForm->setSettings(layout->settings());
        ui->newLayoutForm->setDisabled(true);
    } else {
        ui->newLayoutForm->setDisabled(false);
    }
}

bool NewMapDialog::validateMapID(bool allowEmpty) {
    QString id = ui->lineEdit_MapID->text();
    const QString expectedPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);

    QString errorText;
    if (id.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_MapID->text());
    } else if (!id.startsWith(expectedPrefix)) {
        errorText = QString("%1 '%2' must start with '%3'.").arg(ui->label_MapID->text()).arg(id).arg(expectedPrefix);
    } else if (!this->project->isIdentifierUnique(id)) {
        errorText = QString("%1 '%2' is not unique.").arg(ui->label_MapID->text()).arg(id);
    }

    bool isValid = errorText.isEmpty();
    ui->label_MapIDError->setText(errorText);
    ui->label_MapIDError->setVisible(!isValid);
    ui->lineEdit_MapID->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_lineEdit_MapID_textChanged(const QString &) {
    validateMapID(true);
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
    ui->lineEdit_MapID->setText(Map::mapConstantFromName(text));
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
    useLayoutSettings(this->project->mapLayouts.value(text));
}

void NewMapDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        auto newSettings = this->project->getNewMapSettings();

        // If the location setting is disabled we need to enforce that setting on the new header.
        if (this->headerForm->isLocationDisabled())
            newSettings.header.setLocation(settings.header.location());

        settings = newSettings;
        this->headerForm->setHeader(&settings.header); // TODO: Unnecessary?
        refresh();

    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewMapDialog::accept() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!ui->newLayoutForm->validate()) success = false;
    if (!validateMapID()) success = false;
    if (!validateName()) success = false;
    if (!validateGroup()) success = false;
    if (!validateLayoutID()) success = false;
    if (!success)
        return;

    // Update settings from UI
    saveSettings();

    Map *newMap = new Map;
    newMap->setName(settings.name);
    newMap->setConstantName(settings.id);
    newMap->setHeader(settings.header);
    newMap->setNeedsHealLocation(settings.canFlyTo);

    Layout *layout = this->project->mapLayouts.value(settings.layout.id);
    if (layout) {
        // Layout already exists
        newMap->setNeedsLayoutDir(false); // TODO: Remove this member?
    } else {
        layout = this->project->createNewLayout(settings.layout);
    }
    if (!layout) {
        ui->label_GenericError->setText(QString("Failed to create layout for map. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        delete newMap;
        return;
    }
    ui->label_GenericError->setVisible(false);

    newMap->setLayout(layout);

    this->project->addNewMap(newMap, settings.group);
    emit applied(newMap->name());
    QDialog::accept();
}
