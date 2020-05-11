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
#include <QJSValue>
#include "project.h"
#include "config.h"
#include "map.h"
#include "editor.h"
#include "tileseteditor.h"
#include "regionmapeditor.h"
#include "mapimageexporter.h"
#include "filterchildrenproxymodel.h"
#include "newmappopup.h"
#include "newtilesetdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent);
    ~MainWindow();

    MainWindow() = delete;
    MainWindow(const MainWindow &) = delete;
    MainWindow & operator = (const MainWindow &) = delete;

    // Scripting API
    Q_INVOKABLE QJSValue getBlock(int x, int y);
    void tryRedrawMapArea(bool forceRedraw);
    void tryCommitMapChanges(bool commitChanges);
    Q_INVOKABLE void setBlock(int x, int y, int tile, int collision, int elevation, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void setBlocksFromSelection(int x, int y, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE int getMetatileId(int x, int y);
    Q_INVOKABLE void setMetatileId(int x, int y, int metatileId, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE int getCollision(int x, int y);
    Q_INVOKABLE void setCollision(int x, int y, int collision, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE int getElevation(int x, int y);
    Q_INVOKABLE void setElevation(int x, int y, int elevation, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void bucketFill(int x, int y, int metatileId, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void bucketFillFromSelection(int x, int y, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void magicFill(int x, int y, int metatileId, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void magicFillFromSelection(int x, int y, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void shift(int xDelta, int yDelta, bool forceRedraw = true, bool commitChanges = true);
    Q_INVOKABLE void redraw();
    Q_INVOKABLE void commit();
    Q_INVOKABLE QJSValue getDimensions();
    Q_INVOKABLE int getWidth();
    Q_INVOKABLE int getHeight();
    Q_INVOKABLE void setDimensions(int width, int height);
    Q_INVOKABLE void setWidth(int width);
    Q_INVOKABLE void setHeight(int height);
    Q_INVOKABLE void clearOverlay();
    Q_INVOKABLE void addText(QString text, int x, int y, QString color = "#000000", int fontSize = 12);
    Q_INVOKABLE void addRect(int x, int y, int width, int height, QString color = "#000000");
    Q_INVOKABLE void addFilledRect(int x, int y, int width, int height, QString color = "#000000");
    Q_INVOKABLE void addImage(int x, int y, QString filepath);
    void refreshAfterPaletteChange(Tileset *tileset);
    void setTilesetPalette(Tileset *tileset, int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setPrimaryTilesetPalette(int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setPrimaryTilesetPalettes(QList<QList<QList<int>>> palettes);
    Q_INVOKABLE void setSecondaryTilesetPalette(int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setSecondaryTilesetPalettes(QList<QList<QList<int>>> palettes);
    QJSValue getTilesetPalette(QList<QList<QRgb>> *palettes, int paletteIndex);
    QJSValue getTilesetPalettes(QList<QList<QRgb>> *palettes);
    Q_INVOKABLE QJSValue getPrimaryTilesetPalette(int paletteIndex);
    Q_INVOKABLE QJSValue getPrimaryTilesetPalettes();
    Q_INVOKABLE QJSValue getSecondaryTilesetPalette(int paletteIndex);
    Q_INVOKABLE QJSValue getSecondaryTilesetPalettes();
    void refreshAfterPalettePreviewChange();
    void setTilesetPalettePreview(Tileset *tileset, int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setPrimaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setPrimaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes);
    Q_INVOKABLE void setSecondaryTilesetPalettePreview(int paletteIndex, QList<QList<int>> colors);
    Q_INVOKABLE void setSecondaryTilesetPalettesPreview(QList<QList<QList<int>>> palettes);
    Q_INVOKABLE QJSValue getPrimaryTilesetPalettePreview(int paletteIndex);
    Q_INVOKABLE QJSValue getPrimaryTilesetPalettesPreview();
    Q_INVOKABLE QJSValue getSecondaryTilesetPalettePreview(int paletteIndex);
    Q_INVOKABLE QJSValue getSecondaryTilesetPalettesPreview();
    Q_INVOKABLE QString getPrimaryTileset();
    Q_INVOKABLE QString getSecondaryTileset();
    Q_INVOKABLE void setPrimaryTileset(QString tileset);
    Q_INVOKABLE void setSecondaryTileset(QString tileset);
    Q_INVOKABLE void setGridVisibility(bool visible);
    Q_INVOKABLE bool getGridVisibility();
    Q_INVOKABLE void setBorderVisibility(bool visible);
    Q_INVOKABLE bool getBorderVisibility();
    Q_INVOKABLE void setSmartPathsEnabled(bool visible);
    Q_INVOKABLE bool getSmartPathsEnabled();
    Q_INVOKABLE void registerAction(QString functionName, QString actionName, QString shortcut = "");
    Q_INVOKABLE void setTimeout(QJSValue callback, int milliseconds);
    void invokeCallback(QJSValue callback);
    Q_INVOKABLE void log(QString message);


public slots:
    void scaleMapView(int);

private slots:
    void on_action_Open_Project_triggered();
    void on_action_Reload_Project_triggered();
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
    void onWildMonDataChanged();
    void openNewMapPopupWindow(int, QVariant);
    void onNewMapCreated();

    void on_action_NewMap_triggered();
    void on_actionNew_Tileset_triggered();
    void on_action_Save_triggered();
    void on_tabWidget_2_currentChanged(int index);
    void on_action_Exit_triggered();
    void on_comboBox_Song_currentTextChanged(const QString &arg1);
    void on_comboBox_Location_currentTextChanged(const QString &arg1);
    void on_comboBox_Weather_currentTextChanged(const QString &arg1);
    void on_comboBox_Type_currentTextChanged(const QString &arg1);
    void on_comboBox_BattleScene_currentTextChanged(const QString &arg1);
    void on_checkBox_ShowLocation_clicked(bool checked);
    void on_checkBox_AllowRunning_clicked(bool checked);
    void on_checkBox_AllowBiking_clicked(bool checked);
    void on_checkBox_AllowEscapeRope_clicked(bool checked);
    void on_spinBox_FloorNumber_valueChanged(int offset);
    void on_actionUse_Encounter_Json_triggered(bool checked);
    void on_actionMonitor_Project_Files_triggered(bool checked);
    void on_actionUse_Poryscript_triggered(bool checked);

    void on_mainTabBar_tabBarClicked(int index);

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
    void on_actionExport_Stitched_Map_Image_triggered();

    void on_comboBox_ConnectionDirection_currentIndexChanged(const QString &arg1);
    void on_spinBox_ConnectionOffset_valueChanged(int offset);
    void on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName);
    void on_pushButton_AddConnection_clicked();
    void on_pushButton_RemoveConnection_clicked();
    void on_comboBox_DiveMap_currentTextChanged(const QString &mapName);
    void on_comboBox_EmergeMap_currentTextChanged(const QString &mapName);
    void on_comboBox_PrimaryTileset_currentTextChanged(const QString &arg1);
    void on_comboBox_SecondaryTileset_currentTextChanged(const QString &arg1);
    void on_pushButton_ChangeDimensions_clicked();
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
    void on_actionThemes_triggered();
    void on_pushButton_AddCustomHeaderField_clicked();
    void on_pushButton_DeleteCustomHeaderField_clicked();
    void on_tableWidget_CustomHeaderFields_cellChanged(int row, int column);
    void on_horizontalSlider_MetatileZoom_valueChanged(int value);
    void on_pushButton_NewWildMonGroup_clicked();
    void on_pushButton_ConfigureEncountersJSON_clicked();

    void on_actionRegion_Map_Editor_triggered();

private:
    Ui::MainWindow *ui;
    TilesetEditor *tilesetEditor = nullptr;
    RegionMapEditor *regionMapEditor = nullptr;
    MapImageExporter *mapImageExporter = nullptr;
    FilterChildrenProxyModel *mapListProxyModel;
    NewMapPopup *newmapprompt = nullptr;
    QStandardItemModel *mapListModel;
    QList<QStandardItem*> *mapGroupItemsList;
    QMap<QString, QModelIndex> mapListIndexes;
    Editor *editor = nullptr;
    QIcon* mapIcon;
    QIcon* mapEditedIcon;
    QIcon* mapOpenedIcon;

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

    QList<QAction *> registeredActions;

    bool isProgrammaticEventTabChange;
    bool projectHasUnsavedChanges;

    MapSortOrder mapSortOrder;

    bool setMap(QString, bool scrollTreeView = false);
    void redrawMapScene();
    bool loadDataStructures();
    bool loadProjectCombos();
    bool populateMapList();
    void sortMapList();
    QString getExistingDirectory(QString);
    bool openProject(QString dir);
    QString getDefaultMap();
    void setRecentMap(QString map_name);
    QStandardItem* createMapItem(QString mapName, int groupNum, int inGroupNum);
    static bool mapDimensionsValid(int width, int height);

    void drawMapListIcons(QAbstractItemModel *model);
    void updateMapList();

    void displayMapProperties();
    void checkToolButtons();
    void clickToolButtonFromEditMode(QString editMode);

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
    void setTheme(QString);
    bool openRecentProject();
    void updateTilesetEditor();
    QString getEventGroupFromTabWidget(QWidget *tab);
    void closeSupplementaryWindows();

    bool isProjectOpen();
    void showExportMapImageWindow(bool stitchMode);
};

enum MapListUserRoles {
    GroupRole = Qt::UserRole + 1, // Used to hold the map group number.
    TypeRole,  // Used to differentiate between the different layers of the map list tree view.
    TypeRole2, // Used for various extra data needed.
};

#endif // MAINWINDOW_H
