#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "project.h"
#include "map.h"
#include "tilemaptileselector.h"
#include "history.h"
#include "historyitem.h"

#include <QStringList>
#include <QString>
#include <QVector>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsView>



struct CityMapPosition
{
    QString tilemap;
    int x;
    int y;
};

struct RegionMapEntry
{
    int x;
    int y;
    int width;
    int height;
    QString name;
};

class RegionMapSquare
{
public:
    int x = -1;
    int y = -1;
    uint8_t tile_img_id;
    bool has_map = false;
    QString map_name;
    QString mapsec;
    uint8_t secid;
    bool has_city_map = false;
    QString city_map_name;
    bool duplicated = false;
};

class RegionMap : public QObject
{
    Q_OBJECT

public:
    RegionMap() = default;

    ~RegionMap() {};

    static QString mapSecToMapConstant(QString);

    Project *project;

    //QList<RegionMapSquare> map_squares;
    QVector<RegionMapSquare> map_squares;

    History<RegionMapHistoryItem*> history;

    QString region_map_png_path;
    QString region_map_bin_path;
    QString region_map_entries_path;
    QString region_map_layout_bin_path;
    QString city_map_tiles_path;

    QByteArray mapBinData;

    QMap<QString, QString> sMapNamesMap;// {"{/sMapName_/}LittlerootTown" : "LITTLEROOT{NAME_END} TOWN"}
    QMap<QString, QString> mapSecToMapName;// {"MAPSEC_LITTLEROOT_TOWN" : "LITTLEROOT{NAME_END} TOWN"}
    QMap<QString, struct RegionMapEntry> mapSecToMapEntry;// TODO: add to this on creation of new map

    QVector<QString> sMapNames;

    bool hasUnsavedChanges();

    void init(Project*);

    void readBkgImgBin();
    void readCityMaps();
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

    void resize(int, int);
    void setWidth(int);
    void setHeight(int);
    int  width();
    int  height();
    QSize imgSize();
    unsigned getTileId(int, int);
    int getMapSquareIndex(int, int);

    void resetSquare(int);

    void test();// remove

// TODO: move read / write functions to private (and others)
private:
    int layout_width_;
    int layout_height_;
    int img_width_;
    int img_height_;
    int img_index_(int, int);// returns index int at x,y args (x + y * width_ * 2) // 2 because 
    int layout_index_(int, int);
    void fillMapSquaresFromLayout();
    QString fix_case(QString);// CAPS_WITH_UNDERSCORE to CamelCase

};

#endif // REGIONMAP_H
