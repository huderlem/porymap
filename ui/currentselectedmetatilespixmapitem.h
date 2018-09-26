#ifndef CURRENTSELECTEDMETATILESPIXMAPITEM_H
#define CURRENTSELECTEDMETATILESPIXMAPITEM_H

#include "map.h"
#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class CurrentSelectedMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    CurrentSelectedMetatilesPixmapItem(Map *map, MetatileSelector *metatileSelector) {
        this->map = map;
        this->metatileSelector = metatileSelector;
    }
    Map* map = nullptr;
    MetatileSelector *metatileSelector;
    void draw();
};

#endif // CURRENTSELECTEDMETATILESPIXMAPITEM_H
