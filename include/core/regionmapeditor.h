#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "project.h"
#include "map.h"
#include "tilemaptileselector.h"
//#include "block.h"

#include <QStringList>
#include <QString>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsView>

// if editing map bins, will need to remake the graphics when editing
// are the scenes set in the editor / project / mainwindow files?

/*
 *   - display the region map background image
 *   - edit the region_map_layout.h layout
 *   - edit city maps metatile layout and JUST save the mapname_num.bin
 *   - edit 
 *   who edits pokenav_city_maps 1 and 2?
 *   users can: - add the incbins probably themselves
 *              - add
 *              - edit region map background image
 * 
 * 
 * 
 * 
 *    Editor:
 *        - void displayCityMapMetatileSelector
 *        - void displayRegionMapTileSelector
 *        - void selectRegionMapTile(QString mapname)
 *        - QGraphicsScene *scene_city_map_metatiles
 *        - TilemapTileSelector *city_map_metatile_selector_item
 *        - Tileset *city_map_squares (or tileset_city_map?)
 *        - Tileset *tileset_region_map
 * 
 *    MainWindow:
 * 
 * 
 *    Project:
 *    
 */

// rename this struct
struct CityMapPosition
{
    //
    //QString filename; // eg. dewford_0
    QString tilemap;// eg. "dewford_0"
    int x;
    int y;
};

// class that holds data for each square in this project
// struct?
// TODO: change char / uint8_t to unsigned
class RegionMapSquare
{
public:
    //
    // are positions layout positions? (yes) so out of bounds are all (-1, -1) <-- how it's used in code
    // (GetRegionMapLocationPosition)
    // or image positions
    int x = -1;// x position, 0-indexed from top left
    int y = -1;// y position, 0-indexed from top left
    uint8_t tile_img_id;// tilemap ids for the background image
    bool has_map = false;// whether this square is linked to a map or is empty
    QString map_name;// name of the map associated with this square (if has_map is true): eg. "MAUVILLE_CITY"
    // ^ use project mapsec to names table
    bool has_city_map;// whether there is a city map on this grid
    //QList<struct CityMapPosition> city_maps;
    QString city_map_name;// filename of the city_map tilemap
    //bool is_flyable;//? needed ?
    friend class RegionMap;// not necessary if instance? what
};

class RegionMap : public QObject
{
    Q_OBJECT
//public:
//    explicit Map(QObject *parent = nullptr);

public:
    RegionMap() = default;

    ~RegionMap() {
        delete mapname_abbr;
        delete layout_map_names;
        //delete background_image_tiles;
        //delete map_squares;
        //delete background_image_selector_item;
    };

    static QMap<QString, QList<struct CityMapPosition>> ruby_city_maps_;
    static QString mapSecToMapConstant(QString);

    //RegionMapSquare *map_squares = nullptr;// array of RegionMapSquares
    QList<RegionMapSquare> map_squares;

    QString temp_path;// delete this
    QString city_map_squares_path;
    QString region_map_png_path;
    QString region_map_bin_path;// = QString::null;
    QString city_map_header_path;//dafuq is this?
    QString region_map_layout_path;

    //QMap<QString, somthing> something;// name of map : info about city map, position in layoit, etc.
    //QMap<QString, TilemapTile*> regionMapLayoutTng; // mapName : tilemaptileselector
    // maybe position data to select correct square when changing map on side but only if map is a valid
    //QList<uint8_t>         *background_image_tiles;// the visible ones anyways // using list because replace
    //TilemapTileSelector    *background_image_selector_item;// ?
    QMap<QString, QString> *mapname_abbr;// layout shortcuts mapname:region_map_layout defines (both ways)
    // make this a QHash?? <-- no because something
    QStringList            *layout_map_names;
    // uint8_t border_tile;

    bool hasUnsavedChanges();

    void init(Project*);//QString);

    // parseutil.cpp ?
    void readBkgImgBin();
    void readCityMaps();// more complicated
    void readLayout(QMap<QString, QString>*);

    QString newAbbr(QString);// makes a *unique* 5 character abbreviation from mapname to add to mapname_abbr

    // editing functions
    // if they are booleans, returns true if successful?
    bool placeTile(char, int, int);// place tile at x, y
    bool removeTile(char, int, int);// replaces with 0x00 byte at x,y
    bool placeMap(QString, int, int);
    bool removeMap(QString, int, int);
    bool removeMap(QString);// remove all instances of map

    void save();
    void saveBkgImgBin();
    void saveLayout();
    void saveCityMaps();

    void update();// update the view in case something is broken?
    void resize(int, int);
    void setWidth(int);
    void setHeight(int);
    int  width();
    int  height();
    QSize imgSize();
    unsigned getTileId(int, int);

    // implement these here?
    void undo();
    void redo();

    void test(QMap<QString, QString>*);// remove when done testing obvi

// TODO: move read / write functions to private (and others)
private:
    //
    int layout_width_;
    int layout_height_;
    int img_width_;
    int img_height_;
    int img_index_(int, int);// returns index int at x,y args (x + y * width_ * 2) // 2 because 
    int layout_index_(int, int);

//protected:
    //

//signals:
    //
};

//TilemapTileSelector *city_map_metatile_selector_item = nullptr;

#endif // REGIONMAP_H
