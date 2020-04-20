#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include "blockdata.h"

class HistoryItem {
public:
    Blockdata *metatiles;
    Blockdata *border;
    int layoutWidth;
    int layoutHeight;
    int borderWidth;
    int borderHeight;
    HistoryItem(Blockdata *metatiles, Blockdata *border, int layoutWidth, int layoutHeight, int borderWidth, int borderHeight);
    ~HistoryItem();
};

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
    RegionMapHistoryItem(int type, QVector<uint8_t> tiles, QString cityMap);
    RegionMapHistoryItem(int type, QVector<uint8_t> tiles, int width, int height);
    ~RegionMapHistoryItem();
};

#endif // HISTORYITEM_H
