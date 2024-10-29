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

    QStringList scriptsFileLabels;
    QMap<QString, QJsonValue> customHeaders;

    Layout *layout = nullptr;
    void setLayout(Layout *layout);

    bool isPersistedToFile = true;
    bool hasUnsavedDataChanges = false;

    bool needsLayoutDir = true;
    bool needsHealLocation = false;
    bool scriptsLoaded = false;

    QMap<Event::Group, QList<Event *>> events;
    QList<Event *> ownedEvents; // for memory management

    QList<int> metatileLayerOrder;
    QList<float> metatileLayerOpacity;

    void setName(QString mapName);
    static QString mapConstantFromName(QString mapName, bool includePrefix = true);

    int getWidth();
    int getHeight();
    int getBorderWidth();
    int getBorderHeight();

    QList<Event *> getAllEvents() const;
    QStringList getScriptLabels(Event::Group group = Event::Group::None);
    QString getScriptsFilePath() const;
    void openScript(QString label);
    void removeEvent(Event *);
    void addEvent(Event *);

    void deleteConnections();
    QList<MapConnection*> getConnections() const;
    void removeConnection(MapConnection *);
    void addConnection(MapConnection *);
    void loadConnection(MapConnection *);
    QRect getConnectionRect(const QString &direction, Layout *fromLayout = nullptr);
    QPixmap renderConnection(const QString &direction, Layout *fromLayout = nullptr);

    QUndoStack editHistory;
    void modify();
    void clean();
    bool hasUnsavedChanges();
    void pruneEditHistory();

private:
    void trackConnection(MapConnection*);

    // MapConnections in 'ownedConnections' but not 'connections' persist in the edit history.
    QList<MapConnection*> connections;
    QSet<MapConnection*> ownedConnections;

signals:
    void modified();
    void mapDimensionsChanged(const QSize &size);
    void openScriptRequested(QString label);
    void connectionAdded(MapConnection*);
    void connectionRemoved(MapConnection*);
};

#endif // MAP_H
