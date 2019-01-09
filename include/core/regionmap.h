#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "project.h"
#include "map.h"
#include "tilemaptileselector.h"
#include "history.h"
#include "historyitem.h"

#include <QStringList>
#include <QString>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsView>



// rename this struct?
struct CityMapPosition
{
    //
    //QString filename; // eg. dewford_0 ?
    QString tilemap;// eg. "dewford_0"
    int x;
    int y;
};

struct RegionMapEntry
{
    //
    int x;
    int y;
    int width;
    int height;
    QString name;// mapsection
};

// class that holds data for each square in this project
// struct?
// TODO: change char / uint8_t to unsigned
class RegionMapSquare
{
public:
    //
    int x = -1;// x position, 0-indexed from top left
    int y = -1;// y position, 0-indexed from top left
    uint8_t tile_img_id;// tilemap ids for the background image
    bool has_map = false;// whether this square is linked to a map or is empty
    QString map_name;// name of the map associated with this square (if has_map is true): eg. "MAUVILLE_CITY" (TODO: REMOVE)
    QString mapsec;
    uint8_t secid;
    bool has_city_map = false;// whether there is a city map on this grid
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

    ~RegionMap() {};

    static QMap<QString, QList<struct CityMapPosition>> ruby_city_maps_;
    static QString mapSecToMapConstant(QString);

    Project *project;

    //RegionMapSquare *map_squares = nullptr;// array of RegionMapSquares
    QList<RegionMapSquare> map_squares;

    History<RegionMapHistoryItem*> history;

    QString temp_path;// delete this
    QString city_map_squares_path;
    QString region_map_png_path;
    QString region_map_bin_path;// = QString::null;
    QString city_map_header_path;//dafuq is this?
    QString region_map_layout_path;
    QString region_map_entries_path;
    QString region_map_layout_bin_path;
    QString region_map_city_map_tiles_path;

    QByteArray mapBinData;

    QMap<QString, QString> sMapNames;// {"{/sMapName_/}LittlerootTown" : "LITTLEROOT{NAME_END} TOWN"}
    QMap<QString, QString> mapSecToMapName;// {"MAPSEC_LITTLEROOT_TOWN" : "LITTLEROOT{NAME_END} TOWN"}
    //QList<QPair<QString, struct RegionMapEntry>> mapSecToMapEntry;
    QMap<QString, struct RegionMapEntry> mapSecToMapEntry;// TODO: add to this on creation of new map

    bool hasUnsavedChanges();

    void init(Project*);//QString);

    // parseutil.cpp ?
    void readBkgImgBin();
    void readCityMaps();// more complicated
    void readLayout();

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
    void saveOptions(int, QString, QString, int, int);
    void saveCityMaps();

    void update();// update the view in case something is broken?
    void resize(int, int);
    void setWidth(int);
    void setHeight(int);
    int  width();
    int  height();
    QSize imgSize();
    unsigned getTileId(int, int);
    int getMapSquareIndex(int, int);

    void deleteLayoutSquare(int);

    // implement these here?
    void undo();
    void redo();

    void test();// remove when done testing obvi

// TODO: move read / write functions to private (and others)
private:
    //
    //History<QPair<int, uint8_t>> *history;// (index, tile)
    int layout_width_;
    int layout_height_;
    int img_width_;
    int img_height_;
    int img_index_(int, int);// returns index int at x,y args (x + y * width_ * 2) // 2 because 
    int layout_index_(int, int);
    void fillMapSquaresFromLayout();
    QString fix_case(QString);// CAPS_WITH_UNDERSCORE to CamelCase

//protected:
    //

//signals:
    //
};

//TilemapTileSelector *city_map_metatile_selector_item = nullptr;

#endif // REGIONMAP_H
