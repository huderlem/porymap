#ifndef MAPHEADERFORM_H
#define MAPHEADERFORM_H

#include "project.h"
#include "map.h"
#include "ui_mapheaderform.h"

#include <QWidget>

/*
    This is the UI class used to edit the fields in a map's header.
    It's intended to be used anywhere the UI needs to present an editor for a map's header,
    e.g. for the current map in the main editor or in the new map dialog.
*/

namespace Ui {
class MapHeaderForm;
}

class MapHeaderForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapHeaderForm(QWidget *parent = nullptr);
    ~MapHeaderForm();

    void setProject(Project * project);
    void setMap(Map * map);

    void clearDisplay();
    void clear();

    void refreshLocationsComboBox();

    Ui::MapHeaderForm *ui;

private:
    QPointer<Map> map = nullptr;
    QPointer<Project> project = nullptr;

private slots:
    void on_comboBox_Song_currentTextChanged(const QString &);
    void on_comboBox_Location_currentTextChanged(const QString &);
    void on_comboBox_Weather_currentTextChanged(const QString &);
    void on_comboBox_Type_currentTextChanged(const QString &);
    void on_comboBox_BattleScene_currentTextChanged(const QString &);
    void on_checkBox_RequiresFlash_stateChanged(int);
    void on_checkBox_ShowLocationName_stateChanged(int);
    void on_checkBox_AllowRunning_stateChanged(int);
    void on_checkBox_AllowBiking_stateChanged(int);
    void on_checkBox_AllowEscaping_stateChanged(int);
    void on_spinBox_FloorNumber_valueChanged(int);
};

#endif // MAPHEADERFORM_H
