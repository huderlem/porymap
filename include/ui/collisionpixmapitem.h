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
    unsigned actionId_ = 0;
    QPoint previousPos;
    void updateSelection(QPoint pos);

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, CollisionPixmapItem *);
    void hoverEntered(const QPoint &pos);
    void hoverChanged(const QPoint &pos);
    void hoverCleared();

protected:
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
};

#endif // COLLISIONPIXMAPITEM_H
