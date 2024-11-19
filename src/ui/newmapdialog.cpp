#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewMapDialog::NewMapDialog(QWidget *parent, Project *project) :
    QDialog(parent),
    ui(new Ui::NewMapDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    this->project = project;
    this->settings = &project->newMapSettings;

    // Populate UI using data from project
    this->settings->mapName = project->getNewMapName();
    ui->newLayoutForm->initUi(project);
    ui->comboBox_Group->addItems(project->groupNames);
    ui->comboBox_LayoutID->addItems(project->layoutIds);

    // Names and IDs can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_MapID->setValidator(validator);
    ui->comboBox_Group->setValidator(validator);
    ui->comboBox_LayoutID->setValidator(validator);

    // Create a collapsible section that has all the map header data.
    this->headerForm = new MapHeaderForm();
    this->headerForm->init(project);
    this->headerForm->setHeader(&this->settings->header);
    auto sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(this->headerForm);

    this->headerSection = new CollapsibleSection("Header Data", porymapConfig.newMapHeaderSectionExpanded, 150, this);
    this->headerSection->setContentLayout(sectionLayout);
    ui->layout_HeaderData->addWidget(this->headerSection);
    ui->layout_HeaderData->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &NewMapDialog::dialogButtonClicked);
    connect(ui->comboBox_LayoutID, &QComboBox::currentTextChanged, this, &NewMapDialog::useLayoutIdSettings);

    adjustSize(); // TODO: Save geometry?
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete this->importedLayout;
    delete ui;
}

void NewMapDialog::init() {
    const QSignalBlocker b_LayoutId(ui->comboBox_LayoutID);
    ui->comboBox_LayoutID->setCurrentText(this->settings->layout.id);

    ui->lineEdit_Name->setText(this->settings->mapName);
    ui->comboBox_Group->setTextItem(this->settings->group);
    ui->checkBox_CanFlyTo->setChecked(this->settings->canFlyTo);
    ui->newLayoutForm->setSettings(this->settings->layout);
}

// Creating new map by right-clicking in the map list
void NewMapDialog::init(int tabIndex, QString fieldName) {
    switch (tabIndex)
    {
    case MapListTab::Groups:
        this->settings->group = fieldName;
        ui->label_Group->setDisabled(true);
        ui->comboBox_Group->setDisabled(true);
        break;
    case MapListTab::Areas:
        this->settings->header.setLocation(fieldName);
        this->headerForm->setLocationsDisabled(true);
        break;
    case MapListTab::Layouts:
        ui->label_LayoutID->setDisabled(true);
        ui->comboBox_LayoutID->setDisabled(true);
        useLayoutIdSettings(fieldName);
        break;
    }
    init();
}

// Creating new map from AdvanceMap import
// TODO: Re-use for a "Duplicate Map/Layout" option?
void NewMapDialog::init(Layout *layoutToCopy) {
    if (this->importedLayout)
        delete this->importedLayout;

    this->importedLayout = new Layout();
    this->importedLayout->blockdata = layoutToCopy->blockdata;
    if (!layoutToCopy->border.isEmpty())
        this->importedLayout->border = layoutToCopy->border;

    useLayoutSettings(this->importedLayout);
    init();
}

void NewMapDialog::saveSettings() {
    this->settings->mapName = ui->lineEdit_Name->text();
    this->settings->mapId = ui->lineEdit_MapID->text();
    this->settings->group = ui->comboBox_Group->currentText();
    this->settings->canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    this->settings->layout = ui->newLayoutForm->settings();
    this->settings->layout.id = ui->comboBox_LayoutID->currentText();
    this->settings->layout.name = QString("%1_Layout").arg(this->settings->mapName);
    this->settings->header = this->headerForm->headerData();
    porymapConfig.newMapHeaderSectionExpanded = this->headerSection->isExpanded();
}

void NewMapDialog::useLayoutSettings(const Layout *layout) {
    if (!layout) {
        ui->newLayoutForm->setDisabled(false);
        return;
    }

    this->settings->layout.width = layout->width;
    this->settings->layout.height = layout->height;
    this->settings->layout.borderWidth = layout->border_width;
    this->settings->layout.borderHeight = layout->border_height;
    this->settings->layout.primaryTilesetLabel = layout->tileset_primary_label;
    this->settings->layout.secondaryTilesetLabel = layout->tileset_secondary_label;

    // Don't allow changes to the layout settings
    ui->newLayoutForm->setSettings(this->settings->layout);
    ui->newLayoutForm->setDisabled(true);
}

void NewMapDialog::useLayoutIdSettings(const QString &layoutId) {
    this->settings->layout.id = layoutId;
    useLayoutSettings(this->project->mapLayouts.value(layoutId));
}

// Return true if the "layout ID" field is specifying a layout that already exists.
bool NewMapDialog::isExistingLayout() const {
    return this->project->mapLayouts.contains(this->settings->layout.id);
}

bool NewMapDialog::validateMapID(bool allowEmpty) {
    QString id = ui->lineEdit_MapID->text();
    const QString expectedPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);

    QString errorText;
    if (id.isEmpty()) {
        if (!allowEmpty) errorText = QString("%1 cannot be empty.").arg(ui->label_MapID->text());
    } else if (!id.startsWith(expectedPrefix)) {
        errorText = QString("%1 '%2' must start with '%3'.").arg(ui->label_MapID->text()).arg(id).arg(expectedPrefix);
    } else {
        for (auto i = this->project->mapNamesToMapConstants.constBegin(), end = this->project->mapNamesToMapConstants.constEnd(); i != end; i++) {
            if (id == i.value()) {
                errorText = QString("%1 '%2' is already in use.").arg(ui->label_MapID->text()).arg(id);
                break;
            }
        }
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
    } else if (project->mapNames.contains(name)) {
        errorText = QString("%1 '%2' is already in use.").arg(ui->label_Name->text()).arg(name);
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

void NewMapDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::RejectRole){
        reject();
    } else if (role == QDialogButtonBox::ResetRole) {
        this->project->initNewMapSettings(); // TODO: Don't allow this to change locked settings
        init();
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
    if (!success)
        return;

    // Update settings from UI
    saveSettings();

    Map *newMap = new Map;
    newMap->setName(this->settings->mapName);
    newMap->setConstantName(this->settings->mapId);
    newMap->setHeader(this->settings->header);
    newMap->setNeedsHealLocation(this->settings->canFlyTo);

    Layout *layout = nullptr;
    const bool existingLayout = isExistingLayout();
    if (existingLayout) {
        layout = this->project->mapLayouts.value(this->settings->layout.id);
        newMap->setNeedsLayoutDir(false); // TODO: Remove this member
    } else {
        /* TODO: Re-implement (make sure this won't ever override an existing layout)
        if (this->importedLayout) {
            // Copy layout data from imported layout
            layout->blockdata = this->importedLayout->blockdata;
            if (!this->importedLayout->border.isEmpty())
                layout->border = this->importedLayout->border;
        }
        */
        layout = this->project->createNewLayout(this->settings->layout);
    }
    if (!layout)
        return;

    newMap->setLayout(layout);

    this->project->addNewMap(newMap, this->settings->group);
    emit applied(newMap->name());
    QDialog::accept();
}
