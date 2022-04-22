#pragma once
#ifndef REGIONMAP_H
#define REGIONMAP_H

#include "map.h"
#include "tilemaptileselector.h"
#include "history.h"

#include <QStringList>
#include <QString>
#include <QVector>
#include <QList>
#include <QMap>
#include <QGraphicsScene>
#include <QGraphicsView>

#include <memory>
using std::shared_ptr;

class Project;

enum RegionMapEditorBox {
    BackgroundImage = 1,
    CityMapImage    = 2,
};

class RegionMapHistoryItem {
public:
    int which;
    int mapWidth = 0;
    int mapHeight = 0;
    QVector<uint8_t> tiles;
    QString cityMap;
    RegionMapHistoryItem(int which, QVector<uint8_t> tiles, QString cityMap) {
        this->which = which;
        this->tiles = tiles;
        this->cityMap = cityMap;
    }
    RegionMapHistoryItem(int which, QVector<uint8_t> tiles, int width, int height) {
        this->which = which;
        this->tiles = tiles;
        this->mapWidth = width;
        this->mapHeight = height;
    }
    ~RegionMapHistoryItem() {}
};

struct LayoutSquare
{
    QString map_section;
    int x;
    int y;
    bool has_map = false;
};

struct MapSectionEntry
{
    QString name = "";
    int x = 0;
    int y = 0;
    int width = 1;
    int height = 1;
    bool valid = false;
};

class RegionMap
{
public:
    RegionMap() = delete;
    RegionMap(Project *);

    Project *project = nullptr;

    History<RegionMapHistoryItem*> history; // TODO

    bool loadMapData(poryjson::Json);
    bool loadTilemap(poryjson::Json);
    bool loadLayout(poryjson::Json);
    bool loadEntries();

    void setEntries(tsl::ordered_map<QString, MapSectionEntry> *entries) { this->region_map_entries = entries; }
    MapSectionEntry getEntry(QString section);

    void save();
    void saveTilemap();
    void saveLayout();
    void saveConfig();// ? or do this in the editor only?
    void saveOptions(int id, QString sec, QString name, int x, int y);

    void resize(int width, int height);
    void resetSquare(int index);
    void clearLayout();
    void clearImage();
    void replaceSectionId(unsigned oldId, unsigned newId);

    unsigned getTileId(int index);
    shared_ptr<TilemapTile> getTile(int index);
    unsigned getTileId(int x, int y);
    shared_ptr<TilemapTile> getTile(int x, int y);
    bool squareHasMap(int index);
    QString squareMapSection(int index);
    void setSquareMapSection(int index, QString section);
    int squareX(int index);
    int squareY(int index);
    bool squareInLayout(int x, int y);
    int firstLayoutIndex() { return this->offset_left + this->offset_top * this->tilemap_width; }

    void setTileId(int index, unsigned id);
    void setTile(int index, TilemapTile &tile);
    void setTileData(int index, unsigned id, bool hFlip, bool vFlip, int palette);
    int getMapSquareIndex(int x, int y);
    
    QString palPath();
    QString pngPath();
    QString cityTilesPath();
    QString entriesPath() { return this->entries_path; }

    QVector<uint8_t> getTiles();
    void setTiles(QVector<uint8_t> tileIds);

    QByteArray getTilemap();
    void setTilemap(QByteArray newTilemap);

    QStringList getLayers() { return this->layout_layers; }
    void setLayer(QString layer) { this->current_layer = layer; }
    QString getLayer() { return this->current_layer; }

    QString fixCase(QString);

    int padLeft() { return this->offset_left; }
    int padTop() { return this->offset_top; }
    int padRight() { return this->tilemap_width - this->layout_width - this->offset_left; }
    int padBottom() { return this->tilemap_height - this->layout_height - this->offset_top; }

    int tilemapWidth() { return this->tilemap_width; }
    int tilemapHeight() { return this->tilemap_height; }
    int tilemapSize() { return this->tilemap_width * this->tilemap_height; }
    int tilemapBytes();

    int tilemapToLayoutIndex(int index);

    TilemapFormat tilemapFormat() { return this->tilemap_format; }

    int pixelWidth() { return this->tilemap_width * 8; }
    int pixelHeight() { return this->tilemap_height * 8; }

    QString fullPath(QString local);

private:
    // TODO: defaults needed?
    tsl::ordered_map<QString, MapSectionEntry> *region_map_entries = nullptr;

    QString alias;

    int tilemap_width;
    int tilemap_height;

    int region_width;
    int region_height;

    int layout_width;
    int layout_height;

    int offset_left;
    int offset_top;

    TilemapFormat tilemap_format;

    enum class LayoutFormat { None, Binary, CArray };
    LayoutFormat layout_format;

    QString tileset_path;
    QString tilemap_path;
    QString palette_path = "";

    QString entries_path;
    QString layout_path;

    QString layout_array_label;
    bool layout_uses_layers = false;
    QStringList layout_constants;
    QString layout_qualifiers;

    QList<shared_ptr<TilemapTile>> tilemap;

    QStringList layout_layers; // TODO: is this used?
    QString current_layer;

    // TODO: just use ordered map?
    QMap<QString, QList<LayoutSquare>> layouts; // key: layer, value: layout

    // TODO
    QString city_map_tiles_path;

    int get_tilemap_index(int x, int y);
    int get_layout_index(int x, int y);
};

#endif // REGIONMAP_H
