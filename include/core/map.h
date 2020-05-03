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

#define DEFAULT_BORDER_WIDTH 2
#define DEFAULT_BORDER_HEIGHT 2

// Number of metatiles to draw out from edge of map. Could allow modification of this in the future.
// porymap will reflect changes to it, but the value is hard-coded in the projects at the moment
#define BORDER_DISTANCE 7

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = nullptr);

public:
    QString name;
    QString constantName;
    QString group_num;
    QString song;
    QString layoutId;
    QString location;
    QString requiresFlash;
    QString isFlyable;
    QString weather;
    QString type;
    QString show_location;
    QString allowRunning;
    QString allowBiking;
    QString allowEscapeRope;
    int floorNumber;
    QString battle_scene;
    QString sharedEventsMap = "";
    QString sharedScriptsMap = "";
    QMap<QString, QString> customHeaders;
    MapLayout *layout;
    bool isPersistedToFile = true;
    bool needsLayoutDir = true;
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
    int getBorderWidth();
    int getBorderHeight();
    QPixmap render(bool ignoreCache, MapLayout * fromLayout = nullptr);
    QPixmap renderCollision(qreal opacity, bool ignoreCache);
    bool mapBlockChanged(int i, Blockdata * cache);
    bool borderBlockChanged(int i, Blockdata * cache);
    void cacheBlockdata();
    void cacheCollision();
    Block *getBlock(int x, int y);
    void setBlock(int x, int y, Block block, bool enableScriptCallback = false);
    void floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void _floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void magicFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void undo();
    void redo();
    void commit();
    QList<Event*> getAllEvents();
    void removeEvent(Event*);
    void addEvent(Event*);
    QPixmap renderConnection(MapConnection, MapLayout *);
    QPixmap renderBorder(bool ignoreCache = false);
    void setDimensions(int newWidth, int newHeight, bool setNewBlockdata = true);
    void setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata = true);
    void cacheBorder();
    bool hasUnsavedChanges();

private:
    void setNewDimensionsBlockdata(int newWidth, int newHeight);
    void setNewBorderDimensionsBlockdata(int newWidth, int newHeight);

signals:
    void mapChanged(Map *map);
    void mapNeedsRedrawing();
};

#endif // MAP_H
