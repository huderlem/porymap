#ifndef REGIONMAPEDITOR_H
#define REGIONMAPEDITOR_H

#include "regionmappixmapitem.h"
#include "citymappixmapitem.h"
#include "regionmaplayoutpixmapitem.h"
#include "regionmapentriespixmapitem.h"
#include "regionmap.h"
#include "history.h"
#include "historyitem.h"

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QCloseEvent>
#include <QResizeEvent>

namespace Ui {
class RegionMapEditor;
}

class RegionMapEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit RegionMapEditor(QWidget *parent = 0, Project *pro = nullptr);
    ~RegionMapEditor();

    RegionMap *region_map;

    bool loadRegionMapData();
    bool loadCityMaps();
    void setCurrentSquareOptions();

    void onRegionMapTileSelectorSelectedTileChanged(unsigned id);
    void onCityMapTileSelectorSelectedTileChanged(unsigned id);
    void onRegionMapTileSelectorHoveredTileChanged(unsigned id);
    void onRegionMapTileSelectorHoveredTileCleared();

    void onRegionMapLayoutSelectedTileChanged(int index);
    void onRegionMapLayoutHoveredTileChanged(int index);
    void onRegionMapLayoutHoveredTileCleared();

    void onRegionMapEntriesSelectedTileChanged(QString) {};
    void onRegionMapEntryDragged(int, int);

    void undo();
    void redo();

    void resize(int width, int height);

private:
    Ui::RegionMapEditor *ui;
    Project *project;

    History<RegionMapHistoryItem*> history;

    int currIndex;
    unsigned selectedCityTile;
    unsigned selectedImageTile;
    QString activeEntry;

    bool hasUnsavedChanges = false;
    bool cityMapFirstDraw = true;
    bool regionMapFirstDraw = true;
    bool entriesFirstDraw = true;

    double scaleUpFactor = 2.0;
    double initialScale = 30.0;

    QGraphicsScene *scene_region_map_image   = nullptr;
    QGraphicsScene *scene_city_map_image     = nullptr;
    QGraphicsScene *scene_region_map_layout  = nullptr;
    QGraphicsScene *scene_region_map_entries = nullptr;
    QGraphicsScene *scene_region_map_tiles   = nullptr;
    QGraphicsScene *scene_city_map_tiles     = nullptr;

    TilemapTileSelector *mapsquare_selector_item = nullptr;
    TilemapTileSelector *city_map_selector_item  = nullptr;

    RegionMapEntriesPixmapItem *region_map_entries_item = nullptr;
    RegionMapLayoutPixmapItem *region_map_layout_item = nullptr;
    RegionMapPixmapItem *region_map_item = nullptr;
    CityMapPixmapItem *city_map_item = nullptr;

    void displayRegionMap();
    void displayRegionMapImage();
    void displayRegionMapLayout();
    void displayRegionMapEntriesImage();
    void displayRegionMapLayoutOptions();
    void updateRegionMapLayoutOptions(int index);
    void displayRegionMapTileSelector();
    void displayCityMapTileSelector();
    void displayCityMap(QString name);
    void displayRegionMapEntryOptions();
    void updateRegionMapEntryOptions(QString);
    void importTileImage(bool city = false);

    bool createCityMap(QString name);
    bool tryInsertNewMapEntry(QString);

    void closeEvent(QCloseEvent* event);

private slots:
    void on_action_RegionMap_Save_triggered();
    void on_action_RegionMap_Undo_triggered();
    void on_action_RegionMap_Redo_triggered();
    void on_action_RegionMap_Resize_triggered();
    void on_action_RegionMap_ClearImage_triggered();
    void on_action_RegionMap_ClearLayout_triggered();
    void on_action_Swap_triggered();
    void on_action_Import_RegionMap_ImageTiles_triggered();
    void on_action_Import_CityMap_ImageTiles_triggered();
    void on_tabWidget_Region_Map_currentChanged(int);
    void on_pushButton_RM_Options_delete_clicked();
    void on_comboBox_RM_ConnectedMap_activated(const QString &);
    void on_comboBox_RM_Entry_MapSection_activated(const QString &);
    void on_spinBox_RM_Entry_x_valueChanged(int);
    void on_spinBox_RM_Entry_y_valueChanged(int);
    void on_spinBox_RM_Entry_width_valueChanged(int);
    void on_spinBox_RM_Entry_height_valueChanged(int);
    void on_pushButton_CityMap_add_clicked();
    void on_verticalSlider_Zoom_Map_Image_valueChanged(int);
    void on_verticalSlider_Zoom_Image_Tiles_valueChanged(int);
    void on_verticalSlider_Zoom_City_Map_valueChanged(int);
    void on_verticalSlider_Zoom_City_Tiles_valueChanged(int);
    void on_comboBox_CityMap_picker_currentTextChanged(const QString &);
    void on_lineEdit_RM_MapName_textEdited(const QString &);
    void onHoveredRegionMapTileChanged(int x, int y);
    void onHoveredRegionMapTileCleared();
    void mouseEvent_region_map(QGraphicsSceneMouseEvent *event, RegionMapPixmapItem *item);
    void mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item);
};

#endif // REGIONMAPEDITOR_H
