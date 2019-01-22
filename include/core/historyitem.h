#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include "blockdata.h"
//#include "regionmap.h"

class HistoryItem {
public:
    Blockdata *metatiles;
    int layoutWidth;
    int layoutHeight;
    HistoryItem(Blockdata *metatiles, int layoutWidth, int layoutHeight);
    ~HistoryItem();
};

enum RegionMapEditorBox {
    BackgroundImage = 1,
    CityMapImage    = 2,
    BackroundResize = 3,
};

class RegionMapHistoryItem {
public:
    int which;
    int mapWidth;
    int mapHeight;
    QVector<uint8_t> tiles;
    QString cityMap;
    RegionMapHistoryItem(int type, QVector<uint8_t> tiles);
    RegionMapHistoryItem(int type, QVector<uint8_t> tiles, QString cityMap);
    RegionMapHistoryItem(int type, QVector<uint8_t> tiles, int width, int height);
    ~RegionMapHistoryItem();
};

#endif // HISTORYITEM_H
