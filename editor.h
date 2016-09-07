#ifndef EDITOR_H
#define EDITOR_H

#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>

#include "project.h"


class DraggablePixmapItem : public QGraphicsPixmapItem {
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Event *event;
    DraggablePixmapItem(Event *event_) : QGraphicsPixmapItem(event_->pixmap) {
        event = event_;
        update();
    }
    bool active;
    bool right_click;
    int last_x;
    int last_y;
    void update() {
        int x = event->x() * 16;
        int y = event->y() * 16;
        x -= pixmap().width() / 32 * 16;
        y -= pixmap().height() - 16;
        setX(x);
        setY(y);
        setZValue(event->y());
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class MapPixmapItem : public QGraphicsPixmapItem {
public:
    MapPixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Map *map;
    MapPixmapItem(Map *map_) {
        map = map_;
    }
    bool active;
    bool right_click;
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void undo();
    virtual void redo();
    virtual void draw();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class CollisionPixmapItem : public MapPixmapItem {
public:
    CollisionPixmapItem(QPixmap pixmap): MapPixmapItem(pixmap) {
    }
    CollisionPixmapItem(Map *map_): MapPixmapItem(map_) {
    }

public:
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void draw();
};

class MetatilesPixmapItem : public QGraphicsPixmapItem {
public:
    MetatilesPixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    MetatilesPixmapItem(Map *map_) {
        map = map_;
    }
    Map* map;
    virtual void pick(uint);
    virtual void draw();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class CollisionMetatilesPixmapItem : public MetatilesPixmapItem {
public:
    CollisionMetatilesPixmapItem(Map *map_): MetatilesPixmapItem(map_) {
    }
    virtual void pick(uint collision) {
        map->paint_collision = collision;
        draw();
    }
    virtual void draw() {
        setPixmap(map->renderCollisionMetatiles());
    }
};

class ElevationMetatilesPixmapItem : public MetatilesPixmapItem {
public:
    ElevationMetatilesPixmapItem(Map *map_): MetatilesPixmapItem(map_) {
    }
    virtual void pick(uint elevation) {
        map->paint_elevation = elevation;
        draw();
    }
    virtual void draw() {
        setPixmap(map->renderElevationMetatiles());
    }
};


class Editor
{
public:
    Editor();
public:
    Project *project;
    Map *map;
    void saveProject();
    void save();
    void undo();
    void redo();
    void setMap(QString map_name);
    void displayMap();
    void displayMetatiles();
    void displayCollisionMetatiles();
    void displayElevationMetatiles();
    void displayMapObjects();
    void displayMapConnections();
    void displayMapBorder();

    void setEditingMap();
    void setEditingCollision();
    void setEditingObjects();

    QGraphicsScene *scene;
    QGraphicsPixmapItem *current_view;
    MapPixmapItem *map_item;
    CollisionPixmapItem *collision_item;
    QGraphicsItemGroup *objects_group;

    QGraphicsScene *scene_metatiles;
    QGraphicsScene *scene_collision_metatiles;
    QGraphicsScene *scene_elevation_metatiles;
    MetatilesPixmapItem *metatiles_item;
    CollisionMetatilesPixmapItem *collision_metatiles_item;
    ElevationMetatilesPixmapItem *elevation_metatiles_item;
};

#endif // EDITOR_H
