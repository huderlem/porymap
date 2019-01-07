#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QString>
#include <QModelIndex>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QCloseEvent>
#include <QAbstractItemModel>
#include "project.h"
#include "config.h"
#include "map.h"
#include "editor.h"
#include "tileseteditor.h"
#include "regionmapeditor.h"
#include "filterchildrenproxymodel.h"
#include "newmappopup.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void scaleMapView(int);

private slots:
    void on_action_Open_Project_triggered();
    void on_mapList_activated(const QModelIndex &index);
    void on_action_Save_Project_triggered();
    void openWarpMap(QString map_name, QString warp_num);

    void undo();
    void redo();

    void openInTextEditor();

    void onLoadMapRequested(QString, QString);
    void onMapChanged(Map *map);
    void onMapNeedsRedrawing();
    void onTilesetsSaved(QString, QString);
    void openNewMapPopupWindow(int, QVariant);
    void onNewMapCreated();

    void on_action_NewMap_triggered();
    void on_action_Save_triggered();
    void on_tabWidget_2_currentChanged(int index);
    void on_action_Exit_triggered();
    void on_comboBox_Song_activated(const QString &arg1);
    void on_comboBox_Location_activated(const QString &arg1);
    void on_comboBox_Weather_activated(const QString &arg1);
    void on_comboBox_Type_activated(const QString &arg1);
    void on_comboBox_BattleScene_activated(const QString &arg1);
    void on_checkBox_ShowLocation_clicked(bool checked);
    void on_checkBox_AllowRunning_clicked(bool checked);
    void on_checkBox_AllowBiking_clicked(bool checked);
    void on_checkBox_AllowEscapeRope_clicked(bool checked);

    void on_tabWidget_currentChanged(int index);

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionBetter_Cursors_triggered();
    void on_actionPlayer_View_Rectangle_triggered();
    void on_actionCursor_Tile_Outline_triggered();
    void on_actionPencil_triggered();
    void on_actionPointer_triggered();
    void on_actionFlood_Fill_triggered();
    void on_actionEyedropper_triggered();
    void on_actionMove_triggered();
    void on_actionMap_Shift_triggered();

    void on_toolButton_deleteObject_clicked();
    void on_toolButton_Open_Scripts_clicked();

    void addNewEvent(QString);
    void updateSelectedObjects();
    void updateObjects();

    void on_toolButton_Paint_clicked();

    void on_toolButton_Select_clicked();

    void on_toolButton_Fill_clicked();

    void on_toolButton_Dropper_clicked();

    void on_toolButton_Move_clicked();

    void on_toolButton_Shift_clicked();

    void onOpenMapListContextMenu(const QPoint &point);
    void onAddNewMapToGroupClick(QAction* triggeredAction);
    void onAddNewMapToAreaClick(QAction* triggeredAction);
    void onAddNewMapToLayoutClick(QAction* triggeredAction);
    void onTilesetChanged(QString);
    void currentMetatilesSelectionChanged();

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

    void on_checkBox_Visibility_clicked(bool checked);

    void on_checkBox_ToggleBorder_stateChanged(int arg1);

    void resetMapViewScale();

    void on_actionTileset_Editor_triggered();

    void mapSortOrder_changed(QAction *action);

    void on_lineEdit_filterBox_textChanged(const QString &arg1);

    void closeEvent(QCloseEvent *);

    void eventTabChanged(int index);

    void selectedEventIndexChanged(int index);

    void on_horizontalSlider_CollisionTransparency_valueChanged(int value);
    void on_toolButton_ExpandAll_clicked();
    void on_toolButton_CollapseAll_clicked();
    void on_actionAbout_Porymap_triggered();
    void on_pushButton_AddCustomHeaderField_clicked();
    void on_pushButton_DeleteCustomHeaderField_clicked();
    void on_tableWidget_CustomHeaderFields_cellChanged(int row, int column);
    void on_horizontalSlider_MetatileZoom_valueChanged(int value);

    void on_actionRegion_Map_Editor_triggered();

private:
    Ui::MainWindow *ui;
    TilesetEditor *tilesetEditor = nullptr;
    RegionMapEditor *regionMapEditor = nullptr;
    FilterChildrenProxyModel *mapListProxyModel;
    NewMapPopup *newmapprompt = nullptr;
    QStandardItemModel *mapListModel;
    QList<QStandardItem*> *mapGroupItemsList;
    QMap<QString, QModelIndex> mapListIndexes;
    Editor *editor = nullptr;
    QIcon* mapIcon;
    QIcon* mapEditedIcon;

    QWidget *eventTabObjectWidget;
    QWidget *eventTabWarpWidget;
    QWidget *eventTabTriggerWidget;
    QWidget *eventTabBGWidget;
    QWidget *eventTabHealspotWidget;
    QWidget *eventTabMultipleWidget;

    DraggablePixmapItem *selectedObject;
    DraggablePixmapItem *selectedWarp;
    DraggablePixmapItem *selectedTrigger;
    DraggablePixmapItem *selectedBG;
    DraggablePixmapItem *selectedHealspot;

    bool isProgrammaticEventTabChange;

    MapSortOrder mapSortOrder;

    bool setMap(QString, bool scrollTreeView = false);
    void redrawMapScene();
    void loadDataStructures();
    void populateMapList();
    void sortMapList();
    QString getExistingDirectory(QString);
    bool openProject(QString dir);
    QString getDefaultMap();
    void setRecentMap(QString map_name);
    QStandardItem* createMapItem(QString mapName, int groupNum, int inGroupNum);

    void markAllEdited(QAbstractItemModel *model);
    void markEdited(QModelIndex index);
    void updateMapList();

    void displayMapProperties();
    void checkToolButtons();

    void initWindow();
    void initCustomUI();
    void initExtraShortcuts();
    void initExtraSignals();
    void initEditor();
    void initMiscHeapObjects();
    void initMapSortOrder();
    void setProjectSpecificUIVisibility();
    void loadUserSettings();
    void restoreWindowState();
    bool openRecentProject();
    void updateTilesetEditor();
    QString getEventGroupFromTabWidget(QWidget *tab);

    bool isProjectOpen();
};

enum MapListUserRoles {
    GroupRole = Qt::UserRole + 1, // Used to hold the map group number.
    TypeRole,  // Used to differentiate between the different layers of the map list tree view.
    TypeRole2, // Used for various extra data needed.
};

#endif // MAINWINDOW_H
