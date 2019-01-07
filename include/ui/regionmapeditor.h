#ifndef REGIONMAPEDITOR_H
#define REGIONMAPEDITOR_H

#include "regionmappixmapitem.h"
#include "citymappixmapitem.h"
#include "regionmaplayoutpixmapitem.h"
#include "regionmap.h"

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

// TODO: make members that are not called outside of this private
    RegionMap *region_map;

    QGraphicsScene *scene_region_map_image = nullptr;
    QGraphicsScene *scene_city_map_image = nullptr;
    QGraphicsScene *scene_region_map_layout = nullptr;
    QGraphicsScene *scene_region_map_tiles = nullptr;
    QGraphicsScene *scene_city_map_tiles = nullptr;
    TilemapTileSelector *mapsquare_selector_item = nullptr;
    TilemapTileSelector *city_map_selector_item = nullptr;
    RegionMapPixmapItem *region_map_item = nullptr;
    CityMapPixmapItem *city_map_item = nullptr;
    RegionMapLayoutPixmapItem *region_map_layout_item = nullptr;

    void loadRegionMapData();
    void displayRegionMap();
    void displayRegionMapImage();
    void displayRegionMapLayout();
    void displayRegionMapLayoutOptions();
    void updateRegionMapLayoutOptions(int);
    void displayRegionMapTileSelector();
    void displayCityMapTileSelector();
    void displayCityMap(QString);
    void loadCityMaps();

    void onRegionMapTileSelectorSelectedTileChanged();
    void onRegionMapTileSelectorHoveredTileChanged(unsigned);
    void onRegionMapTileSelectorHoveredTileCleared();

    void onRegionMapLayoutSelectedTileChanged(int);
    void onRegionMapLayoutHoveredTileChanged(int);
    void onRegionMapLayoutHoveredTileCleared();

private:
    Ui::RegionMapEditor *ui;
    Project *project;

    QString rmStatusbarMessage;

private slots:
    void on_action_RegionMap_Save_triggered();
    void on_tabWidget_Region_Map_currentChanged(int);
    void on_pushButton_RM_Options_save_clicked();
    void on_pushButton_CityMap_save_clicked();
    void on_comboBox_CityMap_picker_currentTextChanged(const QString &);
    void onHoveredRegionMapTileChanged(int, int);
    void onHoveredRegionMapTileCleared();
    void mouseEvent_region_map(QGraphicsSceneMouseEvent *, RegionMapPixmapItem *);
    void mouseEvent_city_map(QGraphicsSceneMouseEvent *, CityMapPixmapItem *);
};

#endif // REGIONMAPEDITOR_H
