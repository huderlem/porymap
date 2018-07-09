#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QString>
#include <QModelIndex>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QAbstractItemModel>
#include "project.h"
#include "map.h"
#include "editor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setStatusBarMessage(QString message, int timeout = 0);

private slots:
    void on_action_Open_Project_triggered();
    void on_mapList_activated(const QModelIndex &index);
    void on_action_Save_Project_triggered();

    void undo();
    void redo();

    void onLoadMapRequested(QString, QString);
    void onMapChanged(Map *map);
    void onMapNeedsRedrawing(Map *map);

    void on_action_Save_triggered();
    void on_tabWidget_2_currentChanged(int index);
    void on_action_Exit_triggered();
    void on_comboBox_Song_activated(const QString &arg1);
    void on_comboBox_Location_activated(const QString &arg1);
    void on_comboBox_Visibility_activated(const QString &arg1);
    void on_comboBox_Weather_activated(const QString &arg1);
    void on_comboBox_Type_activated(const QString &arg1);
    void on_comboBox_BattleScene_activated(const QString &arg1);
    void on_checkBox_ShowLocation_clicked(bool checked);

    void on_tabWidget_currentChanged(int index);

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_toolButton_deleteObject_clicked();

    void addNewEvent(QString);
    void updateSelectedObjects();

    void on_toolButton_Paint_clicked();

    void on_toolButton_Select_clicked();

    void on_toolButton_Fill_clicked();

    void on_toolButton_Dropper_clicked();

    void onOpenMapListContextMenu(const QPoint &point);
    void onAddNewMapToGroupClick(QAction* triggeredAction);
    void onTilesetChanged(QString);

    void on_action_Export_Map_Image_triggered();

    void on_comboBox_ConnectionDirection_currentIndexChanged(const QString &arg1);

    void on_spinBox_ConnectionOffset_valueChanged(int offset);

    void on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName);

    void on_pushButton_AddConnection_clicked();

    void on_pushButton_RemoveConnection_clicked();

    void on_comboBox_DiveMap_currentTextChanged(const QString &mapName);

    void on_comboBox_EmergeMap_currentTextChanged(const QString &mapName);

    void on_comboBox_PrimaryTileset_activated(const QString &arg1);

    void on_comboBox_SecondaryTileset_activated(const QString &arg1);

    void on_pushButton_clicked();

    void on_checkBox_smartPaths_stateChanged(int selected);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *mapListModel;
    QList<QStandardItem*> *mapGroupsModel;
    Editor *editor = NULL;
    QIcon* mapIcon;
    void setMap(QString);
    void redrawMapScene();
    void loadDataStructures();
    void populateMapList();
    QString getExistingDirectory(QString);
    void openProject(QString dir);
    QString getDefaultMap();
    void setRecentMap(QString map_name);
    QStandardItem* createMapItem(QString mapName, int groupNum, int inGroupNum);

    void markAllEdited(QAbstractItemModel *model);
    void markEdited(QModelIndex index);
    void updateMapList();

    void displayMapProperties();
    void checkToolButtons();
};

enum MapListUserRoles {
    GroupRole = Qt::UserRole + 1, // Used to hold the map group number.
    TypeRole = Qt::UserRole + 2,  // Used to differentiate between the different layers of the map list tree view.
};

#endif // MAINWINDOW_H
