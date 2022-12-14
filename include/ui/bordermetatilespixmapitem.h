#ifndef BORDERMETATILESPIXMAPITEM_H
#define BORDERMETATILESPIXMAPITEM_H

#include "map.h"
#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class BorderMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    BorderMetatilesPixmapItem(Map *map_, MetatileSelector *metatileSelector) {
        this->map = map_;
        this->map->setBorderItem(this);
        this->metatileSelector = metatileSelector;
        setAcceptHoverEvents(true);
    }
    MetatileSelector *metatileSelector;
    Map *map;
    void draw();
signals:
    void hoveredBorderMetatileSelectionChanged(uint16_t);
    void hoveredBorderMetatileSelectionCleared();
    void borderMetatilesChanged();

private:
    void hoverUpdate(const QPointF &);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
};

#endif // BORDERMETATILESPIXMAPITEM_H
