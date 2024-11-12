#include "newmapdialog.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmapdialog.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

// TODO: Make ui->groupBox_HeaderData collapsible

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

struct NewMapDialog::Settings NewMapDialog::settings = {};

NewMapDialog::NewMapDialog(QWidget *parent, Project *project) :
    QDialog(parent),
    ui(new Ui::NewMapDialog)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->project = project;
    this->existingLayout = false;
    this->importedMap = false;

    // Map names and IDs can only contain word characters, and cannot start with a digit.
    // TODO: Also validate this when we read ProjectIdentifier::define_map_prefix from the config
    static const QRegularExpression re("[A-Za-z_]+[\\w]*");
    auto validator = new QRegularExpressionValidator(re, this);
    ui->lineEdit_Name->setValidator(validator);
    ui->lineEdit_ID->setValidator(validator);

    this->headerData = new MapHeaderForm();
    ui->layout_HeaderData->addWidget(this->headerData);

    connect(ui->spinBox_MapWidth, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){validateMapDimensions();});
    connect(ui->spinBox_MapHeight, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){validateMapDimensions();});
}

NewMapDialog::~NewMapDialog()
{
    saveSettings();
    delete ui;
}

void NewMapDialog::init() {
    // Populate combo boxes
    ui->comboBox_PrimaryTileset->addItems(project->primaryTilesetLabels);
    ui->comboBox_SecondaryTileset->addItems(project->secondaryTilesetLabels);
    ui->comboBox_Group->addItems(project->groupNames);
    this->headerData->setProject(project);

    // Set spin box limits
    ui->spinBox_MapWidth->setMaximum(project->getMaxMapWidth());
    ui->spinBox_MapHeight->setMaximum(project->getMaxMapHeight());
    ui->spinBox_BorderWidth->setMaximum(MAX_BORDER_WIDTH);
    ui->spinBox_BorderHeight->setMaximum(MAX_BORDER_HEIGHT);

    ui->groupBox_BorderDimensions->setVisible(projectConfig.useCustomBorderSize);

    // Restore previous settings
    ui->lineEdit_Name->setText(project->getNewMapName());
    ui->comboBox_Group->setTextItem(settings.group);
    ui->spinBox_MapWidth->setValue(settings.width);
    ui->spinBox_MapHeight->setValue(settings.height);
    ui->spinBox_BorderWidth->setValue(settings.borderWidth);
    ui->spinBox_BorderHeight->setValue(settings.borderHeight);
    ui->comboBox_PrimaryTileset->setTextItem(settings.primaryTilesetLabel);
    ui->comboBox_SecondaryTileset->setTextItem(settings.secondaryTilesetLabel);
    this->headerData->ui->comboBox_Song->setTextItem(settings.song);
    this->headerData->ui->comboBox_Location->setTextItem(settings.location);
    this->headerData->ui->checkBox_RequiresFlash->setChecked(settings.requiresFlash);
    this->headerData->ui->comboBox_Weather->setTextItem(settings.weather);
    this->headerData->ui->comboBox_Type->setTextItem(settings.type);
    this->headerData->ui->comboBox_BattleScene->setTextItem(settings.battleScene);
    this->headerData->ui->checkBox_ShowLocationName->setChecked(settings.showLocationName);
    this->headerData->ui->checkBox_AllowRunning->setChecked(settings.allowRunning);
    this->headerData->ui->checkBox_AllowBiking->setChecked(settings.allowBiking);
    this->headerData->ui->checkBox_AllowEscaping->setChecked(settings.allowEscaping);
    this->headerData->ui->spinBox_FloorNumber->setValue(settings.floorNumber);
    ui->checkBox_CanFlyTo->setChecked(settings.canFlyTo);
}

// Creating new map by right-clicking in the map list
void NewMapDialog::init(int tabIndex, QString fieldName) {
    //initUi();
    switch (tabIndex)
    {
    case MapListTab::Groups:
        settings.group = fieldName;
        //ui->label_Group->setDisabled(true);
        //ui->comboBox_Group->setDisabled(true);
        break;
    case MapListTab::Areas:
        settings.location = fieldName;
        //ui->label_Location->setDisabled(true);
        //ui->comboBox_Location->setDisabled(true);
        break;
    case MapListTab::Layouts:
        useLayout(fieldName);
        break;
    }
    init();
}

// Creating new map from AdvanceMap import
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
    settings.width = project->getDefaultMapDimension();
    settings.height = project->getDefaultMapDimension();
    settings.borderWidth = DEFAULT_BORDER_WIDTH;
    settings.borderHeight = DEFAULT_BORDER_HEIGHT;
    settings.primaryTilesetLabel = project->getDefaultPrimaryTilesetLabel();
    settings.secondaryTilesetLabel = project->getDefaultSecondaryTilesetLabel();
    settings.song = project->defaultSong;
    settings.location = project->mapSectionIdNames.value(0, "0");
    settings.requiresFlash = false;
    settings.weather = project->weatherNames.value(0, "0");
    settings.type = project->mapTypes.value(0, "0");
    settings.battleScene = project->mapBattleScenes.value(0, "0");
    settings.showLocationName = true;
    settings.allowRunning = false;
    settings.allowBiking = false;
    settings.allowEscaping = false;
    settings.floorNumber = 0;
    settings.canFlyTo = false;
}

void NewMapDialog::saveSettings() {
    settings.group = ui->comboBox_Group->currentText();
    settings.width = ui->spinBox_MapWidth->value();
    settings.height = ui->spinBox_MapHeight->value();
    settings.borderWidth = ui->spinBox_BorderWidth->value();
    settings.borderHeight = ui->spinBox_BorderHeight->value();
    settings.primaryTilesetLabel = ui->comboBox_PrimaryTileset->currentText();
    settings.secondaryTilesetLabel = ui->comboBox_SecondaryTileset->currentText();
    settings.song = this->headerData->ui->comboBox_Song->currentText();
    settings.location = this->headerData->ui->comboBox_Location->currentText();
    settings.requiresFlash = this->headerData->ui->checkBox_RequiresFlash->isChecked();
    settings.weather = this->headerData->ui->comboBox_Weather->currentText();
    settings.type = this->headerData->ui->comboBox_Type->currentText();
    settings.battleScene = this->headerData->ui->comboBox_BattleScene->currentText();
    settings.showLocationName = this->headerData->ui->checkBox_ShowLocationName->isChecked();
    settings.allowRunning = this->headerData->ui->checkBox_AllowRunning->isChecked();
    settings.allowBiking = this->headerData->ui->checkBox_AllowBiking->isChecked();
    settings.allowEscaping = this->headerData->ui->checkBox_AllowEscaping->isChecked();
    settings.floorNumber = this->headerData->ui->spinBox_FloorNumber->value();
    settings.canFlyTo = ui->checkBox_CanFlyTo->isChecked();
}

void NewMapDialog::useLayoutSettings(Layout *layout) {
    if (!layout) return;
    settings.width = layout->width;
    settings.height = layout->height;
    settings.borderWidth = layout->border_width;
    settings.borderHeight = layout->border_height;
    settings.primaryTilesetLabel = layout->tileset_primary_label;
    settings.secondaryTilesetLabel = layout->tileset_secondary_label;
}

void NewMapDialog::useLayout(QString layoutId) {
    this->existingLayout = true;
    this->layoutId = layoutId;
    useLayoutSettings(project->mapLayouts.value(this->layoutId));

    // Dimensions and tilesets can't be changed for new maps using an existing layout
    ui->groupBox_MapDimensions->setDisabled(true);
    ui->groupBox_BorderDimensions->setDisabled(true);
    ui->groupBox_Tilesets->setDisabled(true);
}

bool NewMapDialog::validateMapDimensions() {
    int size = project->getMapDataSize(ui->spinBox_MapWidth->value(), ui->spinBox_MapHeight->value());
    int maxSize = project->getMaxMapDataSize();

    QString errorText;
    if (size > maxSize) {
        errorText = QString("The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + 15) * (height + 14) <= %1\n"
                    "The specified map width and height was: (%2 + 15) * (%3 + 14) = %4")
                        .arg(maxSize)
                        .arg(ui->spinBox_MapWidth->value())
                        .arg(ui->spinBox_MapHeight->value())
                        .arg(size);
    }

    bool isValid = errorText.isEmpty();
    ui->label_MapDimensionsError->setText(errorText);
    ui->label_MapDimensionsError->setVisible(!isValid);
    return isValid;
}

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

bool NewMapDialog::validateTilesets() {
    QString primaryTileset = ui->comboBox_PrimaryTileset->currentText();
    QString secondaryTileset = ui->comboBox_SecondaryTileset->currentText();

    QString primaryErrorText;
    if (primaryTileset.isEmpty()) {
        primaryErrorText = QString("The primary tileset cannot be empty.");
    } else if (ui->comboBox_PrimaryTileset->findText(primaryTileset) < 0) {
        primaryErrorText = QString("The specified primary tileset '%1' does not exist.").arg(primaryTileset);
    }

    QString secondaryErrorText;
    if (secondaryTileset.isEmpty()) {
        secondaryErrorText = QString("The secondary tileset cannot be empty.");
    } else if (ui->comboBox_SecondaryTileset->findText(secondaryTileset) < 0) {
        secondaryErrorText = QString("The specified secondary tileset '%2' does not exist.").arg(secondaryTileset);
    }

    QString errorText = QString("%1%2%3")
                        .arg(primaryErrorText)
                        .arg(!primaryErrorText.isEmpty() ? "\n" : "")
                        .arg(secondaryErrorText);

    bool isValid = errorText.isEmpty();
    ui->label_TilesetsError->setText(errorText);
    ui->label_TilesetsError->setVisible(!isValid);
    return isValid;
}

bool NewMapDialog::validateID() {
    QString id = ui->lineEdit_ID->text();

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
    ui->label_IDError->setText(errorText);
    ui->label_IDError->setVisible(!isValid);
    ui->lineEdit_ID->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

void NewMapDialog::on_lineEdit_ID_textChanged(const QString &) {
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
    ui->lineEdit_ID->setText(Map::mapConstantFromName(text));
}

void NewMapDialog::on_pushButton_Accept_clicked() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool success = true;
    if (!validateMapDimensions()) success = false;
    if (!validateMapGroup()) success = false;
    if (!validateTilesets()) success = false;
    if (!validateID()) success = false;
    if (!validateName()) success = false;
    if (!success)
        return;

    // We check if the map name is empty separately from the validation above because it's likely
    // that users will clear the name text box while editing, and we don't want to flash errors at them for this.
    if (ui->lineEdit_Name->text().isEmpty()) {
        ui->label_NameError->setText("The specified map name cannot be empty.");
        ui->label_NameError->setVisible(true);
        ui->lineEdit_Name->setStyleSheet(lineEdit_ErrorStylesheet);
        return;
    }

    Map *newMap = new Map;
    newMap->setName(ui->lineEdit_Name->text());
    newMap->setConstantName(ui->lineEdit_ID->text());
    newMap->setSong(this->headerData->ui->comboBox_Song->currentText());
    newMap->setLocation(this->headerData->ui->comboBox_Location->currentText());
    newMap->setRequiresFlash(this->headerData->ui->checkBox_RequiresFlash->isChecked());
    newMap->setWeather(this->headerData->ui->comboBox_Weather->currentText());
    newMap->setType(this->headerData->ui->comboBox_Type->currentText());
    newMap->setBattleScene(this->headerData->ui->comboBox_BattleScene->currentText());
    newMap->setShowsLocationName(this->headerData->ui->checkBox_ShowLocationName->isChecked());
    if (projectConfig.mapAllowFlagsEnabled) {
        newMap->setAllowsRunning(this->headerData->ui->checkBox_AllowRunning->isChecked());
        newMap->setAllowsBiking(this->headerData->ui->checkBox_AllowBiking->isChecked());
        newMap->setAllowsEscaping(this->headerData->ui->checkBox_AllowEscaping->isChecked());
    }
    if (projectConfig.floorNumberEnabled) {
        newMap->setFloorNumber(this->headerData->ui->spinBox_FloorNumber->value());
    }
    newMap->setNeedsHealLocation(ui->checkBox_CanFlyTo->isChecked());

    Layout *layout;
    if (this->existingLayout) {
        layout = this->project->mapLayouts.value(this->layoutId);
        newMap->setNeedsLayoutDir(false);
    } else {
        layout = new Layout;
        layout->id = Layout::layoutConstantFromName(newMap->name());
        layout->name = QString("%1_Layout").arg(newMap->name());
        layout->width = ui->spinBox_MapWidth->value();
        layout->height = ui->spinBox_MapHeight->value();
        if (projectConfig.useCustomBorderSize) {
            layout->border_width = ui->spinBox_BorderWidth->value();
            layout->border_height = ui->spinBox_BorderHeight->value();
        } else {
            layout->border_width = DEFAULT_BORDER_WIDTH;
            layout->border_height = DEFAULT_BORDER_HEIGHT;
        }
        layout->tileset_primary_label = ui->comboBox_PrimaryTileset->currentText();
        layout->tileset_secondary_label = ui->comboBox_SecondaryTileset->currentText();
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
