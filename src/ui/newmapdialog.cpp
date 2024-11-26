#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"

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
    setModal(true);
    ui->setupUi(this);
    this->project = project;

    QString newMapName;
    QString newLayoutId;
    if (this->mapToCopy) {
        // Duplicating a map, the initial name will be the base map's name
        // with a numbered suffix to make it unique.
        int i = 2;
        do {
            newMapName = QString("%1_%2").arg(this->mapToCopy->name()).arg(i++);
            newLayoutId = Layout::layoutConstantFromName(newMapName);
        } while (!project->isIdentifierUnique(newMapName) || !project->isIdentifierUnique(newLayoutId));
    } else {
        // Not duplicating a map, get a generic new map name.
        newMapName = project->getNewMapName();
        newLayoutId = Layout::layoutConstantFromName(newMapName);
    }

    // We reset these settings for every session with the new map dialog.
    // The rest of the settings are preserved in the project between sessions.
    project->newMapSettings.name = newMapName;
    project->newMapSettings.layout.id = newLayoutId;    

    ui->newLayoutForm->initUi(project);
    
    ui->comboBox_Group->addItems(project->groupNames);
    ui->comboBox_LayoutID->addItems(project->layoutIds);

    // Identifiers can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
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
    ui->layout_HeaderData->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapDialog::dialogButtonClicked);

    refresh();
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

    if (this->mapToCopy && this->mapToCopy->layout()) {
        // When importing a layout these settings shouldn't be changed.
        ui->newLayoutForm->setSettings(this->mapToCopy->layout()->settings());
    } else {
        ui->newLayoutForm->setSettings(settings->layout);
    }
    ui->checkBox_CanFlyTo->setChecked(settings->canFlyTo);
    this->headerForm->setHeaderData(settings->header);
}

void NewMapDialog::saveSettings() {
    Project::NewMapSettings *settings = &this->project->newMapSettings;

    settings->name = ui->lineEdit_Name->text();
    settings->group = ui->comboBox_Group->currentText();
    settings->layout = ui->newLayoutForm->settings();
    settings->layout.id = ui->comboBox_LayoutID->currentText();
    settings->layout.name = Layout::layoutNameFromMapName(settings->name); // TODO: Verify uniqueness
    settings->canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    settings->header = this->headerForm->headerData();

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
        if (this->mapToCopy) {
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
