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
    QString allowRunning;
    QString allowBiking;
    QString allowEscapeRope;
    QString battle_scene;
    MapLayout *layout;
    bool isPersistedToFile = true;
    QImage collision_image;
    QPixmap collision_pixmap;
    QImage image;
    QPixmap pixmap;
    History<HistoryItem*> metatileHistory;
    QMap<QString, QList<Event*>> events;
    QList<MapConnection*> connections;
    void setName(QString mapName);
    static QString mapConstantFromName(QString mapName);
    static QString objectEventsLabelFromName(QString mapName);
    static QString warpEventsLabelFromName(QString mapName);
    static QString coordEventsLabelFromName(QString mapName);
    static QString bgEventsLabelFromName(QString mapName);
    int getWidth();
    int getHeight();
    QPixmap render(bool ignoreCache);
    QPixmap renderCollision(bool ignoreCache);
    bool blockChanged(int, Blockdata*);
    void cacheBlockdata();
    void cacheCollision();
    Block *getBlock(int x, int y);
    void setBlock(int x, int y, Block block);
    void _setBlock(int x, int y, Block block);
    void floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void _floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void undo();
    void redo();
    void commit();
    QList<Event*> getAllEvents();
    void removeEvent(Event*);
    void addEvent(Event*);
    QPixmap renderConnection(MapConnection);
    QPixmap renderBorder();
    void setDimensions(int newWidth, int newHeight, bool setNewBlockData = true);
    void cacheBorder();
    bool hasUnsavedChanges();

private:
    void setNewDimensionsBlockdata(int newWidth, int newHeight);

signals:
    void mapChanged(Map *map);
    void mapNeedsRedrawing();
};

#endif // MAP_H
