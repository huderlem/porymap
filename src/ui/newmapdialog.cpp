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
    auto sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(this->headerForm);

    this->headerSection = new CollapsibleSection("Header Data", porymapConfig.newMapHeaderSectionExpanded, 150, this);
    this->headerSection->setContentLayout(sectionLayout);
    ui->layout_HeaderData->addWidget(this->headerSection);
    ui->layout_HeaderData->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapDialog::dialogButtonClicked);

    setUI(settings);
    adjustSize(); // TODO: Save geometry?
}

// Adding new map to existing map list folder. Initialize settings accordingly.
// Even if we initialize settings like this we'll allow users to change them afterwards,
// because nothing is expecting them to stay at these values.
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
        ui->comboBox_LayoutID->setTextItem(mapListItem);
        break;
    }
}

NewMapDialog::NewMapDialog(Project *project, const Map *mapToCopy, QWidget *parent) :
    NewMapDialog(project, parent)
{
    if (!mapToCopy)
        return;
    // TODO
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete this->importedMap;
    delete ui;
}

void NewMapDialog::setUI(const Project::NewMapSettings &settings) {
    ui->lineEdit_Name->setText(settings.name);
    ui->lineEdit_MapID->setText(settings.id);
    ui->comboBox_Group->setTextItem(settings.group);
    ui->comboBox_LayoutID->setTextItem(settings.layout.id);
    if (this->importedMap && this->importedMap->layout()) {
        // When importing a layout these settings shouldn't be changed.
        ui->newLayoutForm->setSettings(this->importedMap->layout()->settings());
    } else {
        ui->newLayoutForm->setSettings(settings.layout);
    }
    ui->checkBox_CanFlyTo->setChecked(settings.canFlyTo);
    this->headerForm->setHeaderData(settings.header);
}

void NewMapDialog::saveSettings() {
    settings.name = ui->lineEdit_Name->text();
    settings.id = ui->lineEdit_MapID->text();
    settings.group = ui->comboBox_Group->currentText();
    settings.layout = ui->newLayoutForm->settings();
    settings.layout.id = ui->comboBox_LayoutID->currentText();
    // We don't provide full control for naming new layouts here (just via the ID).
    // If a user wants to explicitly name a layout they can create it individually before creating the map.
    settings.layout.name = Layout::layoutNameFromMapName(settings.name); // TODO: Verify uniqueness
    settings.canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    settings.header = this->headerForm->headerData();
    porymapConfig.newMapHeaderSectionExpanded = this->headerSection->isExpanded();
}

void NewMapDialog::setLayout(const Layout *layout) {
    if (layout) {
        ui->comboBox_LayoutID->setTextItem(layout->id);
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
    if (id.isEmpty() || id == expectedPrefix) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_MapID->text());
    } else if (!id.startsWith(expectedPrefix)) {
        errorText = QString("%1 must start with '%2'.").arg(ui->label_MapID->text()).arg(expectedPrefix);
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
    } else if (!this->project->isIdentifierUnique(layoutId)) {
        // Layout name is already in use by something. If we're duplicating a map this isn't allowed.
        if (this->importedMap) {
            errorText = QString("%1 is not unique.").arg(ui->label_LayoutID->text());
        // If we're not duplicating a map this is ok as long as it's the name of an existing layout.
        } else if (!this->project->layoutIds.contains(layoutId)) {
            errorText = QString("%1 must either be the ID for an existing layout, or a unique identifier for a new layout.").arg(ui->label_LayoutID->text());
        }
    }

    bool isValid = errorText.isEmpty();
    ui->label_LayoutIDError->setText(errorText);
    ui->label_LayoutIDError->setVisible(!isValid);
    ui->comboBox_LayoutID->lineEdit()->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_comboBox_LayoutID_currentTextChanged(const QString &text) {
    validateLayoutID(true);
    setLayout(this->project->mapLayouts.value(text));
}

void NewMapDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        setUI(this->project->getNewMapSettings());
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

    Map *map = this->project->createNewMap(settings, this->importedMap);
    if (!map) {
        ui->label_GenericError->setText(QString("Failed to create map. See %1 for details.").arg(getLogPath()));
        ui->label_GenericError->setVisible(true);
        return;
    }
    ui->label_GenericError->setVisible(false);
    QDialog::accept();
}
