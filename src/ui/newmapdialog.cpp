#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

struct NewMapDialog::Settings NewMapDialog::settings = {};

NewMapDialog::NewMapDialog(QWidget *parent, Project *project) :
    QDialog(parent),
    ui(new Ui::NewMapDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui->setupUi(this);
    this->project = project;

    ui->newLayoutForm->initUi(project);

    ui->comboBox_Group->addItems(project->groupNames);

    // Map names and IDs can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_MapID->setValidator(validator);
    ui->comboBox_Group->setValidator(validator);

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
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete this->importedLayout;
    delete ui;
}

void NewMapDialog::init() {
    ui->comboBox_Group->setTextItem(settings.group);
    ui->checkBox_CanFlyTo->setChecked(settings.canFlyTo);
    ui->newLayoutForm->setSettings(settings.layout);
    this->headerForm->setHeader(&settings.header);
    ui->lineEdit_Name->setText(project->getNewMapName());
}

// Creating new map by right-clicking in the map list
void NewMapDialog::init(int tabIndex, QString fieldName) {
    switch (tabIndex)
    {
    case MapListTab::Groups:
        settings.group = fieldName;
        ui->label_Group->setDisabled(true);
        ui->comboBox_Group->setDisabled(true);
        break;
    case MapListTab::Areas:
        settings.header.setLocation(fieldName);
        this->headerForm->setLocationsDisabled(true);
        break;
    case MapListTab::Layouts:
        useLayoutSettings(project->mapLayouts.value(fieldName));
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

void NewMapDialog::setDefaultSettings(const Project *project) {
    settings.group = project->groupNames.at(0);
    settings.canFlyTo = false;
    // TODO: Layout id
    settings.layout.width = project->getDefaultMapDimension();
    settings.layout.height = project->getDefaultMapDimension();
    settings.layout.borderWidth = DEFAULT_BORDER_WIDTH;
    settings.layout.borderHeight = DEFAULT_BORDER_HEIGHT;
    settings.layout.primaryTilesetLabel = project->getDefaultPrimaryTilesetLabel();
    settings.layout.secondaryTilesetLabel = project->getDefaultSecondaryTilesetLabel();
    settings.header.setSong(project->defaultSong);
    settings.header.setLocation(project->mapSectionIdNames.value(0, "0"));
    settings.header.setRequiresFlash(false);
    settings.header.setWeather(project->weatherNames.value(0, "0"));
    settings.header.setType(project->mapTypes.value(0, "0"));
    settings.header.setBattleScene(project->mapBattleScenes.value(0, "0"));
    settings.header.setShowsLocationName(true);
    settings.header.setAllowsRunning(false);
    settings.header.setAllowsBiking(false);
    settings.header.setAllowsEscaping(false);
    settings.header.setFloorNumber(0);
}

void NewMapDialog::saveSettings() {
    settings.group = ui->comboBox_Group->currentText();
    settings.canFlyTo = ui->checkBox_CanFlyTo->isChecked();
    settings.layout = ui->newLayoutForm->settings();
    settings.header = this->headerForm->headerData();
    porymapConfig.newMapHeaderSectionExpanded = this->headerSection->isExpanded();
}

void NewMapDialog::useLayoutSettings(Layout *layout) {
    if (!layout) return;
    settings.layout.id = layout->id;
    settings.layout.width = layout->width;
    settings.layout.height = layout->height;
    settings.layout.borderWidth = layout->border_width;
    settings.layout.borderHeight = layout->border_height;
    settings.layout.primaryTilesetLabel = layout->tileset_primary_label;
    settings.layout.secondaryTilesetLabel = layout->tileset_secondary_label;

    // Don't allow changes to the layout settings
    ui->newLayoutForm->setDisabled(true);
}

// Return true if the "layout ID" field is specifying a layout that already exists.
bool NewMapDialog::isExistingLayout() const {
    return this->project->mapLayouts.contains(settings.layout.id);
}

bool NewMapDialog::validateID(bool allowEmpty) {
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
    validateID(true);
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
        setDefaultSettings(this->project); // TODO: Don't allow this to change locked settings
        init();
    } else if (role == QDialogButtonBox::AcceptRole) {
        accept();
    }
}

void NewMapDialog::accept() {
    saveSettings();

    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!ui->newLayoutForm->validate()) success = false;
    if (!validateID()) success = false;
    if (!validateName()) success = false;
    if (!validateGroup()) success = false;
    if (!success)
        return;

    Map *newMap = new Map;
    newMap->setName(ui->lineEdit_Name->text());
    newMap->setConstantName(ui->lineEdit_MapID->text());
    newMap->setHeader(this->headerForm->headerData());
    newMap->setNeedsHealLocation(settings.canFlyTo);

    Layout *layout;
    const bool existingLayout = isExistingLayout();
    if (existingLayout) {
        layout = this->project->mapLayouts.value(settings.layout.id);
        newMap->setNeedsLayoutDir(false);
    } else {
        layout = new Layout;
        layout->id = Layout::layoutConstantFromName(newMap->name());
        layout->name = QString("%1_Layout").arg(newMap->name());
        layout->width = settings.layout.width;
        layout->height = settings.layout.height;
        if (projectConfig.useCustomBorderSize) {
            layout->border_width = settings.layout.borderWidth;
            layout->border_height = settings.layout.borderHeight;
        } else {
            layout->border_width = DEFAULT_BORDER_WIDTH;
            layout->border_height = DEFAULT_BORDER_HEIGHT;
        }
        layout->tileset_primary_label = settings.layout.primaryTilesetLabel;
        layout->tileset_secondary_label = settings.layout.secondaryTilesetLabel;
        QString basePath = projectConfig.getFilePath(ProjectFilePath::data_layouts_folders);
        layout->border_path = QString("%1%2/border.bin").arg(basePath, newMap->name());
        layout->blockdata_path = QString("%1%2/map.bin").arg(basePath, newMap->name());
    }
    if (this->importedLayout) { // TODO: This seems at odds with existingLayout. Would it be possible to override an existing layout?
        // Copy layout data from imported layout
        layout->blockdata = this->importedLayout->blockdata;
        if (!this->importedLayout->border.isEmpty())
            layout->border = this->importedLayout->border;
    }
    newMap->setLayout(layout);

    this->project->addNewMap(newMap, settings.group);
    emit applied(newMap->name());
    QDialog::accept();
}
