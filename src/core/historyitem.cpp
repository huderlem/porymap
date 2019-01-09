#include "historyitem.h"

HistoryItem::HistoryItem(Blockdata *metatiles, int layoutWidth, int layoutHeight) {
    this->metatiles = metatiles;
    this->layoutWidth = layoutWidth;
    this->layoutHeight = layoutHeight;
}

HistoryItem::~HistoryItem() {
    if (this->metatiles) delete this->metatiles;
}

RegionMapHistoryItem::RegionMapHistoryItem(int which_, int index_, unsigned prev_, unsigned tile_) {
    this->which = which_;
    this->index = index_;
    this->prev  = prev_;
    this->tile  = tile_;
}

RegionMapHistoryItem::~RegionMapHistoryItem() {}
