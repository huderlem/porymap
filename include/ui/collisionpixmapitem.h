#ifndef COLLISIONPIXMAPITEM_H
#define COLLISIONPIXMAPITEM_H

#include "metatileselector.h"
#include "movementpermissionsselector.h"
#include "mappixmapitem.h"
#include "map.h"
#include "settings.h"

class CollisionPixmapItem : public MapPixmapItem {
    Q_OBJECT
public:
    CollisionPixmapItem(Map *map, MovementPermissionsSelector *movementPermissionsSelector, MetatileSelector *metatileSelector, Settings *settings, qreal *opacity)
        : MapPixmapItem(map, metatileSelector, settings){
        this->movementPermissionsSelector = movementPermissionsSelector;
        this->opacity = opacity;
        map->setCollisionItem(this);
    }
    MovementPermissionsSelector *movementPermissionsSelector;
    qreal *opacity;
    void updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event);
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void magicFill(QGraphicsSceneMouseEvent*);
    virtual void pick(QGraphicsSceneMouseEvent*);
    void draw(bool ignoreCache = false);

private:
    unsigned actionId_ = 0;
    QPoint previousPos;

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, CollisionPixmapItem *);
    void hoveredMapMovementPermissionChanged(int, int);
    void hoveredMapMovementPermissionCleared();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // COLLISIONPIXMAPITEM_H
