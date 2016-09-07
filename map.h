#ifndef MAP_H
#define MAP_H

#include "tileset.h"
#include "blockdata.h"
#include "event.h"

#include <QPixmap>
#include <QObject>
#include <QDebug>


template <typename T>
class History {
public:
    QList<T> history;
    int head;

    History() {
        head = -1;
    }
    T pop() {
        if (head > 0) {
            return history.at(--head);
        }
        return NULL;
    }
    T next() {
        if (head + 1 < history.length()) {
            return history.at(++head);
        }
        return NULL;
    }
    void push(T commit) {
        while (head + 1 < history.length()) {
            history.removeLast();
        }
        history.append(commit);
        head++;
    }
};

class Connection {
public:
    Connection() {
    }
public:
    QString direction;
    QString offset;
    QString map_name;
};

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = 0);

public:
    QString name;
    QString attributes_label;
    QString events_label;
    QString scripts_label;
    QString connections_label;
    QString song;
    QString index;
    QString location;
    QString visibility;
    QString weather;
    QString type;
    QString unknown;
    QString show_location;
    QString battle_scene;

    QString width;
    QString height;
    QString border_label;
    QString blockdata_label;
    QString tileset_primary_label;
    QString tileset_secondary_label;

    Tileset *tileset_primary;
    Tileset *tileset_secondary;

    Blockdata* blockdata;

public:
    int getWidth();
    int getHeight();
    Tileset* getBlockTileset(uint);
    Metatile* getMetatile(uint);
    QImage getMetatileImage(uint);
    QImage getMetatileTile(uint);
    QPixmap render();
    QPixmap renderMetatiles();

    QPixmap renderCollision();
    QImage collision_image;
    QPixmap collision_pixmap;
    QImage getCollisionMetatileImage(Block);
    QImage getElevationMetatileImage(Block);
    QImage getCollisionMetatileImage(int);
    QImage getElevationMetatileImage(int);

    QPixmap renderCollisionMetatiles();
    QPixmap renderElevationMetatiles();
    void drawSelection(int i, int w, QPainter *painter);

    bool blockChanged(int, Blockdata*);
    Blockdata* cached_blockdata;
    void cacheBlockdata();
    Blockdata* cached_collision;
    void cacheCollision();
    QImage image;
    QPixmap pixmap;
    QList<QImage> metatile_images;
    int paint_tile;
    int paint_collision;
    int paint_elevation;

    Block *getBlock(int x, int y);
    void setBlock(int x, int y, Block block);
    void _setBlock(int x, int y, Block block);

    void floodFill(int x, int y, uint tile);
    void _floodFill(int x, int y, uint tile);
    void floodFillCollision(int x, int y, uint collision);
    void _floodFillCollision(int x, int y, uint collision);
    void floodFillElevation(int x, int y, uint elevation);
    void _floodFillElevation(int x, int y, uint elevation);
    void floodFillCollisionElevation(int x, int y, uint collision, uint elevation);
    void _floodFillCollisionElevation(int x, int y, uint collision, uint elevation);

    History<Blockdata*> history;
    void undo();
    void redo();
    void commit();

    QString object_events_label;
    QString warps_label;
    QString coord_events_label;
    QString bg_events_label;

    QList<ObjectEvent*> object_events;
    QList<Warp*> warps;
    QList<CoordEvent*> coord_events;
    QList<Sign*> signs;
    QList<HiddenItem*> hidden_items;

    QList<Connection*> connections;
    QPixmap renderConnection(Connection);

    QImage border_image;
    QPixmap border_pixmap;
    Blockdata *border;
    Blockdata *cached_border;
    QPixmap renderBorder();
    void cacheBorder();

signals:

public slots:
};

#endif // MAP_H
