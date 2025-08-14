#ifndef COLLISIONPIXMAPITEM_H
#define COLLISIONPIXMAPITEM_H

#include <QSpinBox>

#include "metatileselector.h"
#include "movementpermissionsselector.h"
#include "layoutpixmapitem.h"
#include "map.h"
#include "settings.h"

class CollisionPixmapItem : public LayoutPixmapItem {
    Q_OBJECT
public:
    CollisionPixmapItem(Layout *layout, QSpinBox * selectedCollision, QSpinBox * selectedElevation, MetatileSelector *metatileSelector, Settings *settings, qreal *opacity)
        : LayoutPixmapItem(layout, metatileSelector, settings){
        this->selectedCollision = selectedCollision;
        this->selectedElevation = selectedElevation;
        this->opacity = opacity;
        layout->setCollisionItem(this);
    }
    QSpinBox * selectedCollision;
    QSpinBox * selectedElevation;
    qreal *opacity;
    void updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event);
    virtual void paint(QGraphicsSceneMouseEvent*) override;
    virtual void floodFill(QGraphicsSceneMouseEvent*) override;
    virtual void magicFill(QGraphicsSceneMouseEvent*) override;
    virtual void pick(QGraphicsSceneMouseEvent*) override;
    void draw(bool ignoreCache = false) override;

private:
    void updateSelection(QPoint pos);
};

#endif // COLLISIONPIXMAPITEM_H
