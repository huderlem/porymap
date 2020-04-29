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
        this->metatileSelector = metatileSelector;
        this->settings = settings;
        this->paintingMode = PaintMode::Metatiles;
        setAcceptHoverEvents(true);
    }
    MapPixmapItem::PaintMode paintingMode;
    Map *map;
    MetatileSelector *metatileSelector;
    Settings *settings;
    bool active;
    bool right_click;
    int paint_tile_initial_x;
    int paint_tile_initial_y;
    QPoint selection_origin;
    QList<QPoint> selection;
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void magicFill(QGraphicsSceneMouseEvent*);
    void _floodFill(int x, int y);
    void _floodFillSmartPath(int initialX, int initialY);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void select(QGraphicsSceneMouseEvent*);
    virtual void shift(QGraphicsSceneMouseEvent*);
    virtual void draw(bool ignoreCache = false);
    void updateMetatileSelection(QGraphicsSceneMouseEvent *event);

private:
    void paintNormal(int x, int y);
    void paintSmartPath(int x, int y);
    static QList<int> smartPathTable;

signals:
    void startPaint(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void endPaint(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void mouseEvent(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void hoveredMapMetatileChanged(int x, int y);
    void hoveredMapMetatileCleared();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

#endif // MAPPIXMAPITEM_H
