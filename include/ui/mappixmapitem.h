#ifndef MAPPIXMAPITEM_H
#define MAPPIXMAPITEM_H

#include "map.h"
#include "settings.h"
#include "metatileselector.h"
#include <QGraphicsPixmapItem>

class MapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

private:
    using QGraphicsPixmapItem::paint;

public:
    enum class PaintMode {
        Disabled,
        Metatiles,
        EventObjects
    };
    MapPixmapItem(Map *map_, MetatileSelector *metatileSelector, Settings *settings) {
        this->map = map_;
        this->map->setMapItem(this);
        this->metatileSelector = metatileSelector;
        this->settings = settings;
        this->paintingMode = PaintMode::Metatiles;
        this->lockedAxis = MapPixmapItem::Axis::None;
        this->prevStraightPathState = false;
        setAcceptHoverEvents(true);
    }
    MapPixmapItem::PaintMode paintingMode;
    Map *map;
    MetatileSelector *metatileSelector;
    Settings *settings;
    bool active;
    bool has_mouse = false;
    bool right_click;
    int paint_tile_initial_x;
    int paint_tile_initial_y;
    bool prevStraightPathState;
    int straight_path_initial_x;
    int straight_path_initial_y;
    QPoint metatilePos;
    enum Axis {
        None = 0,
        X,
        Y
    };
    MapPixmapItem::Axis lockedAxis;
    QPoint selection_origin;
    QList<QPoint> selection;
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void magicFill(QGraphicsSceneMouseEvent*);
    void magicFill(int x, int y, uint16_t metatileId, bool fromScriptCall = false);
    void magicFill(int x, int y, bool fromScriptCall = false);
    void magicFill(
            int initialX,
            int initialY,
            QPoint selectionDimensions,
            QList<uint16_t> *selectedMetatiles,
            QList<QPair<uint16_t, uint16_t>> *selectedCollisions,
            bool fromScriptCall = false);
    void floodFill(int x, int y, bool fromScriptCall = false);
    void floodFill(int x, int y, uint16_t metatileId, bool fromScriptCall = false);
    void floodFill(int initialX,
                   int initialY,
                   QPoint selectionDimensions,
                   QList<uint16_t> *selectedMetatiles,
                   QList<QPair<uint16_t, uint16_t>> *selectedCollisions,
                   bool fromScriptCall = false);
    void floodFillSmartPath(int initialX, int initialY, bool fromScriptCall = false);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void select(QGraphicsSceneMouseEvent*);
    virtual void shift(QGraphicsSceneMouseEvent*);
    void shift(int xDelta, int yDelta, bool fromScriptCall = false);
    virtual void draw(bool ignoreCache = false);
    void updateMetatileSelection(QGraphicsSceneMouseEvent *event);
    void paintNormal(int x, int y, bool fromScriptCall = false);
    void lockNondominantAxis(QGraphicsSceneMouseEvent *event);
    QPoint adjustCoords(QPoint pos);

private:
    void paintSmartPath(int x, int y, bool fromScriptCall = false);
    static QList<int> smartPathTable;

    unsigned actionId_ = 0;

signals:
    void startPaint(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void endPaint(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void mouseEvent(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void hoveredMapMetatileChanged(const QPoint &pos);
    void hoveredMapMetatileCleared();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverEnterEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // MAPPIXMAPITEM_H
