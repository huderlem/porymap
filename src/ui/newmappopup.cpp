#include "newmappopup.h"
#include "event.h"
#include "maplayout.h"
#include "mainwindow.h"
#include "ui_newmappopup.h"

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
}

NewMapPopup::~NewMapPopup()
{
    delete ui;
}

void NewMapPopup::init(int group) {
    setDefaultValues(group);
}

void NewMapPopup::setDefaultValues(int groupNum) {
    ui->lineEdit_NewMap_Name->setText(project->getNewMapName());

    QMap<QString, QStringList> tilesets = project->getTilesets();
    ui->comboBox_NewMap_Primary_Tileset->addItems(tilesets.value("primary"));
    ui->comboBox_NewMap_Secondary_Tileset->addItems(tilesets.value("secondary"));

    ui->comboBox_NewMap_Group->addItems(*project->groupNames);
    ui->comboBox_NewMap_Group->setCurrentText("gMapGroup" + QString::number(groupNum));

    ui->spinBox_NewMap_Width->setValue(20);
    ui->spinBox_NewMap_Height->setValue(20);

    ui->comboBox_NewMap_Type->addItems(*project->mapTypes);
    ui->comboBox_NewMap_Location->addItems(*project->regionMapSections);

    ui->frame_NewMap_Options->setEnabled(true);
}

void NewMapPopup::on_lineEdit_NewMap_Name_textChanged(const QString &text) {
    if (project->mapNames->contains(text)) {
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
    Map *newMap = new Map;
    MapLayout *layout = new MapLayout;

    // If map name is not unique, use default value. Also replace spaces with underscores.
    QString newMapName = this->ui->lineEdit_NewMap_Name->text().replace(" ","_");
    if (project->mapNames->contains(newMapName)) {
        newMapName = project->getNewMapName();
    }
    newMap->name = newMapName;

    newMap->type = this->ui->comboBox_NewMap_Type->currentText();
    newMap->location = this->ui->comboBox_NewMap_Location->currentText();

    layout->width = QString::number(this->ui->spinBox_NewMap_Width->value());
    layout->height = QString::number(this->ui->spinBox_NewMap_Height->value());
    layout->tileset_primary_label = this->ui->comboBox_NewMap_Primary_Tileset->currentText();
    layout->tileset_secondary_label = this->ui->comboBox_NewMap_Secondary_Tileset->currentText();
    layout->label = QString("%1_Layout").arg(newMap->name);
    layout->name = MapLayout::getNameFromLabel(layout->label);
    layout->border_label = QString("%1_MapBorder").arg(newMap->name);
    layout->border_path = QString("data/layouts/%1/border.bin").arg(newMap->name);
    layout->blockdata_label = QString("%1_MapBlockdata").arg(newMap->name);
    layout->blockdata_path = QString("data/layouts/%1/map.bin").arg(newMap->name);

    // EMERGENCY (MUST FIX):causes a segfault...
    // how do I add new event to newMap without behavior being undefined???
    if (this->ui->checkBox_NewMap_Flyable->isChecked()) {
        Event *healSpot = new Event;
        healSpot = Event::createNewEvent("event_heal_location", newMapName);
        healSpot->put("map_name", newMapName);
        newMap->addEvent(healSpot);
    }

    newMap->layout = layout;
    newMap->layout_label = layout->label;

    map = newMap;
    group = this->ui->comboBox_NewMap_Group->currentText().remove("gMapGroup").toInt();

    emit applied();

    this->close();
}
