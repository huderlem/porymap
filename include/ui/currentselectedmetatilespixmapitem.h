#ifndef CURRENTSELECTEDMETATILESPIXMAPITEM_H
#define CURRENTSELECTEDMETATILESPIXMAPITEM_H

#include "map.h"
#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class CurrentSelectedMetatilesPixmapItem : public QGraphicsPixmapItem {
public:
    CurrentSelectedMetatilesPixmapItem(Map *map, MetatileSelector *metatileSelector) {
        this->map = map;
        this->metatileSelector = metatileSelector;
    }
    Map* map = nullptr;
    MetatileSelector *metatileSelector;
    void draw();

    void setMap(Map *map) { this->map = map; }
};

#endif // CURRENTSELECTEDMETATILESPIXMAPITEM_H
