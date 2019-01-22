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

    bool hasUnsavedChanges = false;

    double scaleUpFactor = 2.0;
    double scaleDownFactor = 1.0 / scaleUpFactor;

    int scaleRegionMapTiles = 1;
    int scaleRegionMapImage = 1;
    int scaleCityMapTiles = 1;
    int scaleCityMapImage = 1;

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

    void closeEvent(QCloseEvent*);

private slots:
    void on_action_RegionMap_Save_triggered();
    void on_action_RegionMap_Undo_triggered();
    void on_action_RegionMap_Redo_triggered();
    void on_action_RegionMap_Resize_triggered();
    void on_action_RegionMap_Generate_triggered();
    void on_tabWidget_Region_Map_currentChanged(int);
    void on_pushButton_RM_Options_delete_clicked();
    void on_comboBox_RM_ConnectedMap_activated(const QString &);
    void on_pushButton_CityMap_add_clicked();
    void on_verticalSlider_Zoom_Map_Image_valueChanged(int);
    void on_verticalSlider_Zoom_Image_Tiles_valueChanged(int);
    void on_verticalSlider_Zoom_City_Map_valueChanged(int);
    void on_verticalSlider_Zoom_City_Tiles_valueChanged(int);
    void on_comboBox_CityMap_picker_currentTextChanged(const QString &);
    void on_spinBox_RM_Options_x_valueChanged(int);
    void on_spinBox_RM_Options_y_valueChanged(int);
    void on_lineEdit_RM_MapName_textEdited(const QString &);
    void onHoveredRegionMapTileChanged(int, int);
    void onHoveredRegionMapTileCleared();
    void mouseEvent_region_map(QGraphicsSceneMouseEvent *, RegionMapPixmapItem *);
    void mouseEvent_city_map(QGraphicsSceneMouseEvent *, CityMapPixmapItem *);
};

#endif // REGIONMAPEDITOR_H
