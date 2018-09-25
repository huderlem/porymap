#ifndef MAP_H
#define MAP_H

#include "blockdata.h"
#include "history.h"
#include "historyitem.h"
#include "mapconnection.h"
#include "maplayout.h"
#include "tileset.h"
#include "event.h"

#include <QPixmap>
#include <QObject>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <math.h>

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = nullptr);

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
    QString requiresFlash;
    QString isFlyable; // TODO: implement this
    QString weather;
    QString type;
    QString unknown;
    QString show_location;
    QString battle_scene;
    MapLayout *layout;
    int scale_exp = 0;
    double scale_base = sqrt(2); // adjust scale factor with this

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
    uint16_t getSelectedBlockIndex(int);
    int getDisplayedBlockIndex(int);
    QPixmap render(bool ignoreCache);

    QPixmap renderCollision(bool ignoreCache);
    QImage collision_image;
    QPixmap collision_pixmap;
    QImage getCollisionMetatileImage(Block);
    QImage getCollisionMetatileImage(int, int);

    void drawSelection(int i, int w, int selectionWidth, int selectionHeight, QPainter *painter, int gridWidth);

    bool blockChanged(int, Blockdata*);
    void cacheBlockdata();
    void cacheCollision();
    QImage image;
    QPixmap pixmap;
    QList<QImage> metatile_images;
    bool smart_paths_enabled = false;
    int paint_tile_initial_x;
    int paint_tile_initial_y;

    Block *getBlock(int x, int y);
    void setBlock(int x, int y, Block block);
    void _setBlock(int x, int y, Block block);

    void floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void _floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);

    History<HistoryItem*> history;
    void undo();
    void redo();
    void commit();

    QList<Event*> getAllEvents();
    void removeEvent(Event *event);
    void addEvent(Event *event);
    QMap<QString, QList<Event*>> events;

    QList<MapConnection*> connections;
    QPixmap renderConnection(MapConnection);
    void setNewDimensionsBlockdata(int newWidth, int newHeight);
    void setDimensions(int newWidth, int newHeight, bool setNewBlockData = true);

    QPixmap renderBorder();
    void cacheBorder();

    bool hasUnsavedChanges();
    void hoveredTileChanged(int x, int y, int block);
    void clearHoveredTile();
    void hoveredMetatileSelectionChanged(uint16_t);
    void clearHoveredMetatileSelection();
    void clearHoveredMovementPermissionTile();

signals:
    void mapChanged(Map *map);
    void mapNeedsRedrawing();
    void statusBarMessage(QString);

public slots:
    void hoveredMovementPermissionTileChanged(int collision, int elevation);
};

#endif // MAP_H
