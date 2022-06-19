#include "newmappopup.h"
#include "event.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmappopup.h"
#include "config.h"

#include <QMap>
#include <QSet>
#include <QPalette>
#include <QStringList>

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

void NewMapPopup::init(int type, int group, QString sec, QString layoutId) {
    switch (type)
    {
        case MapSortOrder::Group:
            setDefaultValues(group, QString());
            break;
        case MapSortOrder::Area:
            setDefaultValues(group, sec);
            break;
        case MapSortOrder::Layout:
            useLayout(layoutId);
            setDefaultValues(group, QString());
            break;
    }
    connectSignals();
}

void NewMapPopup::initImportMap(MapLayout *mapLayout) {
    this->importedMap = true;
    setDefaultValuesImportMap(mapLayout);
    connectSignals();
}

bool NewMapPopup::checkNewMapDimensions() {
    int numMetatiles = project->getMapDataSize(ui->spinBox_NewMap_Width->value(), ui->spinBox_NewMap_Height->value());
    int maxMetatiles = project->getMaxMapDataSize();

    if (numMetatiles > maxMetatiles) {
        ui->frame_NewMap_Warning->setVisible(true);
        //ui->label_NewMap_WarningMessage->setText("WARNING: The specified map dimensions are too large.");
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

    //ui->icon_NewMap_WarningIcon->setPixmap();
    connect(ui->spinBox_NewMap_Width, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});
    connect(ui->spinBox_NewMap_Height, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){checkNewMapDimensions();});
}

void NewMapPopup::useLayout(QString layoutId) {
    this->existingLayout = true;
    this->layoutId = layoutId;
}

void NewMapPopup::setDefaultValues(int groupNum, QString mapSec) {
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());

    QMap<QString, QStringList> tilesets = project->getTilesetLabels();
    ui->comboBox_NewMap_Primary_Tileset->addItems(tilesets.value("primary"));
    ui->comboBox_NewMap_Secondary_Tileset->addItems(tilesets.value("secondary"));

    ui->comboBox_NewMap_Group->addItems(project->groupNames);
    ui->comboBox_NewMap_Group->setCurrentText(project->groupNames.at(groupNum));

    ui->comboBox_Song->addItems(project->songNames);

    if (existingLayout) {
        ui->spinBox_NewMap_Width->setValue(project->mapLayouts.value(layoutId)->width.toInt(nullptr, 0));
        ui->spinBox_NewMap_Height->setValue(project->mapLayouts.value(layoutId)->height.toInt(nullptr, 0));
        ui->comboBox_NewMap_Primary_Tileset->setCurrentText(project->mapLayouts.value(layoutId)->tileset_primary_label);
        ui->comboBox_NewMap_Secondary_Tileset->setCurrentText(project->mapLayouts.value(layoutId)->tileset_secondary_label);
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
    }

    ui->comboBox_NewMap_Type->addItems(project->mapTypes);
    ui->comboBox_NewMap_Location->addItems(project->mapSectionValueToName.values());
    if (!mapSec.isEmpty()) ui->comboBox_NewMap_Location->setCurrentText(mapSec);
    ui->checkBox_NewMap_Show_Location->setChecked(true);

    ui->frame_NewMap_Options->setEnabled(true);

    setDefaultValuesProjectConfig(false, NULL);
}

void NewMapPopup::setDefaultValuesImportMap(MapLayout *mapLayout) {
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());

    QMap<QString, QStringList> tilesets = project->getTilesetLabels();
    ui->comboBox_NewMap_Primary_Tileset->addItems(tilesets.value("primary"));
    ui->comboBox_NewMap_Secondary_Tileset->addItems(tilesets.value("secondary"));

    ui->comboBox_NewMap_Group->addItems(project->groupNames);
    ui->comboBox_NewMap_Group->setCurrentText(project->groupNames.at(0));

    ui->comboBox_Song->addItems(project->songNames);

    ui->spinBox_NewMap_Width->setValue(mapLayout->width.toInt(nullptr, 0));
    ui->spinBox_NewMap_Height->setValue(mapLayout->height.toInt(nullptr, 0));
    ui->comboBox_NewMap_Primary_Tileset->setCurrentText(mapLayout->tileset_primary_label);
    ui->comboBox_NewMap_Secondary_Tileset->setCurrentText(mapLayout->tileset_secondary_label);

    ui->comboBox_NewMap_Type->addItems(project->mapTypes);
    ui->comboBox_NewMap_Location->addItems(project->mapSectionValueToName.values());
    ui->checkBox_NewMap_Show_Location->setChecked(true);

    ui->frame_NewMap_Options->setEnabled(true);

    setDefaultValuesProjectConfig(true, mapLayout);

    map = new Map();
    map->layout = new MapLayout();
    map->layout->blockdata = mapLayout->blockdata;

    if (!mapLayout->border.isEmpty()) {
        map->layout->border = mapLayout->border;
    }
}

void NewMapPopup::setDefaultValuesProjectConfig(bool importedMap, MapLayout *mapLayout) {
    switch (projectConfig.getBaseGameVersion())
    {
    case BaseGameVersion::pokeruby:
        ui->checkBox_NewMap_Allow_Running->setVisible(false);
        ui->checkBox_NewMap_Allow_Biking->setVisible(false);
        ui->checkBox_NewMap_Allow_Escape_Rope->setVisible(false);
        ui->label_NewMap_Allow_Running->setVisible(false);
        ui->label_NewMap_Allow_Biking->setVisible(false);
        ui->label_NewMap_Allow_Escape_Rope->setVisible(false);
        break;
    case BaseGameVersion::pokeemerald:
        ui->checkBox_NewMap_Allow_Running->setVisible(true);
        ui->checkBox_NewMap_Allow_Biking->setVisible(true);
        ui->checkBox_NewMap_Allow_Escape_Rope->setVisible(true);
        ui->label_NewMap_Allow_Running->setVisible(true);
        ui->label_NewMap_Allow_Biking->setVisible(true);
        ui->label_NewMap_Allow_Escape_Rope->setVisible(true);
        break;
    case BaseGameVersion::pokefirered:
        ui->checkBox_NewMap_Allow_Running->setVisible(true);
        ui->checkBox_NewMap_Allow_Biking->setVisible(true);
        ui->checkBox_NewMap_Allow_Escape_Rope->setVisible(true);
        ui->label_NewMap_Allow_Running->setVisible(true);
        ui->label_NewMap_Allow_Biking->setVisible(true);
        ui->label_NewMap_Allow_Escape_Rope->setVisible(true);
        break;
    }
    if (projectConfig.getUseCustomBorderSize()) {
        if (importedMap) {
            ui->spinBox_NewMap_BorderWidth->setValue(mapLayout->border_width.toInt(nullptr, 0));
            ui->spinBox_NewMap_BorderHeight->setValue(mapLayout->border_height.toInt(nullptr, 0));
        }
        ui->spinBox_NewMap_BorderWidth->setVisible(true);
        ui->spinBox_NewMap_BorderHeight->setVisible(true);
        ui->label_NewMap_BorderWidth->setVisible(true);
        ui->label_NewMap_BorderHeight->setVisible(true);
    } else {
        if (importedMap) {
            ui->spinBox_NewMap_BorderWidth->setValue(DEFAULT_BORDER_WIDTH);
            ui->spinBox_NewMap_BorderHeight->setValue(DEFAULT_BORDER_HEIGHT);
        }
        ui->spinBox_NewMap_BorderWidth->setVisible(false);
        ui->spinBox_NewMap_BorderHeight->setVisible(false);
        ui->label_NewMap_BorderWidth->setVisible(false);
        ui->label_NewMap_BorderHeight->setVisible(false);
    }
    if (projectConfig.getFloorNumberEnabled()) {
        ui->spinBox_NewMap_Floor_Number->setVisible(true);
        ui->label_NewMap_Floor_Number->setVisible(true);
    } else {
        ui->spinBox_NewMap_Floor_Number->setVisible(false);
        ui->label_NewMap_Floor_Number->setVisible(false);
    }
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
    newMap->song = this->ui->comboBox_Song->currentText();
    newMap->requiresFlash = "0";
    newMap->weather = this->project->weatherNames.value(0, "WEATHER_NONE");
    newMap->show_location = this->ui->checkBox_NewMap_Show_Location->isChecked() ? "1" : "0";
    newMap->battle_scene = this->project->mapBattleScenes.value(0, "MAP_BATTLE_SCENE_NORMAL");

    if (this->existingLayout) {
        layout = this->project->mapLayouts.value(this->layoutId);
        newMap->needsLayoutDir = false;
    } else {
        layout = new MapLayout;
        layout->id = MapLayout::layoutConstantFromName(newMapName);
        layout->name = QString("%1_Layout").arg(newMap->name);
        layout->width = QString::number(this->ui->spinBox_NewMap_Width->value());
        layout->height = QString::number(this->ui->spinBox_NewMap_Height->value());
        if (projectConfig.getUseCustomBorderSize()) {
            layout->border_width = QString::number(this->ui->spinBox_NewMap_BorderWidth->value());
            layout->border_height = QString::number(this->ui->spinBox_NewMap_BorderHeight->value());
        } else {
            layout->border_width = QString::number(DEFAULT_BORDER_WIDTH);
            layout->border_height = QString::number(DEFAULT_BORDER_HEIGHT);
        }
        layout->tileset_primary_label = this->ui->comboBox_NewMap_Primary_Tileset->currentText();
        layout->tileset_secondary_label = this->ui->comboBox_NewMap_Secondary_Tileset->currentText();
        layout->border_path = QString("data/layouts/%1/border.bin").arg(newMapName);
        layout->blockdata_path = QString("data/layouts/%1/map.bin").arg(newMapName);
    }

    if (this->importedMap) {
        layout->blockdata = map->layout->blockdata;
        if (!map->layout->border.isEmpty())
            layout->border = map->layout->border;
    }

    if (this->ui->checkBox_NewMap_Flyable->isChecked()) {
        newMap->isFlyable = "TRUE";
    }

    if (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby) {
        newMap->allowRunning = this->ui->checkBox_NewMap_Allow_Running->isChecked() ? "1" : "0";
        newMap->allowBiking = this->ui->checkBox_NewMap_Allow_Biking->isChecked() ? "1" : "0";
        newMap->allowEscapeRope = this->ui->checkBox_NewMap_Allow_Escape_Rope->isChecked() ? "1" : "0";
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
