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
    void setName(QString mapName);
    QString name() const { return m_name; }
    QString constantName() const { return m_constantName; }

    static QString mapConstantFromName(QString mapName, bool includePrefix = true);

    void setLayout(Layout *layout);
    Layout* layout() const { return m_layout; }

    void setLayoutId(const QString &layoutId) { m_layoutId = layoutId; }
    QString layoutId() const { return m_layoutId; }

    int getWidth() const;
    int getHeight() const;
    int getBorderWidth() const;
    int getBorderHeight() const;

    // TODO: Combine these into a separate MapHeader class?
    void setSong(const QString &song);
    void setLocation(const QString &location);
    void setRequiresFlash(bool requiresFlash);
    void setWeather(const QString &weather);
    void setType(const QString &type);
    void setShowsLocation(bool showsLocation);
    void setAllowsRunning(bool allowsRunning);
    void setAllowsBiking(bool allowsBiking);
    void setAllowsEscaping(bool allowsEscaping);
    void setFloorNumber(int floorNumber);
    void setBattleScene(const QString &battleScene);

    QString song() const { return m_song; }
    QString location() const { return m_location; }
    bool requiresFlash() const { return m_requiresFlash; }
    QString weather() const { return m_weather; }
    QString type() const { return m_type; }
    bool showsLocation() const { return m_showsLocation; }
    bool allowsRunning() const { return m_allowsRunning; }
    bool allowsBiking() const { return m_allowsBiking; }
    bool allowsEscaping() const { return m_allowsEscaping; }
    int floorNumber() const { return m_floorNumber; }
    QString battleScene() const { return m_battleScene; }

    void setSharedEventsMap(const QString &sharedEventsMap) { m_sharedEventsMap = sharedEventsMap; }
    void setSharedScriptsMap(const QString &sharedScriptsMap) { m_sharedScriptsMap = sharedScriptsMap; }

    QString sharedEventsMap() const { return m_sharedEventsMap; }
    QString sharedScriptsMap() const { return m_sharedScriptsMap; }

    void setNeedsLayoutDir(bool needsLayoutDir) { m_needsLayoutDir = needsLayoutDir; }
    void setNeedsHealLocation(bool needsHealLocation) { m_needsHealLocation = needsHealLocation; }
    void setIsPersistedToFile(bool persistedToFile) { m_isPersistedToFile = persistedToFile; }
    void setHasUnsavedDataChanges(bool unsavedDataChanges) { m_hasUnsavedDataChanges = unsavedDataChanges; }

    bool needsLayoutDir() const { return m_needsLayoutDir; }
    bool needsHealLocation() const { return m_needsHealLocation; }
    bool isPersistedToFile() const { return m_isPersistedToFile; }
    bool hasUnsavedDataChanges() const { return m_hasUnsavedDataChanges; }

    void resetEvents();
    QList<Event *> getEvents(Event::Group group = Event::Group::None) const;
    Event* getEvent(Event::Group group, int index) const;
    int getNumEvents(Event::Group group = Event::Group::None) const;
    QStringList getScriptLabels(Event::Group group = Event::Group::None);
    QString getScriptsFilePath() const;
    void openScript(QString label);
    void removeEvent(Event *);
    void addEvent(Event *);
    int getIndexOfEvent(Event *) const;

    void deleteConnections();
    QList<MapConnection*> getConnections() const;
    void removeConnection(MapConnection *);
    void addConnection(MapConnection *);
    void loadConnection(MapConnection *);
    QRect getConnectionRect(const QString &direction, Layout *fromLayout = nullptr);
    QPixmap renderConnection(const QString &direction, Layout *fromLayout = nullptr);

    QUndoStack* editHistory() const { return m_editHistory; }
    void commit(QUndoCommand*);
    void modify();
    void setClean();
    bool hasUnsavedChanges() const;
    void pruneEditHistory();

    void setCustomAttributes(const QMap<QString, QJsonValue> &attributes) { m_customAttributes = attributes; }
    QMap<QString, QJsonValue> customAttributes() const { return m_customAttributes; }

private:
    QString m_name;
    QString m_constantName;
    QString m_layoutId; // TODO: Why do we do half this->layout()->id and half this->layoutId. Should these ever be different?

    QString m_song;
    QString m_location;
    bool m_requiresFlash;
    QString m_weather;
    QString m_type;
    bool m_showsLocation;
    bool m_allowsRunning;
    bool m_allowsBiking;
    bool m_allowsEscaping;
    int m_floorNumber = 0;
    QString m_battleScene;

    QString m_sharedEventsMap = "";
    QString m_sharedScriptsMap = "";

    QStringList m_scriptsFileLabels;
    QMap<QString, QJsonValue> m_customAttributes;

    Layout *m_layout = nullptr;

    bool m_isPersistedToFile = true;
    bool m_hasUnsavedDataChanges = false;
    bool m_needsLayoutDir = true;
    bool m_needsHealLocation = false;
    bool m_scriptsLoaded = false;

    QMap<Event::Group, QList<Event *>> m_events;
    QList<Event *> m_ownedEvents; // for memory management

    QList<int> m_metatileLayerOrder;
    QList<float> m_metatileLayerOpacity;

    void trackConnection(MapConnection*);

    // MapConnections in 'ownedConnections' but not 'connections' persist in the edit history.
    QList<MapConnection*> m_connections;
    QSet<MapConnection*> m_ownedConnections;

    QUndoStack *m_editHistory = nullptr;

signals:
    void modified();
    void mapDimensionsChanged(const QSize &size);
    void openScriptRequested(QString label);
    void connectionAdded(MapConnection*);
    void connectionRemoved(MapConnection*);
};

#endif // MAP_H
