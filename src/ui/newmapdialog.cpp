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
    this->existingLayout = false; // TODO: Replace, we can determine this from the Layout ID combo box
    this->importedMap = false;

    ui->newLayoutForm->initUi(project);

    ui->comboBox_Group->addItems(project->groupNames);

    // Map names and IDs can only contain word characters, and cannot start with a digit.
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_MapID->setValidator(validator);

    // Create a collapsible section that has all the map header data.
    this->headerForm = new MapHeaderForm();
    this->headerForm->init(project);
    auto sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(this->headerForm);

    this->headerSection = new CollapsibleSection("Header Data", porymapConfig.newMapHeaderSectionExpanded, 150, this);
    this->headerSection->setContentLayout(sectionLayout);
    ui->layout_HeaderData->addWidget(this->headerSection);
    ui->layout_HeaderData->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding));
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
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
        useLayout(fieldName);
        break;
    }
    init();
}

// Creating new map from AdvanceMap import
// TODO: Re-use for a "Duplicate Map/Layout" option?
void NewMapDialog::init(Layout *layout) {
    this->importedMap = true;
    useLayoutSettings(layout);

    // TODO: These are probably leaking
    this->map = new Map();
    this->map->setLayout(new Layout());
    this->map->layout()->blockdata = layout->blockdata;

    if (!layout->border.isEmpty()) {
        this->map->layout()->border = layout->border;
    }
    init();
}

void NewMapDialog::setDefaultSettings(Project *project) {
    settings.group = project->groupNames.at(0);
    settings.canFlyTo = false;
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
    settings.layout.width = layout->width;
    settings.layout.height = layout->height;
    settings.layout.borderWidth = layout->border_width;
    settings.layout.borderHeight = layout->border_height;
    settings.layout.primaryTilesetLabel = layout->tileset_primary_label;
    settings.layout.secondaryTilesetLabel = layout->tileset_secondary_label;

    // Don't allow changes to the layout settings
    ui->newLayoutForm->setDisabled(true);
}

void NewMapDialog::useLayout(QString layoutId) {
    this->existingLayout = true;
    this->layoutId = layoutId;
    useLayoutSettings(project->mapLayouts.value(this->layoutId));    
}

// TODO: Create the map group if it doesn't exist
bool NewMapDialog::validateMapGroup() {
    this->group = project->groupNames.indexOf(ui->comboBox_Group->currentText());

    QString errorText;
    if (this->group < 0) {
        errorText = QString("The specified map group '%1' does not exist.")
                        .arg(ui->comboBox_Group->currentText());
    }

    bool isValid = errorText.isEmpty();
    ui->label_GroupError->setText(errorText);
    ui->label_GroupError->setVisible(!isValid);
    return isValid;
}

bool NewMapDialog::validateID() {
    QString id = ui->lineEdit_MapID->text();

    QString errorText;
    QString expectedPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
    if (!id.startsWith(expectedPrefix)) {
        errorText = QString("The specified ID name '%1' must start with '%2'.").arg(id).arg(expectedPrefix);
    } else {
        for (auto i = project->mapNamesToMapConstants.constBegin(), end = project->mapNamesToMapConstants.constEnd(); i != end; i++) {
            if (id == i.value()) {
                errorText = QString("The specified ID name '%1' is already in use.").arg(id);
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
    validateID();
}

bool NewMapDialog::validateName() {
    QString name = ui->lineEdit_Name->text();

    QString errorText;
    if (project->mapNames.contains(name)) {
        errorText = QString("The specified map name '%1' is already in use.").arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_NameError->setText(errorText);
    ui->label_NameError->setVisible(!isValid);
    ui->lineEdit_Name->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_lineEdit_Name_textChanged(const QString &text) {
    validateName();
    ui->lineEdit_MapID->setText(Map::mapConstantFromName(text));
}

void NewMapDialog::on_pushButton_Accept_clicked() {
    saveSettings();

    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!ui->newLayoutForm->validate()) success = false;
    if (!validateMapGroup()) success = false;
    if (!validateID()) success = false;
    if (!validateName()) success = false;
    if (!success)
        return;

    // We check if the map name is empty separately from validateName, because validateName is also used during editing.
    // It's likely that users will clear the name text box while editing, and we don't want to flash errors at them for this.
    if (ui->lineEdit_Name->text().isEmpty()) {
        ui->label_NameError->setText("The specified map name cannot be empty.");
        ui->label_NameError->setVisible(true);
        ui->lineEdit_Name->setStyleSheet(lineEdit_ErrorStylesheet);
        return;
    }

    Map *newMap = new Map;
    newMap->setName(ui->lineEdit_Name->text());
    newMap->setConstantName(ui->lineEdit_MapID->text());
    newMap->setHeader(this->headerForm->headerData());
    newMap->setNeedsHealLocation(settings.canFlyTo);

    Layout *layout;
    if (this->existingLayout) {
        layout = this->project->mapLayouts.value(this->layoutId);
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
    if (this->importedMap) {
        layout->blockdata = map->layout()->blockdata;
        if (!map->layout()->border.isEmpty())
            layout->border = map->layout()->border;
    }
    newMap->setLayout(layout);

    if (this->existingLayout) {
        project->loadMapLayout(newMap);
    }
    map = newMap;
    emit applied();
    this->close();
}
