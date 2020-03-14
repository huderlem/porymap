#include "historyitem.h"

HistoryItem::HistoryItem(Blockdata *metatiles, Blockdata *border, int layoutWidth, int layoutHeight, int borderWidth, int borderHeight) {
    this->metatiles = metatiles;
    this->border = border;
    this->layoutWidth = layoutWidth;
    this->layoutHeight = layoutHeight;
    this->borderWidth = borderWidth;
    this->borderHeight = borderHeight;
}

HistoryItem::~HistoryItem() {
    if (this->metatiles) delete this->metatiles;
    if (this->border) delete this->border;
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
