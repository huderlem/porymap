#pragma once
#ifndef MAP_H
#define MAP_H

#include "blockdata.h"
#include "mapconnection.h"
#include "maplayout.h"
#include "tileset.h"
#include "events.h"
#include "mapheader.h"

#include <QUndoStack>
#include <QPixmap>
#include <QObject>
#include <QGraphicsPixmapItem>
#include <QFileSystemWatcher>
#include <math.h>

#define DEFAULT_BORDER_WIDTH 2
#define DEFAULT_BORDER_HEIGHT 2

// Maximum based only on data type (u8) of map border width/height
#define MAX_BORDER_WIDTH 255
#define MAX_BORDER_HEIGHT 255

class LayoutPixmapItem;
class CollisionPixmapItem;
class BorderMetatilesPixmapItem;

class Map : public QObject
{
    Q_OBJECT
public:
    explicit Map(QObject *parent = nullptr);
    explicit Map(const Map &other, QObject *parent = nullptr);
    ~Map();

public:
    void setName(const QString &mapName) { m_name = mapName; }
    QString name() const { return m_name; }

    void setConstantName(const QString &constantName) { m_constantName = constantName; }
    QString constantName() const { return m_constantName; }

    static QString mapConstantFromName(const QString &name);
    QString expectedConstantName() const { return Map::mapConstantFromName(m_name); }

    void setLayout(Layout *layout);
    Layout* layout() const { return m_layout; }

    void setLayoutId(const QString &layoutId) { m_layoutId = layoutId; }
    QString layoutId() const { return layout() ? layout()->id : m_layoutId; }

    int getWidth() const;
    int getHeight() const;
    int getBorderWidth() const;
    int getBorderHeight() const;

    void setHeader(const MapHeader &header) { *m_header = header; }
    MapHeader* header() const { return m_header; }

    void setSharedEventsMap(const QString &sharedEventsMap) { m_sharedEventsMap = sharedEventsMap; }
    void setSharedScriptsMap(const QString &sharedScriptsMap);

    QString sharedEventsMap() const { return m_sharedEventsMap; }
    QString sharedScriptsMap() const { return m_sharedScriptsMap; }

    void setNeedsHealLocation(bool needsHealLocation) { m_needsHealLocation = needsHealLocation; }
    void setIsPersistedToFile(bool persistedToFile) { m_isPersistedToFile = persistedToFile; }
    void setHasUnsavedDataChanges(bool unsavedDataChanges) { m_hasUnsavedDataChanges = unsavedDataChanges; }

    bool needsHealLocation() const { return m_needsHealLocation; }
    bool isPersistedToFile() const { return m_isPersistedToFile; }
    bool hasUnsavedDataChanges() const { return m_hasUnsavedDataChanges; }

    void resetEvents();
    QList<Event *> getEvents(Event::Group group = Event::Group::None) const;
    Event* getEvent(Event::Group group, int index) const;
    Event* getEvent(Event::Group group, const QString &idName) const;
    QStringList getEventIdNames(Event::Group group) const;
    int getNumEvents(Event::Group group = Event::Group::None) const;
    void removeEvent(Event *);
    void addEvent(Event *);
    int getIndexOfEvent(Event *) const;
    bool hasEvent(Event *) const;

    QStringList getScriptLabels(Event::Group group = Event::Group::None);
    QString getScriptsFilepath() const;
    void openScript(const QString &label);

    static QString getJsonFilepath(const QString &mapName);
    QString getJsonFilepath() const { return getJsonFilepath(m_name); }

    void deleteConnections();
    QList<MapConnection*> getConnections() const { return m_connections; }
    MapConnection* getConnection(const QString &direction) const;
    void removeConnection(MapConnection *);
    void addConnection(MapConnection *);
    void loadConnection(MapConnection *);
    QRect getConnectionRect(const QString &direction, Layout *fromLayout = nullptr) const;
    QPixmap renderConnection(const QString &direction, Layout *fromLayout = nullptr);

    QUndoStack* editHistory() const { return m_editHistory; }
    void commit(QUndoCommand*);
    void modify();
    void setClean();
    bool hasUnsavedChanges() const;
    void pruneEditHistory();

    void setCustomAttributes(const QJsonObject &attributes) { m_customAttributes = attributes; }
    QJsonObject customAttributes() const { return m_customAttributes; }

private:
    QString m_name;
    QString m_constantName;
    QString m_layoutId; // Only needed if layout fails to load.
    QString m_sharedEventsMap = "";
    QString m_sharedScriptsMap = "";

    QStringList m_scriptLabels;
    QJsonObject m_customAttributes;

    MapHeader *m_header = nullptr;
    Layout *m_layout = nullptr;

    bool m_isPersistedToFile = true;
    bool m_hasUnsavedDataChanges = false;
    bool m_needsHealLocation = false;
    bool m_scriptsLoaded = false;
    bool m_loggedScriptsFileError = false;

    QMap<Event::Group, QList<Event *>> m_events;
    QSet<Event *> m_ownedEvents; // for memory management

    QList<int> m_metatileLayerOrder;
    QList<float> m_metatileLayerOpacity;

    void trackConnection(MapConnection*);

    // MapConnections in 'ownedConnections' but not 'connections' persist in the edit history.
    QList<MapConnection*> m_connections;
    QSet<MapConnection*> m_ownedConnections;

    QPointer<QUndoStack> m_editHistory;
    QPointer<QFileSystemWatcher> m_scriptFileWatcher;

    void invalidateScripts();

signals:
    void modified();
    void scriptsModified();
    void mapDimensionsChanged(const QSize &size);
    void openScriptRequested(QString label);
    void connectionAdded(MapConnection*);
    void connectionRemoved(MapConnection*);
    void layoutChanged();
};

#endif // MAP_H
