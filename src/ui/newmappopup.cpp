#include "newmappopup.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmappopup.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QPalette>
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
    delete ui;
}

void NewMapPopup::initSettings(Project *project) {
    settings.group = project->groupNames.at(0);
    settings.width = project->getDefaultMapSize();
    settings.height = project->getDefaultMapSize();
    settings.borderWidth = DEFAULT_BORDER_WIDTH;
    settings.borderHeight = DEFAULT_BORDER_HEIGHT;
    settings.primaryTileset = project->getDefaultPrimaryTilesetLabel();
    settings.secondaryTileset = project->getDefaultSecondaryTilesetLabel();
    settings.type = project->mapTypes.at(0);
    settings.location = project->mapSectionValueToName.values().at(0);
    settings.song = project->songNames.at(0);
    settings.canFlyTo = false;
    settings.showLocationName = true;
    settings.allowRunning = false;
    settings.allowBiking = false;
    settings.allowEscaping = false;
    settings.floorNumber = 0;
}

void NewMapPopup::init(MapSortOrder type, QVariant data) {
    int groupNum = 0;
    QString mapSec = QString();

    switch (type)
    {
    case MapSortOrder::Group:
        groupNum = data.toInt();
        break;
    case MapSortOrder::Area:
        mapSec = data.toString();
        break;
    case MapSortOrder::Layout:
        this->existingLayout = true;
        this->layoutId = data.toString();
        break;
    }

    populateComboBoxes();
    setDefaultValues(groupNum, mapSec);
    setDefaultValuesProjectConfig();
    connectSignals();
}

void NewMapPopup::initImportMap(MapLayout *mapLayout) {
    this->importedMap = true;
    populateComboBoxes();
    setDefaultValuesImportMap(mapLayout);
    setDefaultValuesProjectConfig();
    connectSignals();
}

void NewMapPopup::populateComboBoxes() {
    ui->comboBox_NewMap_Primary_Tileset->addItems(project->primaryTilesetLabels);
    ui->comboBox_NewMap_Secondary_Tileset->addItems(project->secondaryTilesetLabels);
    ui->comboBox_NewMap_Group->addItems(project->groupNames);
    ui->comboBox_NewMap_Song->addItems(project->songNames);
    ui->comboBox_NewMap_Type->addItems(project->mapTypes);
    ui->comboBox_NewMap_Location->addItems(project->mapSectionValueToName.values());
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

void NewMapPopup::connectSignals() {
    ui->spinBox_NewMap_Width->setMinimum(1);
    ui->spinBox_NewMap_Height->setMinimum(1);
    ui->spinBox_NewMap_Width->setMaximum(project->getMaxMapWidth());
    ui->spinBox_NewMap_Height->setMaximum(project->getMaxMapHeight());

    connect(ui->spinBox_NewMap_Width, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});
    connect(ui->spinBox_NewMap_Height, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});
}

void NewMapPopup::setDefaultValues(int groupNum, QString mapSec) {
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());

    ui->comboBox_NewMap_Group->setTextItem(project->groupNames.at(groupNum));

    if (this->existingLayout) {
        MapLayout * layout = project->mapLayouts.value(layoutId);
        ui->spinBox_NewMap_Width->setValue(layout->width);
        ui->spinBox_NewMap_Height->setValue(layout->height);
        ui->spinBox_NewMap_BorderWidth->setValue(layout->border_width);
        ui->spinBox_NewMap_BorderHeight->setValue(layout->border_height);
        ui->comboBox_NewMap_Primary_Tileset->setTextItem(layout->tileset_primary_label);
        ui->comboBox_NewMap_Secondary_Tileset->setTextItem(layout->tileset_secondary_label);
        ui->spinBox_NewMap_Width->setDisabled(true);
        ui->spinBox_NewMap_Height->setDisabled(true);
        ui->spinBox_NewMap_BorderWidth->setDisabled(true);
        ui->spinBox_NewMap_BorderHeight->setDisabled(true);
        ui->comboBox_NewMap_Primary_Tileset->setDisabled(true);
        ui->comboBox_NewMap_Secondary_Tileset->setDisabled(true);
    } else {
        ui->spinBox_NewMap_Width->setValue(project->getDefaultMapSize());
        ui->spinBox_NewMap_Height->setValue(project->getDefaultMapSize());
        ui->spinBox_NewMap_BorderWidth->setValue(DEFAULT_BORDER_WIDTH);
        ui->spinBox_NewMap_BorderHeight->setValue(DEFAULT_BORDER_HEIGHT);
        ui->comboBox_NewMap_Primary_Tileset->setTextItem(project->getDefaultPrimaryTilesetLabel());
        ui->comboBox_NewMap_Secondary_Tileset->setTextItem(project->getDefaultSecondaryTilesetLabel());
    }

    if (!mapSec.isEmpty()) ui->comboBox_NewMap_Location->setTextItem(mapSec);
    ui->checkBox_NewMap_Show_Location->setChecked(true);

    ui->frame_NewMap_Options->setEnabled(true);
}

void NewMapPopup::setDefaultValuesImportMap(MapLayout *mapLayout) {
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());

    ui->comboBox_NewMap_Group->setTextItem(project->groupNames.at(0));

    ui->spinBox_NewMap_Width->setValue(mapLayout->width);
    ui->spinBox_NewMap_Height->setValue(mapLayout->height);
    ui->spinBox_NewMap_BorderWidth->setValue(mapLayout->border_width);
    ui->spinBox_NewMap_BorderHeight->setValue(mapLayout->border_height);

    ui->comboBox_NewMap_Primary_Tileset->setTextItem(mapLayout->tileset_primary_label);
    ui->comboBox_NewMap_Secondary_Tileset->setTextItem(mapLayout->tileset_secondary_label);

    ui->checkBox_NewMap_Show_Location->setChecked(true);

    ui->frame_NewMap_Options->setEnabled(true);

    this->map = new Map();
    this->map->layout = new MapLayout();
    this->map->layout->blockdata = mapLayout->blockdata;

    if (!mapLayout->border.isEmpty()) {
        this->map->layout->border = mapLayout->border;
    }
}

void NewMapPopup::setDefaultValuesProjectConfig() {
    bool hasFlags = (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby);
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
}

void NewMapPopup::on_lineEdit_NewMap_Name_textChanged(const QString &text) {
    if (project->mapNames.contains(text)) {
        QPalette palette = this->ui->lineEdit_NewMap_Name->palette();
        QColor color = Qt::red;
        color.setAlpha(25);
        palette.setColor(QPalette::Base, color);
        this->ui->lineEdit_NewMap_Name->setPalette(palette);
    } else {
        this->ui->lineEdit_NewMap_Name->setPalette(QPalette());
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
    QString newMapName = this->ui->lineEdit_NewMap_Name->text().remove(QRegularExpression("[^a-zA-Z0-9_]+"));
    newMapName.remove(QRegularExpression("^[0-9]*"));
    if (project->mapNames.contains(newMapName) || newMapName.isEmpty()) {
        newMapName = project->getNewMapName();
    }

    newMap->name = newMapName;
    newMap->type = this->ui->comboBox_NewMap_Type->currentText();
    newMap->location = this->ui->comboBox_NewMap_Location->currentText();
    newMap->song = this->ui->comboBox_NewMap_Song->currentText();
    newMap->requiresFlash = false;
    newMap->weather = this->project->weatherNames.value(0, "WEATHER_NONE");
    newMap->show_location = this->ui->checkBox_NewMap_Show_Location->isChecked();
    newMap->battle_scene = this->project->mapBattleScenes.value(0, "MAP_BATTLE_SCENE_NORMAL");

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

    if (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby) {
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
