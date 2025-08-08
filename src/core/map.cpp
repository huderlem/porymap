#include "history.h"
#include "map.h"
#include "imageproviders.h"
#include "scripting.h"
#include "utility.h"
#include "editcommands.h"
#include "project.h"

#include <QTime>
#include <QPainter>
#include <QImage>
#include <QRegularExpression>


Map::Map(QObject *parent) : QObject(parent)
{
    m_editHistory = new QUndoStack(this);

    resetEvents();

    m_header = new MapHeader(this);
    connect(m_header, &MapHeader::modified, this, &Map::modified);
}

Map::Map(const Map &other, QObject *parent) : Map(parent) {
    m_name = other.m_name;
    m_constantName = other.m_constantName;
    m_sharedEventsMap = other.m_sharedEventsMap;
    m_sharedScriptsMap = other.m_sharedScriptsMap;
    m_customAttributes = other.m_customAttributes;
    *m_header = *other.m_header;
    m_layout = other.m_layout;
    m_isPersistedToFile = false;

    // Copy events
    for (auto i = other.m_events.constBegin(); i != other.m_events.constEnd(); i++) {
        for (const auto &event : i.value())
            addEvent(event->duplicate());
    }

    // Duplicating the map connections is probably not desirable, so we skip them.
}

Map::~Map() {
    qDeleteAll(m_ownedEvents);
    m_ownedEvents.clear();
    deleteConnections();
}

// Note: Map does not take ownership of layout
void Map::setLayout(Layout *layout) {
    if (layout == m_layout)
        return;
    m_layout = layout;
    if (layout) {
        m_layoutId = layout->id;
    }
    emit layoutChanged();
}

// We don't enforce this for existing maps, but for creating new maps we need to formulaically generate a new MAP_NAME ID.
QString Map::mapConstantFromName(const QString &name) {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix) + Util::toDefineCase(name);
}

// Get the portion of the map that can be rendered when rendered as a map connection.
// Cardinal connections render the nearest segment of their map and within the bounds of the border draw distance,
// Dive/Emerge connections are rendered normally within the bounds of their parent map.
QRect Map::getConnectionRect(const QString &direction, Layout * fromLayout) const {
    int x = 0, y = 0;
    int w = pixelWidth(), h = pixelHeight();

    QMargins viewDistance = Project::getPixelViewDistance();
    if (direction == "up") {
        h = qMin(h, viewDistance.top());
        y = pixelHeight() - h;
    } else if (direction == "down") {
        h = qMin(h, viewDistance.bottom());
    } else if (direction == "left") {
        w = qMin(w, viewDistance.left());
        x = pixelWidth() - w;
    } else if (direction == "right") {
        w = qMin(w, viewDistance.right());
    } else if (MapConnection::isDiving(direction)) {
        if (fromLayout) {
            w = qMin(w, fromLayout->pixelWidth());
            h = qMin(h, fromLayout->pixelHeight());
        }
    } else {
        // Unknown direction
        return QRect();
    }
    return QRect(x, y, w, h);
}

QPixmap Map::renderConnection(const QString &direction, Layout * fromLayout) {
    if (!m_layout)
        return QPixmap();

    QRect bounds = getConnectionRect(direction, fromLayout);
    if (!bounds.isValid())
        return QPixmap();

    // 'fromLayout' will be used in 'render' to get the palettes from the parent map.
    // Dive/Emerge connections render normally with their own palettes, so we ignore this.
    if (MapConnection::isDiving(direction))
        fromLayout = nullptr;

    QPixmap connectionPixmap = m_layout->render(true, fromLayout, bounds);
    return connectionPixmap.copy(bounds);
}

void Map::openScript(const QString &label) {
    emit openScriptRequested(label);
}

void Map::setSharedScriptsMap(const QString &sharedScriptsMap) {
    if (m_sharedScriptsMap == sharedScriptsMap)
        return;
    m_sharedScriptsMap = sharedScriptsMap;
    invalidateScripts();
}

void Map::invalidateScripts() {
    m_scriptsLoaded = false;

    // m_scriptFileWatcher is a QPointer so clearing it shouldn't be necessary,
    // but it's possible that Map::getScriptLabels will be called before events are processed.
    delete m_scriptFileWatcher;
    m_scriptFileWatcher = nullptr;

    emit scriptsModified();
}

QStringList Map::getScriptLabels(Event::Group group) {
    if (!m_scriptsLoaded && m_isPersistedToFile) {
        const QString scriptsFilepath = getScriptsFilepath();
        QString error;
        m_scriptLabels = ParseUtil::getGlobalScriptLabels(scriptsFilepath, &error);

        if (!error.isEmpty() && !m_loggedScriptsFileError) {
            logWarn(QString("Failed to read scripts file '%1' for %2: %3")
                            .arg(Util::stripPrefix(scriptsFilepath, projectConfig.projectDir() + "/"))
                            .arg(m_name)
                            .arg(error));

            // Setting this flag here (and below) lets us skip some steps and logging if we already know it failed.
            // Script labels may be re-requested often, so we don't want to fill the log with warnings.
            m_loggedScriptsFileError = true;
        }

        if (porymapConfig.monitorFiles && !m_loggedScriptsFileError) {
            if (!m_scriptFileWatcher) {
                // Only create the file watcher when it's first needed (even an empty QFileSystemWatcher will consume system resources).
                // The other option would be for Porymap to have a single global QFileSystemWatcher, but that has complications of its own.
                m_scriptFileWatcher = new QFileSystemWatcher(this);
                connect(m_scriptFileWatcher, &QFileSystemWatcher::fileChanged, this, &Map::invalidateScripts);
            }
            // m_scriptFileWatcher can stil be nullptr here if the inotify limit was reached on Linux.
            // Porymap isn't using enough resources in general for this to be a problem, but the user may have lowered the inotify limit.
            if (!m_scriptFileWatcher) {
                logWarn(QString("Failed to add scripts file '%1' to file watcher for %2: Reached system resource limit.")
                                .arg(Util::stripPrefix(scriptsFilepath, projectConfig.projectDir() + "/"))
                                .arg(m_name));
                m_loggedScriptsFileError = true;
            } else if (!m_scriptFileWatcher->files().contains(scriptsFilepath) && !m_scriptFileWatcher->addPath(scriptsFilepath)) {
                logWarn(QString("Failed to add scripts file '%1' to file watcher for %2.")
                                .arg(Util::stripPrefix(scriptsFilepath, projectConfig.projectDir() + "/"))
                                .arg(m_name));
                m_loggedScriptsFileError = true;
            }
        }

        m_scriptsLoaded = true;
    }

    QStringList scriptLabels = m_scriptLabels;

    // Add script labels currently in-use by the map's events
    for (const auto &event : getEvents(group)) {
        scriptLabels.append(event->getScripts());
    }

    scriptLabels.sort(Qt::CaseInsensitive);
    scriptLabels.removeAll("");
    scriptLabels.removeAll("0");
    scriptLabels.removeAll("0x0");
    scriptLabels.removeDuplicates();

    return scriptLabels;
}

QString Map::getScriptsFilepath() const {
    const bool usePoryscript = projectConfig.usePoryScript;
    auto path = QDir::cleanPath(QString("%1/%2/%3/scripts")
                                        .arg(projectConfig.projectDir())
                                        .arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders))
                                        .arg(!m_sharedScriptsMap.isEmpty() ? m_sharedScriptsMap : m_name));
    auto extension = Project::getScriptFileExtension(usePoryscript);
    if (usePoryscript && !QFile::exists(path + extension))
        extension = Project::getScriptFileExtension(false);
    path += extension;
    return path;
}

QString Map::getJsonFilepath(const QString &mapName) {
    return QDir::cleanPath(QString("%1/%2/%3/map.json")
                                        .arg(projectConfig.projectDir())
                                        .arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders))
                                        .arg(mapName));
}

void Map::resetEvents() {
    m_events[Event::Group::Object].clear();
    m_events[Event::Group::Warp].clear();
    m_events[Event::Group::Coord].clear();
    m_events[Event::Group::Bg].clear();
    m_events[Event::Group::Heal].clear();
}

QList<Event *> Map::getEvents(Event::Group group) const {
    if (group == Event::Group::None) {
        // Get all events
        QList<Event *> all_events;
        for (const auto &event_list : m_events) {
            all_events << event_list;
        }
        return all_events;
    }
    return m_events[group];
}

Event* Map::getEvent(Event::Group group, int index) const {
    return m_events[group].value(index, nullptr);
}

Event* Map::getEvent(Event::Group group, const QString &idName) const {
    if (idName.isEmpty())
        return nullptr;

    bool idIsNumber;
    int id = idName.toInt(&idIsNumber, 0);
    if (idIsNumber)
        return getEvent(group, id - Event::getIndexOffset(group));

    auto events = getEvents(group);
    for (const auto &event : events) {
        if (event->getIdName() == idName) {
            return event;
        }
    }
    return nullptr;
}

// Returns a list of ID names for the given event group (or all events, if no group is given).
// For events with no explicit ID name, their index string is given instead.
QStringList Map::getEventIdNames(Event::Group group) const {
    QList<Event::Group> groups;
    if (group == Event::Group::None) {
        groups = Event::groups();
    } else {
        groups.append(group);
    }

    QStringList idNames;
    for (const auto &group : groups) {
        const auto events = m_events[group];
        int indexOffset = Event::getIndexOffset(group);
        for (int i = 0; i < events.length(); i++) {
            QString idName = events.at(i)->getIdName();
            if (idName.isEmpty()) {
                idName = QString::number(i + indexOffset);
            }
            idNames.append(idName);
        }
    }
    return idNames;
}

int Map::getNumEvents(Event::Group group) const {
    if (group == Event::Group::None) {
        // Total number of events
        int numEvents = 0;
        for (auto it = m_events.constBegin(); it != m_events.constEnd(); it++) {
            numEvents += it.value().length();
        }
        return numEvents;
    }
    return m_events[group].length();
}

bool Map::hasEvents() const {
    for (auto it = m_events.constBegin(); it != m_events.constEnd(); it++) {
        if (!it.value().isEmpty()) {
            return true;
        }
    }
    return false;
}

void Map::removeEvent(Event *event) {
    for (auto i = m_events.begin(); i != m_events.end(); i++) {
        i.value().removeAll(event);
    }
}

void Map::addEvent(Event *event) {
    event->setMap(this);
    m_events[event->getEventGroup()].append(event);
    if (!m_ownedEvents.contains(event)) m_ownedEvents.insert(event);
}

int Map::getIndexOfEvent(Event *event) const {
    return m_events.value(event->getEventGroup()).indexOf(event);
}

bool Map::hasEvent(Event *event) const {
    return getIndexOfEvent(event) >= 0;
}

void Map::deleteConnections() {
    qDeleteAll(m_ownedConnections);
    m_ownedConnections.clear();
    m_connections.clear();
}

void Map::addConnection(MapConnection *connection) {
    if (!connection || m_connections.contains(connection))
        return;

    // Maps should only have one Dive/Emerge connection at a time.
    // (Users can technically have more by editing their data manually, but we will only display one at a time)
    // Any additional connections being added (this can happen via mirroring) are tracked for deleting but otherwise ignored.
    if (connection->isDiving()) {
        for (const auto &i : m_connections) {
            if (i->direction() == connection->direction()) {
                trackConnection(connection);
                return;
            }
        }
    }

    loadConnection(connection);
    modify();
    emit connectionAdded(connection);
    return;
}

void Map::loadConnection(MapConnection *connection) {
    if (!connection)
        return;

    if (!m_connections.contains(connection))
        m_connections.append(connection);

    trackConnection(connection);
}

void Map::trackConnection(MapConnection *connection) {
    connection->setParentMap(this, false);

    if (!m_ownedConnections.contains(connection)) {
        m_ownedConnections.insert(connection);
        connect(connection, &MapConnection::parentMapChanged, [=](Map *, Map *after) {
            if (after != this && after != nullptr) {
                // MapConnection's parent has been reassigned, it's no longer our responsibility
                m_ownedConnections.remove(connection);
                QObject::disconnect(connection, &MapConnection::parentMapChanged, this, nullptr);
            }
        });
    }
}

// We retain ownership of this MapConnection until it's assigned to a new parent map.
void Map::removeConnection(MapConnection *connection) {
    if (!m_connections.removeOne(connection))
        return;
    connection->setParentMap(nullptr, false);
    modify();
    emit connectionRemoved(connection);
}

// Return the first map connection that has the given direction.
MapConnection* Map::getConnection(const QString &direction) const {
    for (const auto &connection : m_connections) {
        if (connection->direction() == direction)
            return connection;
    }
    return nullptr;
}

void Map::commit(QUndoCommand *cmd) {
    m_editHistory->push(cmd);
}

void Map::modify() {
    emit modified();
}

void Map::setClean() {
    m_editHistory->setClean();
    m_hasUnsavedDataChanges = false;
    m_isPersistedToFile = true;
}

bool Map::hasUnsavedChanges() const {
    return !m_editHistory->isClean() || (m_layout && m_layout->hasUnsavedChanges()) || m_hasUnsavedDataChanges || !m_isPersistedToFile;
}

void Map::pruneEditHistory() {
    // Edit history for map connections gets messy because edits on other maps can affect the current map.
    // To avoid complications we clear MapConnection edit history when the user opens a different map.
    // No other edits within a single map depend on MapConnections so they can be pruned safely.
    static const QSet<int> mapConnectionIds = {
        ID_MapConnectionMove,
        ID_MapConnectionChangeDirection,
        ID_MapConnectionChangeMap,
        ID_MapConnectionAdd,
        ID_MapConnectionRemove
    };
    for (int i = 0; i < m_editHistory->count(); i++) {
        // Qt really doesn't expect editing commands in the stack to be valid (fair).
        // A better future design might be to have separate edit histories per map tab,
        // and dumping the entire Connections tab history with QUndoStack::clear.
        auto command = const_cast<QUndoCommand*>(m_editHistory->command(i));
        if (mapConnectionIds.contains(command->id()))
            command->setObsolete(true);
    }
}
