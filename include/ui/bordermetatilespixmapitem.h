#ifndef BORDERMETATILESPIXMAPITEM_H
#define BORDERMETATILESPIXMAPITEM_H

#include "maplayout.h"
#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class BorderMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    BorderMetatilesPixmapItem(Layout *layout, MetatileSelector *metatileSelector) {
        this->layout = layout;
        this->layout->setBorderItem(this);
        this->metatileSelector = metatileSelector;
        setAcceptHoverEvents(true);
    }
    MetatileSelector *metatileSelector;
    Layout *layout;
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
