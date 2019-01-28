#include "historyitem.h"

HistoryItem::HistoryItem(Blockdata *metatiles, int layoutWidth, int layoutHeight) {
    this->metatiles = metatiles;
    this->layoutWidth = layoutWidth;
    this->layoutHeight = layoutHeight;
}

HistoryItem::~HistoryItem() {
    if (this->metatiles) delete this->metatiles;
}

RegionMapHistoryItem::RegionMapHistoryItem(int which, QVector<uint8_t> tiles, QString cityMap) {
    this->which = which;
    this->tiles = tiles;
    this->cityMap = cityMap;
}

RegionMapHistoryItem::RegionMapHistoryItem(int which, QVector<uint8_t> tiles, int width, int height) {
    this->which = which;
    this->tiles = tiles;
    this->mapWidth = width;
    this->mapHeight = height;
}

RegionMapHistoryItem::~RegionMapHistoryItem() {}
