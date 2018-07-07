#ifndef MAP_H
#define MAP_H

#include "tileset.h"
#include "blockdata.h"
#include "event.h"

#include <QPixmap>
#include <QObject>
#include <QDebug>
#include <QGraphicsPixmapItem>


template <typename T>
class History {
public:
    History() {

    }
    T back() {
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
        if (saved > head) {
            saved = -1;
        }
        history.append(commit);
        head++;
    }
    T current() {
        if (head < 0 || history.length() == 0) {
            return NULL;
        }
        return history.at(head);
    }
    void save() {
        saved = head;
    }
    bool isSaved() {
        return saved == head;
    }

private:
    QList<T> history;
    int head = -1;
    int saved = -1;
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

class MapLayout {
public:
    MapLayout() {}
    int index;
    QString name;
    QString label;
    QString width;
    QString height;
    QString border_label;
    QString border_path;
    QString blockdata_label;
    QString blockdata_path;
    QString tileset_primary_label;
    QString tileset_secondary_label;
    Tileset *tileset_primary = NULL;
    Tileset *tileset_secondary = NULL;
    Blockdata* blockdata = NULL;
    QImage border_image;
    QPixmap border_pixmap;
    Blockdata *border = NULL;
    Blockdata *cached_blockdata = NULL;
    Blockdata *cached_collision = NULL;
    Blockdata *cached_border = NULL;
    bool has_unsaved_changes = false;
public:
    static QString getNameFromLabel(QString label) {
        // ASSUMPTION: strip off "_Layout" from layout label. Directories in 'data/layouts/' must be well-formed.
        return label.replace(label.lastIndexOf("_Layout"), label.length(), "");
    }
};

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = 0);

public:
    QString name;
    QString constantName;
    QString group_num;
    QString layout_label;
    QString events_label;
    QString scripts_label;
    QString connections_label;
    QString song;
    QString layout_id;
    QString location;
    QString visibility;
    QString weather;
    QString type;
    QString unknown;
    QString show_location;
    QString battle_scene;
    MapLayout *layout;

    bool isPersistedToFile = true;

public:
    void setName(QString mapName);
    static QString mapConstantFromName(QString mapName);
    static QString objectEventsLabelFromName(QString mapName);
    static QString warpEventsLabelFromName(QString mapName);
    static QString coordEventsLabelFromName(QString mapName);
    static QString bgEventsLabelFromName(QString mapName);
    int getWidth();
    int getHeight();
    int getSelectedBlockIndex(int);
    int getDisplayedBlockIndex(int);
    QPixmap render(bool ignoreCache);
    QPixmap renderMetatiles();

    QPixmap renderCollision(bool ignoreCache);
    QImage collision_image;
    QPixmap collision_pixmap;
    QImage getCollisionMetatileImage(Block);
    QImage getElevationMetatileImage(Block);
    QImage getCollisionMetatileImage(int);
    QImage getElevationMetatileImage(int);

    QPixmap renderCollisionMetatiles();
    QPixmap renderElevationMetatiles();
    void drawSelection(int i, int w, int selectionWidth, int selectionHeight, QPainter *painter);

    bool blockChanged(int, Blockdata*);
    void cacheBlockdata();
    void cacheCollision();
    QImage image;
    QPixmap pixmap;
    QList<QImage> metatile_images;
    bool smart_paths_enabled = false;
    int paint_metatile_initial_x;
    int paint_metatile_initial_y;
    int paint_tile_index;
    int paint_tile_width = 1;
    int paint_tile_height = 1;
    int paint_tile_initial_x;
    int paint_tile_initial_y;
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

    QList<Event*> getAllEvents();
    void removeEvent(Event *event);
    void addEvent(Event *event);
    QMap<QString, QList<Event*>> events;

    QList<Connection*> connections;
    QList<QGraphicsPixmapItem*> connection_items;
    QPixmap renderConnection(Connection);

    QPixmap renderBorder();
    void cacheBorder();

    bool hasUnsavedChanges();
    void hoveredTileChanged(int x, int y, int block);
    void clearHoveredTile();
    void hoveredMetatileChanged(int block);
    void clearHoveredMetatile();
    void hoveredCollisionTileChanged(int collision);
    void clearHoveredCollisionTile();
    void hoveredElevationTileChanged(int elevation);
    void clearHoveredElevationTile();

signals:
    void paintTileChanged(Map *map);
    void paintCollisionChanged(Map *map);
    void mapChanged(Map *map);
    void statusBarMessage(QString);

public slots:
};

#endif // MAP_H
