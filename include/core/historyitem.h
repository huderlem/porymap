#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include "blockdata.h"

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
};

class RegionMapHistoryItem {
public:
    int which;// region map or city map
    int index;
    unsigned tile;
    unsigned prev;
    RegionMapHistoryItem(int type, int index, unsigned prev, unsigned tile);
    ~RegionMapHistoryItem();
};

#endif // HISTORYITEM_H
