#include "history.h"
#include "map.h"
#include "imageproviders.h"
#include "scripting.h"

#include "editcommands.h"

#include <QTime>
#include <QPainter>
#include <QImage>
#include <QRegularExpression>


Map::Map(QObject *parent) : QObject(parent)
{
    editHistory.setClean();
}

Map::~Map() {
    qDeleteAll(ownedEvents);
    ownedEvents.clear();
    deleteConnections();
}

void Map::setName(QString mapName) {
    name = mapName;
    constantName = mapConstantFromName(mapName);
    scriptsLoaded = false;
}

void Map::setLayout(Layout *layout) {
    this->layout = layout;
    if (layout) {
        this->layoutId = layout->id;
    }
}

QString Map::mapConstantFromName(QString mapName, bool includePrefix) {
    // Transform map names of the form 'GraniteCave_B1F` into map constants like 'MAP_GRANITE_CAVE_B1F'.
    static const QRegularExpression caseChange("([a-z])([A-Z])");
    QString nameWithUnderscores = mapName.replace(caseChange, "\\1_\\2");
    const QString prefix = includePrefix ? projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix) : "";
    QString withMapAndUppercase = prefix + nameWithUnderscores.toUpper();
    static const QRegularExpression underscores("_+");
    QString constantName = withMapAndUppercase.replace(underscores, "_");

    // Handle special cases.
    // SSTidal needs to be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

int Map::getWidth() {
    return layout->getWidth();
}

int Map::getHeight() {
    return layout->getHeight();
}

int Map::getBorderWidth() {
    return layout->getBorderWidth();
}

int Map::getBorderHeight() {
    return layout->getBorderHeight();
}

// Get the portion of the map that can be rendered when rendered as a map connection.
// Cardinal connections render the nearest segment of their map and within the bounds of the border draw distance,
// Dive/Emerge connections are rendered normally within the bounds of their parent map.
QRect Map::getConnectionRect(const QString &direction, Layout * fromLayout) {
    int x = 0, y = 0;
    int w = getWidth(), h = getHeight();

    if (direction == "up") {
        h = qMin(h, BORDER_DISTANCE);
        y = getHeight() - h;
    } else if (direction == "down") {
        h = qMin(h, BORDER_DISTANCE);
    } else if (direction == "left") {
        w = qMin(w, BORDER_DISTANCE);
        x = getWidth() - w;
    } else if (direction == "right") {
        w = qMin(w, BORDER_DISTANCE);
    } else if (MapConnection::isDiving(direction)) {
        if (fromLayout) {
            w = qMin(w, fromLayout->getWidth());
            h = qMin(h, fromLayout->getHeight());
        }
    } else {
        // Unknown direction
        return QRect();
    }
    return QRect(x, y, w, h);
}

QPixmap Map::renderConnection(const QString &direction, Layout * fromLayout) {
    QRect bounds = getConnectionRect(direction, fromLayout);
    if (!bounds.isValid())
        return QPixmap();

    // 'fromLayout' will be used in 'render' to get the palettes from the parent map.
    // Dive/Emerge connections render normally with their own palettes, so we ignore this.
    if (MapConnection::isDiving(direction))
        fromLayout = nullptr;

    QPixmap connectionPixmap = this->layout->render(true, fromLayout, bounds);
    return connectionPixmap.copy(bounds.x() * 16, bounds.y() * 16, bounds.width() * 16, bounds.height() * 16);
}

void Map::openScript(QString label) {
    emit openScriptRequested(label);
}

QList<Event *> Map::getAllEvents() const {
    QList<Event *> all_events;
    for (const auto &event_list : events) {
        all_events << event_list;
    }
    return all_events;
}

QStringList Map::getScriptLabels(Event::Group group) {
    if (!this->scriptsLoaded) {
        this->scriptsFileLabels = ParseUtil::getGlobalScriptLabels(this->getScriptsFilePath());
        this->scriptsLoaded = true;
    }

    QStringList scriptLabels;

    // Get script labels currently in-use by the map's events
    if (group == Event::Group::None) {
        ScriptTracker scriptTracker;
        for (Event *event : this->getAllEvents()) {
            event->accept(&scriptTracker);
        }
        scriptLabels = scriptTracker.getScripts();
    } else {
        ScriptTracker scriptTracker;
        for (Event *event : events.value(group)) {
            event->accept(&scriptTracker);
        }
        scriptLabels = scriptTracker.getScripts();
    }

    // Add scripts from map's scripts file, and empty names.
    scriptLabels.append(this->scriptsFileLabels);
    scriptLabels.sort(Qt::CaseInsensitive);
    scriptLabels.prepend("0x0");
    scriptLabels.prepend("NULL");

    scriptLabels.removeAll("");
    scriptLabels.removeDuplicates();

    return scriptLabels;
}

QString Map::getScriptsFilePath() const {
    const bool usePoryscript = projectConfig.usePoryScript;
    auto path = QDir::cleanPath(QString("%1/%2/%3/scripts")
                                        .arg(projectConfig.projectDir)
                                        .arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders))
                                        .arg(this->name));
    auto extension = Project::getScriptFileExtension(usePoryscript);
    if (usePoryscript && !QFile::exists(path + extension))
        extension = Project::getScriptFileExtension(false);
    path += extension;
    return path;
}

void Map::removeEvent(Event *event) {
    for (Event::Group key : events.keys()) {
        events[key].removeAll(event);
    }
}

void Map::addEvent(Event *event) {
    event->setMap(this);
    events[event->getEventGroup()].append(event);
    if (!ownedEvents.contains(event)) ownedEvents.append(event);
}

void Map::deleteConnections() {
    qDeleteAll(this->ownedConnections);
    this->ownedConnections.clear();
    this->connections.clear();
}

QList<MapConnection*> Map::getConnections() const {
    return this->connections;
}

void Map::addConnection(MapConnection *connection) {
    if (!connection || this->connections.contains(connection))
        return;

    // Maps should only have one Dive/Emerge connection at a time.
    // (Users can technically have more by editing their data manually, but we will only display one at a time)
    // Any additional connections being added (this can happen via mirroring) are tracked for deleting but otherwise ignored.
    if (MapConnection::isDiving(connection->direction())) {
        for (auto i : this->connections) {
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

    if (!this->connections.contains(connection))
        this->connections.append(connection);

    trackConnection(connection);
}

void Map::trackConnection(MapConnection *connection) {
    connection->setParentMap(this, false);

    if (!this->ownedConnections.contains(connection)) {
        this->ownedConnections.insert(connection);
        connect(connection, &MapConnection::parentMapChanged, [=](Map *, Map *after) {
            if (after != this && after != nullptr) {
                // MapConnection's parent has been reassigned, it's no longer our responsibility
                this->ownedConnections.remove(connection);
                QObject::disconnect(connection, &MapConnection::parentMapChanged, this, nullptr);
            }
        });
    }
}

// We retain ownership of this MapConnection until it's assigned to a new parent map.
void Map::removeConnection(MapConnection *connection) {
    if (!this->connections.removeOne(connection))
        return;
    connection->setParentMap(nullptr, false);
    modify();
    emit connectionRemoved(connection);
}

void Map::modify() {
    emit modified();
}

void Map::clean() {
    this->hasUnsavedDataChanges = false;
}

bool Map::hasUnsavedChanges() const {
    return !editHistory.isClean() || this->layout->hasUnsavedChanges() || hasUnsavedDataChanges || !isPersistedToFile;
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
    for (int i = 0; i < this->editHistory.count(); i++) {
        // Qt really doesn't expect editing commands in the stack to be valid (fair).
        // A better future design might be to have separate edit histories per map tab,
        // and dumping the entire Connections tab history with QUndoStack::clear.
        auto command = const_cast<QUndoCommand*>(this->editHistory.command(i));
        if (mapConnectionIds.contains(command->id()))
            command->setObsolete(true);
    }
}
