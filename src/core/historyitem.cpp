#include "historyitem.h"

HistoryItem::HistoryItem(Blockdata *metatiles, int layoutWidth, int layoutHeight) {
    this->metatiles = metatiles;
    this->layoutWidth = layoutWidth;
    this->layoutHeight = layoutHeight;
}

HistoryItem::~HistoryItem() {
    if (this->metatiles) delete this->metatiles;
}
