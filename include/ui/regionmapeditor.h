#ifndef REGIONMAPEDITOR_H
#define REGIONMAPEDITOR_H

#include "regionmappixmapitem.h"
#include "citymappixmapitem.h"
#include "regionmaplayoutpixmapitem.h"
#include "regionmap.h"
#include "history.h"
#include "historyitem.h"

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QCloseEvent>

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

    void loadRegionMapData();
    void loadCityMaps();

    void onRegionMapTileSelectorSelectedTileChanged(unsigned);
    void onCityMapTileSelectorSelectedTileChanged(unsigned);
    void onRegionMapTileSelectorHoveredTileChanged(unsigned);
    void onRegionMapTileSelectorHoveredTileCleared();

    void onRegionMapLayoutSelectedTileChanged(int);
    void onRegionMapLayoutHoveredTileChanged(int);
    void onRegionMapLayoutHoveredTileCleared();

    void undo();
    void redo();

    void resize(int, int);

private:
    Ui::RegionMapEditor *ui;
    Project *project;

    History<RegionMapHistoryItem*> history;

    int currIndex;
    unsigned selectedCityTile;
    unsigned selectedImageTile;

    double scaleUpFactor = 2.0;
    double scaleDownFactor = 1.0 / scaleUpFactor;

    double scaleRegionMapTiles = 1.0;
    double scaleRegionMapImage = 1.0;
    double scaleCityMapTiles = 1.0;
    double scaleCityMapImage = 1.0;

    QGraphicsScene *scene_region_map_image  = nullptr;
    QGraphicsScene *scene_city_map_image    = nullptr;
    QGraphicsScene *scene_region_map_layout = nullptr;
    QGraphicsScene *scene_region_map_tiles  = nullptr;
    QGraphicsScene *scene_city_map_tiles    = nullptr;

    TilemapTileSelector *mapsquare_selector_item = nullptr;
    TilemapTileSelector *city_map_selector_item  = nullptr;

    RegionMapLayoutPixmapItem *region_map_layout_item = nullptr;
    RegionMapPixmapItem *region_map_item = nullptr;
    CityMapPixmapItem *city_map_item = nullptr;

    void displayRegionMap();
    void displayRegionMapImage();
    void displayRegionMapLayout();
    void displayRegionMapLayoutOptions();
    void updateRegionMapLayoutOptions(int);
    void displayRegionMapTileSelector();
    void displayCityMapTileSelector();
    void displayCityMap(QString);

    bool createCityMap(QString);

private slots:
    void on_action_RegionMap_Save_triggered();
    void on_action_RegionMap_Undo_triggered();
    void on_action_RegionMap_Redo_triggered();
    void on_action_RegionMap_Resize_triggered();
    void on_action_RegionMap_Generate_triggered();
    void on_tabWidget_Region_Map_currentChanged(int);
    void on_pushButton_RM_Options_save_clicked();
    void on_pushButton_RM_Options_delete_clicked();
    void on_pushButton_CityMap_save_clicked();
    void on_pushButton_CityMap_add_clicked();
    void on_pushButton_Zoom_In_Image_Tiles_clicked();
    void on_pushButton_Zoom_Out_Image_Tiles_clicked();
    void on_pushButton_Zoom_In_City_Tiles_clicked();
    void on_pushButton_Zoom_Out_City_Tiles_clicked();
    void on_pushButton_Zoom_In_City_Map_clicked();
    void on_pushButton_Zoom_Out_City_Map_clicked();
    void on_pushButton_Zoom_In_Map_Image_clicked();
    void on_pushButton_Zoom_Out_Map_Image_clicked();
    void on_comboBox_CityMap_picker_currentTextChanged(const QString &);
    void on_spinBox_RM_Options_x_valueChanged(int);
    void on_spinBox_RM_Options_y_valueChanged(int);
    void onHoveredRegionMapTileChanged(int, int);
    void onHoveredRegionMapTileCleared();
    void mouseEvent_region_map(QGraphicsSceneMouseEvent *, RegionMapPixmapItem *);
    void mouseEvent_city_map(QGraphicsSceneMouseEvent *, CityMapPixmapItem *);
};

#endif // REGIONMAPEDITOR_H
