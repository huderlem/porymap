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

#endif // HISTORYITEM_H
