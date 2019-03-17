#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "map.h"
#include "project.h"
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
    uint8_t tile_img_id = 0x00;
    uint8_t secid = 0x00;
    bool has_map = false;
    bool has_city_map = false;
    bool duplicated = false;
    QString map_name;
    QString mapsec;
    QString city_map_name;
};

class RegionMap : public QObject
{
    Q_OBJECT

public:
    RegionMap() = default;

    ~RegionMap() {};

    Project *project = nullptr;

    QVector<RegionMapSquare> map_squares;
    History<RegionMapHistoryItem*> history;

    const int padLeft   = 1;
    const int padRight  = 3;
    const int padTop    = 2;
    const int padBottom = 3;

    void init(Project*);

    void readBkgImgBin();
    void readLayout();

    void save();
    void saveBkgImgBin();
    void saveLayout();
    void saveOptions(int id, QString sec, QString name, int x, int y);

    void resize(int width, int height);
    void resetSquare(int index);
    void clearLayout();
    void clearImage();
    void replaceSectionId(unsigned oldId, unsigned newId);

    int  width();
    int  height();
    QSize imgSize();
    unsigned getTileId(int x, int y);
    int getMapSquareIndex(int x, int y);
    QString pngPath();
    QString cityTilesPath();

    QVector<uint8_t> getTiles();
    void setTiles(QVector<uint8_t> tileIds);

private:
    int layout_width_;
    int layout_height_;
    int img_width_;
    int img_height_;

    QString region_map_png_path;
    QString region_map_bin_path;
    QString region_map_entries_path;
    QString region_map_layout_bin_path;
    QString city_map_tiles_path;

    QMap<QString, QString> sMapNamesMap;
    QMap<QString, struct RegionMapEntry> mapSecToMapEntry;
    QVector<QString> sMapNames;

    int img_index_(int x, int y);
    int layout_index_(int x, int y);

    QString fix_case(QString);
};

#endif // REGIONMAP_H
