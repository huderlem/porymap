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

class LayoutPixmapItem;
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

    QMap<QString, QJsonValue> customHeaders;

    Layout *layout = nullptr;
    void setLayout(Layout *layout);

    bool isPersistedToFile = true;
    bool hasUnsavedDataChanges = false;

    bool needsLayoutDir = true;
    bool needsHealLocation = false;

    QMap<Event::Group, QList<Event *>> events;
    QList<Event *> ownedEvents; // for memory management

    QList<MapConnection*> connections;

    void setName(QString mapName);
    static QString mapConstantFromName(QString mapName);

    /// !HERE /* layout related stuff */
    int getWidth();
    int getHeight();
    int getBorderWidth();
    int getBorderHeight();

    QList<Event *> getAllEvents() const;
    QStringList eventScriptLabels(Event::Group group = Event::Group::None) const;
    void removeEvent(Event *);
    void addEvent(Event *);

    void openScript(QString label);

    QUndoStack editHistory;
    void modify();
    void clean();
    bool hasUnsavedChanges();

    QPixmap renderConnection(MapConnection, Layout *);



signals:
    void mapChanged(Map *map);
    void modified();
    void mapDimensionsChanged(const QSize &size);
    void mapNeedsRedrawing();
    void openScriptRequested(QString label);
};

#endif // MAP_H
