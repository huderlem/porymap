#ifndef CURRENTSELECTEDMETATILESPIXMAPITEM_H
#define CURRENTSELECTEDMETATILESPIXMAPITEM_H

#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class Layout;

class CurrentSelectedMetatilesPixmapItem : public QGraphicsPixmapItem {
public:
    CurrentSelectedMetatilesPixmapItem(Layout *layout, MetatileSelector *metatileSelector) {
        this->layout = layout;
        this->metatileSelector = metatileSelector;
    }
    Layout *layout = nullptr;
    MetatileSelector *metatileSelector;
    void draw();

    void setLayout(Layout *layout) { this->layout = layout; }
};

QPixmap drawMetatileSelection(MetatileSelection selection, Layout *layout);

#endif // CURRENTSELECTEDMETATILESPIXMAPITEM_H
