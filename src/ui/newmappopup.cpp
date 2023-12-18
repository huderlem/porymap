#include "newmappopup.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmappopup.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QStringList>

struct NewMapPopup::Settings NewMapPopup::settings = {};

NewMapPopup::NewMapPopup(QWidget *parent, Project *project) :
    QMainWindow(parent),
    ui(new Ui::NewMapPopup)
{
    ui->setupUi(this);
    this->project = project;
    this->existingLayout = false;
    this->importedMap = false;
}

NewMapPopup::~NewMapPopup()
{
    saveSettings();
    delete ui;
}

void NewMapPopup::init() {
    // Populate combo boxes
    ui->comboBox_NewMap_Primary_Tileset->addItems(project->primaryTilesetLabels);
    ui->comboBox_NewMap_Secondary_Tileset->addItems(project->secondaryTilesetLabels);
    ui->comboBox_NewMap_Group->addItems(project->groupNames);
    ui->comboBox_NewMap_Song->addItems(project->songNames);
    ui->comboBox_NewMap_Type->addItems(project->mapTypes);
    ui->comboBox_NewMap_Location->addItems(project->mapSectionValueToName.values());

    // Set spin box limits
    ui->spinBox_NewMap_Width->setMinimum(1);
    ui->spinBox_NewMap_Height->setMinimum(1);
    ui->spinBox_NewMap_Width->setMaximum(project->getMaxMapWidth());
    ui->spinBox_NewMap_Height->setMaximum(project->getMaxMapHeight());
    ui->spinBox_NewMap_BorderWidth->setMinimum(1);
    ui->spinBox_NewMap_BorderHeight->setMinimum(1);
    ui->spinBox_NewMap_BorderWidth->setMaximum(MAX_BORDER_WIDTH);
    ui->spinBox_NewMap_BorderHeight->setMaximum(MAX_BORDER_HEIGHT);
    ui->spinBox_NewMap_Floor_Number->setMinimum(-128);
    ui->spinBox_NewMap_Floor_Number->setMaximum(127);

    // Hide config specific ui elements
    bool hasFlags = projectConfig.getMapAllowFlagsEnabled();
    ui->checkBox_NewMap_Allow_Running->setVisible(hasFlags);
    ui->checkBox_NewMap_Allow_Biking->setVisible(hasFlags);
    ui->checkBox_NewMap_Allow_Escape_Rope->setVisible(hasFlags);
    ui->label_NewMap_Allow_Running->setVisible(hasFlags);
    ui->label_NewMap_Allow_Biking->setVisible(hasFlags);
    ui->label_NewMap_Allow_Escape_Rope->setVisible(hasFlags);

    bool hasCustomBorders = projectConfig.getUseCustomBorderSize();
    ui->spinBox_NewMap_BorderWidth->setVisible(hasCustomBorders);
    ui->spinBox_NewMap_BorderHeight->setVisible(hasCustomBorders);
    ui->label_NewMap_BorderWidth->setVisible(hasCustomBorders);
    ui->label_NewMap_BorderHeight->setVisible(hasCustomBorders);

    bool hasFloorNumber = projectConfig.getFloorNumberEnabled();
    ui->spinBox_NewMap_Floor_Number->setVisible(hasFloorNumber);
    ui->label_NewMap_Floor_Number->setVisible(hasFloorNumber);

    // Restore previous settings
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());
    ui->comboBox_NewMap_Group->setTextItem(settings.group);
    ui->spinBox_NewMap_Width->setValue(settings.width);
    ui->spinBox_NewMap_Height->setValue(settings.height);
    ui->spinBox_NewMap_BorderWidth->setValue(settings.borderWidth);
    ui->spinBox_NewMap_BorderHeight->setValue(settings.borderHeight);
    ui->comboBox_NewMap_Primary_Tileset->setTextItem(settings.primaryTilesetLabel);
    ui->comboBox_NewMap_Secondary_Tileset->setTextItem(settings.secondaryTilesetLabel);
    ui->comboBox_NewMap_Type->setTextItem(settings.type);
    ui->comboBox_NewMap_Location->setTextItem(settings.location);
    ui->comboBox_NewMap_Song->setTextItem(settings.song);
    ui->checkBox_NewMap_Flyable->setChecked(settings.canFlyTo);
    ui->checkBox_NewMap_Show_Location->setChecked(settings.showLocationName);
    ui->checkBox_NewMap_Allow_Running->setChecked(settings.allowRunning);
    ui->checkBox_NewMap_Allow_Biking->setChecked(settings.allowBiking);
    ui->checkBox_NewMap_Allow_Escape_Rope->setChecked(settings.allowEscaping);
    ui->spinBox_NewMap_Floor_Number->setValue(settings.floorNumber);

    // Connect signals
    connect(ui->spinBox_NewMap_Width, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});
    connect(ui->spinBox_NewMap_Height, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});

    ui->frame_NewMap_Options->setEnabled(true);
}

// Creating new map by right-clicking in the map list
void NewMapPopup::init(MapSortOrder type, QVariant data) {
    switch (type)
    {
    case MapSortOrder::Group:
        settings.group = project->groupNames.at(data.toInt());
        break;
    case MapSortOrder::Area:
        settings.location = data.toString();
        break;
    case MapSortOrder::Layout:
        useLayout(data.toString());
        break;
    }
    init();
}

// Creating new map from AdvanceMap import
void NewMapPopup::init(MapLayout *mapLayout) {
    this->importedMap = true;
    useLayoutSettings(mapLayout);

    this->map = new Map();
    this->map->layout = new MapLayout();
    this->map->layout->blockdata = mapLayout->blockdata;

    if (!mapLayout->border.isEmpty()) {
        this->map->layout->border = mapLayout->border;
    }
    init();
}

bool NewMapPopup::checkNewMapDimensions() {
    int numMetatiles = project->getMapDataSize(ui->spinBox_NewMap_Width->value(), ui->spinBox_NewMap_Height->value());
    int maxMetatiles = project->getMaxMapDataSize();

    if (numMetatiles > maxMetatiles) {
        ui->frame_NewMap_Warning->setVisible(true);
        QString errorText = QString("Error: The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + 15) * (height + 14) <= %1\n"
                    "The specified map width and height was: (%2 + 15) * (%3 + 14) = %4")
                        .arg(maxMetatiles)
                        .arg(ui->spinBox_NewMap_Width->value())
                        .arg(ui->spinBox_NewMap_Height->value())
                        .arg(numMetatiles);
        ui->label_NewMap_WarningMessage->setText(errorText);
        ui->label_NewMap_WarningMessage->setWordWrap(true);
        return false;
    }
    else {
        ui->frame_NewMap_Warning->setVisible(false);
        ui->label_NewMap_WarningMessage->clear();
        return true;
    }
}

bool NewMapPopup::checkNewMapGroup() {
    group = project->groupNames.indexOf(this->ui->comboBox_NewMap_Group->currentText());

    if (group < 0) {
        ui->frame_NewMap_Warning->setVisible(true);
        QString errorText = QString("Error: The specified map group '%1' does not exist.")
                        .arg(ui->comboBox_NewMap_Group->currentText());
        ui->label_NewMap_WarningMessage->setText(errorText);
        ui->label_NewMap_WarningMessage->setWordWrap(true);
        return false;
    } else {
        ui->frame_NewMap_Warning->setVisible(false);
        ui->label_NewMap_WarningMessage->clear();
        return true;
    }
}

void NewMapPopup::setDefaultSettings(Project *project) {
    settings.group = project->groupNames.at(0);
    settings.width = project->getDefaultMapSize();
    settings.height = project->getDefaultMapSize();
    settings.borderWidth = DEFAULT_BORDER_WIDTH;
    settings.borderHeight = DEFAULT_BORDER_HEIGHT;
    settings.primaryTilesetLabel = project->getDefaultPrimaryTilesetLabel();
    settings.secondaryTilesetLabel = project->getDefaultSecondaryTilesetLabel();
    settings.type = project->mapTypes.at(0);
    settings.location = project->mapSectionValueToName.values().at(0);
    settings.song = project->defaultSong;
    settings.canFlyTo = false;
    settings.showLocationName = true;
    settings.allowRunning = false;
    settings.allowBiking = false;
    settings.allowEscaping = false;
    settings.floorNumber = 0;
}

void NewMapPopup::saveSettings() {
    settings.group = ui->comboBox_NewMap_Group->currentText();
    settings.width = ui->spinBox_NewMap_Width->value();
    settings.height = ui->spinBox_NewMap_Height->value();
    settings.borderWidth = ui->spinBox_NewMap_BorderWidth->value();
    settings.borderHeight = ui->spinBox_NewMap_BorderHeight->value();
    settings.primaryTilesetLabel = ui->comboBox_NewMap_Primary_Tileset->currentText();
    settings.secondaryTilesetLabel = ui->comboBox_NewMap_Secondary_Tileset->currentText();
    settings.type = ui->comboBox_NewMap_Type->currentText();
    settings.location = ui->comboBox_NewMap_Location->currentText();
    settings.song = ui->comboBox_NewMap_Song->currentText();
    settings.canFlyTo = ui->checkBox_NewMap_Flyable->isChecked();
    settings.showLocationName = ui->checkBox_NewMap_Show_Location->isChecked();
    settings.allowRunning = ui->checkBox_NewMap_Allow_Running->isChecked();
    settings.allowBiking = ui->checkBox_NewMap_Allow_Biking->isChecked();
    settings.allowEscaping = ui->checkBox_NewMap_Allow_Escape_Rope->isChecked();
    settings.floorNumber = ui->spinBox_NewMap_Floor_Number->value();
}

void NewMapPopup::useLayoutSettings(MapLayout *layout) {
    if (!layout) return;
    settings.width = layout->width;
    settings.height = layout->height;
    settings.borderWidth = layout->border_width;
    settings.borderHeight = layout->border_height;
    settings.primaryTilesetLabel = layout->tileset_primary_label;
    settings.secondaryTilesetLabel = layout->tileset_secondary_label;
}

void NewMapPopup::useLayout(QString layoutId) {
    this->existingLayout = true;
    this->layoutId = layoutId;
    useLayoutSettings(project->mapLayouts.value(this->layoutId));

    // Dimensions and tilesets can't be changed for new maps using an existing layout
    ui->spinBox_NewMap_Width->setDisabled(true);
    ui->spinBox_NewMap_Height->setDisabled(true);
    ui->spinBox_NewMap_BorderWidth->setDisabled(true);
    ui->spinBox_NewMap_BorderHeight->setDisabled(true);
    ui->comboBox_NewMap_Primary_Tileset->setDisabled(true);
    ui->comboBox_NewMap_Secondary_Tileset->setDisabled(true);
}

void NewMapPopup::on_lineEdit_NewMap_Name_textChanged(const QString &text) {
    if (project->mapNames.contains(text)) {
        this->ui->lineEdit_NewMap_Name->setStyleSheet("QLineEdit { background-color: rgba(255, 0, 0, 25%) }");
    } else {
        this->ui->lineEdit_NewMap_Name->setStyleSheet("");
    }
}

void NewMapPopup::on_pushButton_NewMap_Accept_clicked() {
    if (!checkNewMapDimensions() || !checkNewMapGroup()) {
        // ignore when map dimensions or map group are invalid
        return;
    }
    Map *newMap = new Map;
    MapLayout *layout;

    // If map name is not unique, use default value. Also use only valid characters.
    // After stripping invalid characters, strip any leading digits.
    static const QRegularExpression re_invalidChars("[^a-zA-Z0-9_]+");
    QString newMapName = this->ui->lineEdit_NewMap_Name->text().remove(re_invalidChars);
    static const QRegularExpression re_NaN("^[0-9]*");
    newMapName.remove(re_NaN);
    if (project->mapNames.contains(newMapName) || newMapName.isEmpty()) {
        newMapName = project->getNewMapName();
    }

    newMap->name = newMapName;
    newMap->type = this->ui->comboBox_NewMap_Type->currentText();
    newMap->location = this->ui->comboBox_NewMap_Location->currentText();
    newMap->song = this->ui->comboBox_NewMap_Song->currentText();
    newMap->requiresFlash = false;
    newMap->weather = this->project->weatherNames.value(0);
    newMap->show_location = this->ui->checkBox_NewMap_Show_Location->isChecked();
    newMap->battle_scene = this->project->mapBattleScenes.value(0);

    if (this->existingLayout) {
        layout = this->project->mapLayouts.value(this->layoutId);
        newMap->needsLayoutDir = false;
    } else {
        layout = new MapLayout;
        layout->id = MapLayout::layoutConstantFromName(newMapName);
        layout->name = QString("%1_Layout").arg(newMap->name);
        layout->width = this->ui->spinBox_NewMap_Width->value();
        layout->height = this->ui->spinBox_NewMap_Height->value();
        if (projectConfig.getUseCustomBorderSize()) {
            layout->border_width = this->ui->spinBox_NewMap_BorderWidth->value();
            layout->border_height = this->ui->spinBox_NewMap_BorderHeight->value();
        } else {
            layout->border_width = DEFAULT_BORDER_WIDTH;
            layout->border_height = DEFAULT_BORDER_HEIGHT;
        }
        layout->tileset_primary_label = this->ui->comboBox_NewMap_Primary_Tileset->currentText();
        layout->tileset_secondary_label = this->ui->comboBox_NewMap_Secondary_Tileset->currentText();
        QString basePath = projectConfig.getFilePath(ProjectFilePath::data_layouts_folders);
        layout->border_path = QString("%1%2/border.bin").arg(basePath, newMapName);
        layout->blockdata_path = QString("%1%2/map.bin").arg(basePath, newMapName);
    }

    if (this->importedMap) {
        layout->blockdata = map->layout->blockdata;
        if (!map->layout->border.isEmpty())
            layout->border = map->layout->border;
    }

    if (this->ui->checkBox_NewMap_Flyable->isChecked()) {
        newMap->needsHealLocation = true;
    }

    if (projectConfig.getMapAllowFlagsEnabled()) {
        newMap->allowRunning = this->ui->checkBox_NewMap_Allow_Running->isChecked();
        newMap->allowBiking = this->ui->checkBox_NewMap_Allow_Biking->isChecked();
        newMap->allowEscaping = this->ui->checkBox_NewMap_Allow_Escape_Rope->isChecked();
    }
    if (projectConfig.getFloorNumberEnabled()) {
        newMap->floorNumber = this->ui->spinBox_NewMap_Floor_Number->value();
    }

    newMap->layout = layout;
    newMap->layoutId = layout->id;
    if (this->existingLayout) {
        project->loadMapLayout(newMap);
    }
    map = newMap;
    emit applied();
    this->close();
}
