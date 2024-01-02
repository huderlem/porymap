#pragma once
#ifndef MAP_H
#define MAP_H

#include "blockdata.h"
#include "mapconnection.h"
#include "maplayout.h"
#include "tileset.h"
#include "events.h"

#include <QUndoStack>
#include <QPixmap>
#include <QObject>
#include <QGraphicsPixmapItem>
#include <math.h>

#define DEFAULT_BORDER_WIDTH 2
#define DEFAULT_BORDER_HEIGHT 2

// Maximum based only on data type (u8) of map border width/height
#define MAX_BORDER_WIDTH 255
#define MAX_BORDER_HEIGHT 255

// Number of metatiles to draw out from edge of map. Could allow modification of this in the future.
// porymap will reflect changes to it, but the value is hard-coded in the projects at the moment
#define BORDER_DISTANCE 7

class MapPixmapItem;
class CollisionPixmapItem;
class BorderMetatilesPixmapItem;

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = nullptr);
    ~Map();

public:
    QString name;
    QString constantName;
    QString song;
    QString layoutId;
    QString location;
    bool requiresFlash;
    QString weather;
    QString type;
    bool show_location;
    bool allowRunning;
    bool allowBiking;
    bool allowEscaping;
    int floorNumber = 0;
    QString battle_scene;
    QString sharedEventsMap = "";
    QString sharedScriptsMap = "";
    QStringList scriptsFileLabels;
    QMap<QString, QJsonValue> customHeaders;
    MapLayout *layout;
    bool isPersistedToFile = true;
    bool hasUnsavedDataChanges = false;
    bool needsLayoutDir = true;
    bool needsHealLocation = false;
    QImage collision_image;
    QPixmap collision_pixmap;
    QImage image;
    QPixmap pixmap;

    QMap<Event::Group, QList<Event *>> events;
    QList<Event *> ownedEvents; // for memory management

    QList<MapConnection*> connections;
    QList<int> metatileLayerOrder;
    QList<float> metatileLayerOpacity;

    void setName(QString mapName);
    static QString mapConstantFromName(QString mapName, bool includePrefix = true);
    int getWidth();
    int getHeight();
    int getBorderWidth();
    int getBorderHeight();
    QPixmap render(bool ignoreCache = false, MapLayout *fromLayout = nullptr, QRect bounds = QRect(0, 0, -1, -1));
    QPixmap renderCollision(bool ignoreCache);
    bool mapBlockChanged(int i, const Blockdata &cache);
    bool borderBlockChanged(int i, const Blockdata &cache);
    void cacheBlockdata();
    void cacheCollision();
    bool getBlock(int x, int y, Block *out);
    void setBlock(int x, int y, Block block, bool enableScriptCallback = false);
    void setBlockdata(Blockdata blockdata, bool enableScriptCallback = false);
    uint16_t getBorderMetatileId(int x, int y);
    void setBorderMetatileId(int x, int y, uint16_t metatileId, bool enableScriptCallback = false);
    void setBorderBlockData(Blockdata blockdata, bool enableScriptCallback = false);
    void floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void _floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void magicFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    QList<Event *> getAllEvents() const;
    QStringList getScriptLabels(Event::Group group = Event::Group::None) const;
    void removeEvent(Event *);
    void addEvent(Event *);
    QPixmap renderConnection(MapConnection, MapLayout *);
    QPixmap renderBorder(bool ignoreCache = false);
    void setDimensions(int newWidth, int newHeight, bool setNewBlockdata = true, bool enableScriptCallback = false);
    void setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata = true, bool enableScriptCallback = false);
    void clearBorderCache();
    void cacheBorder();
    bool hasUnsavedChanges();
    bool isWithinBounds(int x, int y);
    bool isWithinBorderBounds(int x, int y);
    void openScript(QString label);
    QString getScriptsFilePath() const;

    MapPixmapItem *mapItem = nullptr;
    void setMapItem(MapPixmapItem *item) { mapItem = item; }

    CollisionPixmapItem *collisionItem = nullptr;
    void setCollisionItem(CollisionPixmapItem *item) { collisionItem = item; }

    BorderMetatilesPixmapItem *borderItem = nullptr;
    void setBorderItem(BorderMetatilesPixmapItem *item) { borderItem = item; }

    QUndoStack editHistory;
    void modify();
    void clean();

private:
    void setNewDimensionsBlockdata(int newWidth, int newHeight);
    void setNewBorderDimensionsBlockdata(int newWidth, int newHeight);

signals:
    void mapChanged(Map *map);
    void modified();
    void mapDimensionsChanged(const QSize &size);
    void mapNeedsRedrawing();
    void openScriptRequested(QString label);
};

#endif // MAP_H
