#include "editor.h"
#include "event.h"
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>

Editor::Editor(Ui::MainWindow* ui)
{
    this->ui = ui;
    selected_events = new QList<DraggablePixmapItem*>;
}

void Editor::saveProject() {
    if (project) {
        project->saveAllMaps();
        project->saveAllDataStructures();
    }
}

void Editor::save() {
    if (project && map) {
        project->saveMap(map);
        project->saveAllDataStructures();
    }
}

void Editor::undo() {
    if (current_view) {
        map->undo();
        map_item->draw();
    }
}

void Editor::redo() {
    if (current_view) {
        map->redo();
        map_item->draw();
    }
}

void Editor::setEditingMap() {
    current_view = map_item;
    if (map_item) {
        displayMapConnections();
        map_item->draw();
        map_item->setVisible(true);
        map_item->setEnabled(true);
        setConnectionsVisibility(true);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(true);
    setConnectionItemsVisible(false);
}

void Editor::setEditingCollision() {
    current_view = collision_item;
    if (collision_item) {
        displayMapConnections();
        collision_item->draw();
        collision_item->setVisible(true);
        setConnectionsVisibility(true);
    }
    if (map_item) {
        map_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(true);
    setConnectionItemsVisible(false);
}

void Editor::setEditingObjects() {
    current_view = map_item;
    if (events_group) {
        events_group->setVisible(true);
    }
    if (map_item) {
        map_item->setVisible(true);
        map_item->setEnabled(false);
        setConnectionsVisibility(true);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    setBorderItemsVisible(true);
    setConnectionItemsVisible(false);
}

void Editor::setEditingConnections() {
    current_view = map_item;
    if (map_item) {
        map_item->draw();
        map_item->setVisible(true);
        map_item->setEnabled(false);
        populateConnectionMapPickers();
        ui->label_NumConnections->setText(QString::number(map->connections.length()));
        setConnectionsVisibility(false);
        setDiveEmergeControls();
        setConnectionEditControlsEnabled(selected_connection_item != NULL);
        if (selected_connection_item) {
            onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
            setConnectionMap(selected_connection_item->connection->map_name);
            setCurrentConnectionDirection(selected_connection_item->connection->direction);
        }
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(true, 0.4);
    setConnectionItemsVisible(true);
}

void Editor::setDiveEmergeControls() {
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
    ui->comboBox_DiveMap->setCurrentText("");
    ui->comboBox_EmergeMap->setCurrentText("");
    for (Connection* connection : map->connections) {
        if (connection->direction == "dive") {
            ui->comboBox_DiveMap->setCurrentText(connection->map_name);
        } else if (connection->direction == "emerge") {
            ui->comboBox_EmergeMap->setCurrentText(connection->map_name);
        }
    }
    ui->comboBox_DiveMap->blockSignals(false);
    ui->comboBox_EmergeMap->blockSignals(false);
}

void Editor::populateConnectionMapPickers() {
    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);

    ui->comboBox_ConnectedMap->clear();
    ui->comboBox_ConnectedMap->addItems(*project->mapNames);
    ui->comboBox_DiveMap->clear();
    ui->comboBox_DiveMap->addItems(*project->mapNames);
    ui->comboBox_EmergeMap->clear();
    ui->comboBox_EmergeMap->addItems(*project->mapNames);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
}

void Editor::setConnectionItemsVisible(bool visible) {
    for (ConnectionPixmapItem* item : connection_edit_items) {
        item->setVisible(visible);
        item->setEnabled(visible);
    }
}

void Editor::setBorderItemsVisible(bool visible, qreal opacity) {
    for (QGraphicsPixmapItem* item : borderItems) {
        item->setVisible(visible);
        item->setOpacity(opacity);
    }
}

void Editor::setCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;

    selected_connection_item->connection->direction = curDirection;

    Map *connected_map = project->getMap(selected_connection_item->connection->map_name);
    QPixmap pixmap = connected_map->renderConnection(*selected_connection_item->connection);
    int offset = selected_connection_item->connection->offset.toInt(nullptr, 0);
    selected_connection_item->initialOffset = offset;
    int x = 0, y = 0;
    if (selected_connection_item->connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (selected_connection_item->connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (selected_connection_item->connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (selected_connection_item->connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    selected_connection_item->basePixmap = pixmap;
    QPainter painter(&pixmap);
    painter.setPen(QColor(255, 0, 255));
    painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    painter.end();
    selected_connection_item->setPixmap(pixmap);
    selected_connection_item->initialX = x;
    selected_connection_item->initialY = y;
    selected_connection_item->blockSignals(true);
    selected_connection_item->setX(x);
    selected_connection_item->setY(y);
    selected_connection_item->setZValue(-1);
    selected_connection_item->blockSignals(false);

    setConnectionEditControlValues(selected_connection_item->connection);
}

void Editor::updateCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;

    QString originalDirection = selected_connection_item->connection->direction;
    setCurrentConnectionDirection(curDirection);
    updateMirroredConnectionDirection(selected_connection_item->connection, originalDirection);
}

void Editor::onConnectionMoved(Connection* connection) {
    updateMirroredConnectionOffset(connection);
    onConnectionOffsetChanged(connection->offset.toInt());
}

void Editor::onConnectionOffsetChanged(int newOffset) {
    ui->spinBox_ConnectionOffset->blockSignals(true);
    ui->spinBox_ConnectionOffset->setValue(newOffset);
    ui->spinBox_ConnectionOffset->blockSignals(false);

}

void Editor::setConnectionEditControlValues(Connection* connection) {
    QString mapName = connection ? connection->map_name : "";
    QString direction = connection ? connection->direction : "";
    int offset = connection ? connection->offset.toInt() : 0;

    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->spinBox_ConnectionOffset->blockSignals(true);

    ui->comboBox_ConnectedMap->setCurrentText(mapName);
    ui->comboBox_ConnectionDirection->setCurrentText(direction);
    ui->spinBox_ConnectionOffset->setValue(offset);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_ConnectionDirection->blockSignals(false);
    ui->spinBox_ConnectionOffset->blockSignals(false);
}

void Editor::setConnectionEditControlsEnabled(bool enabled) {
    ui->comboBox_ConnectionDirection->setEnabled(enabled);
    ui->comboBox_ConnectedMap->setEnabled(enabled);
    ui->spinBox_ConnectionOffset->setEnabled(enabled);

    if (!enabled) {
        setConnectionEditControlValues(false);
    }
}

void Editor::onConnectionItemSelected(ConnectionPixmapItem* connectionItem) {
    if (!connectionItem)
        return;

    for (ConnectionPixmapItem* item : connection_edit_items) {
        bool isSelectedItem = item == connectionItem;
        int zValue = isSelectedItem ? 0 : -1;
        qreal opacity = isSelectedItem ? 1 : 0.75;
        item->setZValue(zValue);
        item->render(opacity);
        if (isSelectedItem) {
            QPixmap pixmap = item->pixmap();
            QPainter painter(&pixmap);
            painter.setPen(QColor(255, 0, 255));
            painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
            painter.end();
            item->setPixmap(pixmap);
        }
    }
    selected_connection_item = connectionItem;
    setConnectionEditControlsEnabled(true);
    setConnectionEditControlValues(selected_connection_item->connection);
    ui->spinBox_ConnectionOffset->setMaximum(selected_connection_item->getMaxOffset());
    ui->spinBox_ConnectionOffset->setMinimum(selected_connection_item->getMinOffset());
    onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
}

void Editor::setSelectedConnectionFromMap(QString mapName) {
    // Search for the first connection that connects to the given map map.
    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->connection->map_name == mapName) {
            onConnectionItemSelected(item);
            break;
        }
    }
}

void Editor::onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem) {
    emit loadMapRequested(connectionItem->connection->map_name, map->name);
}

void Editor::onConnectionDirectionChanged(QString newDirection) {
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->comboBox_ConnectionDirection->setCurrentText(newDirection);
    ui->comboBox_ConnectionDirection->blockSignals(false);
}

void Editor::onBorderMetatilesChanged() {
    displayMapBorder();
}

void Editor::setConnectionsVisibility(bool visible) {
    for (QGraphicsPixmapItem* item : connection_items) {
        item->setVisible(visible);
        item->setActive(visible);
    }
}

void Editor::setMap(QString map_name) {
    if (map_name.isNull()) {
        return;
    }
    if (project) {
        map = project->loadMap(map_name);
        displayMap();
        selected_events->clear();
        updateSelectedEvents();
    }
}

void Editor::mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item) {
    if (map_edit_mode == "paint") {
        item->paint(event);
    } else if (map_edit_mode == "fill") {
        item->floodFill(event);
    } else if (map_edit_mode == "pick") {
        item->pick(event);
    } else if (map_edit_mode == "select") {
        item->select(event);
    }
}
void Editor::mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item) {
    if (map_edit_mode == "paint") {
        item->paint(event);
    } else if (map_edit_mode == "fill") {
        item->floodFill(event);
    } else if (map_edit_mode == "pick") {
        item->pick(event);
    } else if (map_edit_mode == "select") {
        item->select(event);
    }
}

void Editor::displayMap() {
    if (!scene)
        scene = new QGraphicsScene;

    if (map_item && scene) {
        scene->removeItem(map_item);
        delete map_item;
    }
    map_item = new MapPixmapItem(map);
    connect(map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(mouseEvent_map(QGraphicsSceneMouseEvent*,MapPixmapItem*)));

    map_item->draw(true);
    scene->addItem(map_item);

    if (collision_item && scene) {
        scene->removeItem(collision_item);
        delete collision_item;
    }
    collision_item = new CollisionPixmapItem(map);
    connect(collision_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)),
            this, SLOT(mouseEvent_collision(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)));

    collision_item->draw(true);
    scene->addItem(collision_item);

    int tw = 16;
    int th = 16;
    scene->setSceneRect(
        -6 * tw,
        -6 * th,
        map_item->pixmap().width() + 12 * tw,
        map_item->pixmap().height() + 12 * th
    );

    displayMetatiles();
    displayBorderMetatiles();
    displayCollisionMetatiles();
    displayElevationMetatiles();
    displayMapEvents();
    displayMapConnections();
    displayMapBorder();
    displayMapGrid();

    if (map_item) {
        map_item->setVisible(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
}

void Editor::displayMetatiles() {
    if (metatiles_item && metatiles_item->scene()) {
        metatiles_item->scene()->removeItem(metatiles_item);
        delete metatiles_item;
    }

    scene_metatiles = new QGraphicsScene;
    metatiles_item = new MetatilesPixmapItem(map);
    metatiles_item->draw();
    scene_metatiles->addItem(metatiles_item);
}

void Editor::displayBorderMetatiles() {
    if (selected_border_metatiles_item && selected_border_metatiles_item->scene()) {
        selected_border_metatiles_item->scene()->removeItem(selected_border_metatiles_item);
        delete selected_border_metatiles_item;
    }

    scene_selected_border_metatiles = new QGraphicsScene;
    selected_border_metatiles_item = new BorderMetatilesPixmapItem(map);
    selected_border_metatiles_item->draw();
    scene_selected_border_metatiles->addItem(selected_border_metatiles_item);

    connect(selected_border_metatiles_item, SIGNAL(borderMetatilesChanged()), this, SLOT(onBorderMetatilesChanged()));
}

void Editor::displayCollisionMetatiles() {
    if (collision_metatiles_item && collision_metatiles_item->scene()) {
        collision_metatiles_item->scene()->removeItem(collision_metatiles_item);
        delete collision_metatiles_item;
    }

    scene_collision_metatiles = new QGraphicsScene;
    collision_metatiles_item = new CollisionMetatilesPixmapItem(map);
    collision_metatiles_item->draw();
    scene_collision_metatiles->addItem(collision_metatiles_item);
}

void Editor::displayElevationMetatiles() {
    if (elevation_metatiles_item && elevation_metatiles_item->scene()) {
        elevation_metatiles_item->scene()->removeItem(elevation_metatiles_item);
        delete elevation_metatiles_item;
    }

    scene_elevation_metatiles = new QGraphicsScene;
    elevation_metatiles_item = new ElevationMetatilesPixmapItem(map);
    elevation_metatiles_item->draw();
    scene_elevation_metatiles->addItem(elevation_metatiles_item);
}

void Editor::displayMapEvents() {
    if (events_group) {
        for (QGraphicsItem *child : events_group->childItems()) {
            events_group->removeFromGroup(child);
            delete child;
        }

        if (events_group->scene()) {
            events_group->scene()->removeItem(events_group);
        }

        delete events_group;
    }

    events_group = new EventGroup;
    scene->addItem(events_group);

    QList<Event *> events = map->getAllEvents();
    project->loadEventPixmaps(events);
    for (Event *event : events) {
        addMapEvent(event);
    }
    //objects_group->setFiltersChildEvents(false);
    events_group->setHandlesChildEvents(false);

    emit objectsChanged();
}

DraggablePixmapItem *Editor::addMapEvent(Event *event) {
    DraggablePixmapItem *object = new DraggablePixmapItem(event);
    object->editor = this;
    events_group->addToGroup(object);
    return object;
}

void Editor::displayMapConnections() {
    for (QGraphicsPixmapItem* item : connection_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    connection_items.clear();

    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    selected_connection_item = NULL;
    connection_edit_items.clear();

    for (Connection *connection : map->connections) {
        if (connection->direction == "dive" || connection->direction == "emerge") {
            continue;
        }
        createConnectionItem(connection, false);
    }

    if (!connection_edit_items.empty()) {
        onConnectionItemSelected(connection_edit_items.first());
    }
}

void Editor::createConnectionItem(Connection* connection, bool hide) {
    Map *connected_map = project->getMap(connection->map_name);
    QPixmap pixmap = connected_map->renderConnection(*connection);
    int offset = connection->offset.toInt(nullptr, 0);
    int x = 0, y = 0;
    if (connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    item->setZValue(-1);
    item->setX(x);
    item->setY(y);
    scene->addItem(item);
    connection_items.append(item);
    item->setVisible(!hide);

    ConnectionPixmapItem *connection_edit_item = new ConnectionPixmapItem(pixmap, connection, x, y, map->getWidth(), map->getHeight());
    connection_edit_item->setX(x);
    connection_edit_item->setY(y);
    connection_edit_item->setZValue(-1);
    scene->addItem(connection_edit_item);
    connect(connection_edit_item, SIGNAL(connectionMoved(Connection*)), this, SLOT(onConnectionMoved(Connection*)));
    connect(connection_edit_item, SIGNAL(connectionItemSelected(ConnectionPixmapItem*)), this, SLOT(onConnectionItemSelected(ConnectionPixmapItem*)));
    connect(connection_edit_item, SIGNAL(connectionItemDoubleClicked(ConnectionPixmapItem*)), this, SLOT(onConnectionItemDoubleClicked(ConnectionPixmapItem*)));
    connection_edit_items.append(connection_edit_item);
}

void Editor::displayMapBorder() {
    for (QGraphicsPixmapItem* item : borderItems) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    borderItems.clear();

    QPixmap pixmap = map->renderBorder();
    for (int y = -6; y < map->getHeight() + 6; y += 2)
    for (int x = -6; x < map->getWidth() + 6; x += 2) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
        item->setX(x * 16);
        item->setY(y * 16);
        item->setZValue(-2);
        scene->addItem(item);
        borderItems.append(item);
    }
}

void Editor::displayMapGrid() {
    for (QGraphicsLineItem* item : gridLines) {
        if (item && item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    gridLines.clear();
    ui->checkBox_ToggleGrid->disconnect();

    int pixelWidth = map->getWidth() * 16;
    int pixelHeight = map->getHeight() * 16;
    for (int i = 0; i <= map->getWidth(); i++) {
        int x = i * 16;
        QGraphicsLineItem *line = scene->addLine(x, 0, x, pixelHeight);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
    for (int j = 0; j <= map->getHeight(); j++) {
        int y = j * 16;
        QGraphicsLineItem *line = scene->addLine(0, y, pixelWidth, y);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
}

void Editor::updateConnectionOffset(int offset) {
    if (!selected_connection_item)
        return;

    selected_connection_item->blockSignals(true);
    offset = qMin(offset, selected_connection_item->getMaxOffset());
    offset = qMax(offset, selected_connection_item->getMinOffset());
    selected_connection_item->connection->offset = QString::number(offset);
    if (selected_connection_item->connection->direction == "up" || selected_connection_item->connection->direction == "down") {
        selected_connection_item->setX(selected_connection_item->initialX + (offset - selected_connection_item->initialOffset) * 16);
    } else if (selected_connection_item->connection->direction == "left" || selected_connection_item->connection->direction == "right") {
        selected_connection_item->setY(selected_connection_item->initialY + (offset - selected_connection_item->initialOffset) * 16);
    }
    selected_connection_item->blockSignals(false);
    updateMirroredConnectionOffset(selected_connection_item->connection);
}

void Editor::setConnectionMap(QString mapName) {
    if (!mapName.isEmpty() && !project->mapNames->contains(mapName)) {
        qDebug() << "Invalid map name " << mapName << " specified for connection.";
        return;
    }
    if (!selected_connection_item)
        return;

    if (mapName.isEmpty()) {
        removeCurrentConnection();
        return;
    }

    QString originalMapName = selected_connection_item->connection->map_name;
    setConnectionEditControlsEnabled(true);
    selected_connection_item->connection->map_name = mapName;
    setCurrentConnectionDirection(selected_connection_item->connection->direction);
    updateMirroredConnectionMap(selected_connection_item->connection, originalMapName);
}

void Editor::addNewConnection() {
    // Find direction with least number of connections.
    QMap<QString, int> directionCounts = QMap<QString, int>({{"up", 0}, {"right", 0}, {"down", 0}, {"left", 0}});
    for (Connection* connection : map->connections) {
        directionCounts[connection->direction]++;
    }
    QString minDirection = "up";
    int minCount = INT_MAX;
    for (QString direction : directionCounts.keys()) {
        if (directionCounts[direction] < minCount) {
            minDirection = direction;
            minCount = directionCounts[direction];
        }
    }

    // Don't connect the map to itself.
    QString defaultMapName = project->mapNames->first();
    if (defaultMapName == map->name) {
        defaultMapName = project->mapNames->value(1);
    }

    Connection* newConnection = new Connection;
    newConnection->direction = minDirection;
    newConnection->offset = "0";
    newConnection->map_name = defaultMapName;
    map->connections.append(newConnection);
    createConnectionItem(newConnection, true);
    onConnectionItemSelected(connection_edit_items.last());
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    updateMirroredConnection(newConnection, newConnection->direction, newConnection->map_name);
}

void Editor::updateMirroredConnectionOffset(Connection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name);
}
void Editor::updateMirroredConnectionDirection(Connection* connection, QString originalDirection) {
    updateMirroredConnection(connection, originalDirection, connection->map_name);
}
void Editor::updateMirroredConnectionMap(Connection* connection, QString originalMapName) {
    updateMirroredConnection(connection, connection->direction, originalMapName);
}
void Editor::removeMirroredConnection(Connection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name, true);
}
void Editor::updateMirroredConnection(Connection* connection, QString originalDirection, QString originalMapName, bool isDelete) {
    if (!ui->checkBox_MirrorConnections->isChecked())
        return;

    static QMap<QString, QString> oppositeDirections = QMap<QString, QString>({
        {"up", "down"}, {"right", "left"},
        {"down", "up"}, {"left", "right"},
        {"dive", "emerge"},{"emerge", "dive"}});
    QString oppositeDirection = oppositeDirections.value(originalDirection);

    // Find the matching connection in the connected map.
    QMap<QString, Map*> *mapcache = project->map_cache;
    Connection* mirrorConnection = NULL;
    Map* otherMap = project->getMap(originalMapName);
    for (Connection* conn : otherMap->connections) {
        if (conn->direction == oppositeDirection && conn->map_name == map->name) {
            mirrorConnection = conn;
        }
    }

    if (isDelete) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
        }
        return;
    }

    if (connection->direction != originalDirection || connection->map_name != originalMapName) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
            mirrorConnection = NULL;
            otherMap = project->getMap(connection->map_name);
        }
    }

    // Create a new mirrored connection, if a matching one doesn't already exist.
    if (!mirrorConnection) {
        mirrorConnection = new Connection;
        mirrorConnection->direction = oppositeDirections.value(connection->direction);
        mirrorConnection->map_name = map->name;
        otherMap->connections.append(mirrorConnection);
    }

    mirrorConnection->offset = QString::number(-connection->offset.toInt());
}

void Editor::removeCurrentConnection() {
    if (!selected_connection_item)
        return;

    map->connections.removeOne(selected_connection_item->connection);
    connection_edit_items.removeOne(selected_connection_item);
    removeMirroredConnection(selected_connection_item->connection);

    if (selected_connection_item && selected_connection_item->scene()) {
        selected_connection_item->scene()->removeItem(selected_connection_item);
        delete selected_connection_item;
    }

    selected_connection_item = NULL;
    setConnectionEditControlsEnabled(false);
    ui->spinBox_ConnectionOffset->setValue(0);
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    if (connection_edit_items.length() > 0) {
        onConnectionItemSelected(connection_edit_items.last());
    }
}

void Editor::updateDiveMap(QString mapName) {
    updateDiveEmergeMap(mapName, "dive");
}

void Editor::updateEmergeMap(QString mapName) {
    updateDiveEmergeMap(mapName, "emerge");
}

void Editor::updateDiveEmergeMap(QString mapName, QString direction) {
    if (!mapName.isEmpty() && !project->mapNamesToMapConstants->contains(mapName)) {
        qDebug() << "Invalid " << direction << " map connection: " << mapName;
        return;
    }

    Connection* connection = NULL;
    for (Connection* conn : map->connections) {
        if (conn->direction == direction) {
            connection = conn;
            break;
        }
    }

    if (mapName.isEmpty()) {
        // Remove dive/emerge connection
        if (connection) {
            map->connections.removeOne(connection);
            removeMirroredConnection(connection);
        }
    } else {
        if (!connection) {
            connection = new Connection;
            connection->direction = direction;
            connection->offset = "0";
            connection->map_name = mapName;
            map->connections.append(connection);
            updateMirroredConnection(connection, connection->direction, connection->map_name);
        } else {
            QString originalMapName = connection->map_name;
            connection->map_name = mapName;
            updateMirroredConnectionMap(connection, originalMapName);
        }
    }

    ui->label_NumConnections->setText(QString::number(map->connections.length()));
}

void Editor::updatePrimaryTileset(QString tilesetLabel)
{
    map->layout->tileset_primary_label = tilesetLabel;
    map->layout->tileset_primary = project->getTileset(tilesetLabel);
    emit tilesetChanged(map->name);
}

void Editor::updateSecondaryTileset(QString tilesetLabel)
{
    map->layout->tileset_secondary_label = tilesetLabel;
    map->layout->tileset_secondary = project->getTileset(tilesetLabel);
    emit tilesetChanged(map->name);
}

void MetatilesPixmapItem::paintTileChanged(Map *map) {
    draw();
}

void MetatilesPixmapItem::draw() {
    setPixmap(map->renderMetatiles());
}

void MetatilesPixmapItem::updateCurHoveredMetatile(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if (x < 0 || x >= width || y < 0 || y >= height) {
        map->clearHoveredMetatile();
    } else {
        int block = y * width + x;
        map->hoveredMetatileChanged(block);
    }
}

void MetatilesPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    updateCurHoveredMetatile(event->pos());
}
void MetatilesPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    map->clearHoveredMetatile();
}
void MetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    map->paint_metatile_initial_x = x;
    map->paint_metatile_initial_y = y;
    updateSelection(event->pos(), event->button());
}
void MetatilesPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    updateCurHoveredMetatile(event->pos());
    Qt::MouseButton button = event->button();
    if (button == Qt::MouseButton::NoButton) {
        Qt::MouseButtons heldButtons = event->buttons();
        if (heldButtons & Qt::RightButton) {
            button = Qt::RightButton;
        } else if (heldButtons & Qt::LeftButton) {
            button = Qt::LeftButton;
        }
    }
    updateSelection(event->pos(), button);
}
void MetatilesPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    updateSelection(event->pos(), event->button());
}
void MetatilesPixmapItem::updateSelection(QPointF pos, Qt::MouseButton button) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if ((x >= 0 && x < width) && (y >=0 && y < height)) {
        int baseTileX = x < map->paint_metatile_initial_x ? x : map->paint_metatile_initial_x;
        int baseTileY = y < map->paint_metatile_initial_y ? y : map->paint_metatile_initial_y;
        map->paint_tile_index = baseTileY * 8 + baseTileX;
        map->paint_tile_width = abs(map->paint_metatile_initial_x - x) + 1;
        map->paint_tile_height = abs(map->paint_metatile_initial_y - y) + 1;
        map->smart_paths_enabled = button == Qt::RightButton
                                && map->paint_tile_width == 3
                                && map->paint_tile_height == 3;
        emit map->paintTileChanged(map);
    }
}

void BorderMetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;

    for (int i = 0; i < map->paint_tile_width && (i + x) < 2; i++) {
        for (int j = 0; j < map->paint_tile_height && (j + y) < 2; j++) {
            int blockIndex = (j + y) * 2 + (i + x);
            int tile = map->getSelectedBlockIndex(map->paint_tile_index + i + (j * 8));
            (*map->layout->border->blocks)[blockIndex].tile = tile;
        }
    }

    draw();
    emit borderMetatilesChanged();
}

void BorderMetatilesPixmapItem::draw() {
    QImage image(32, 32, QImage::Format_RGBA8888);
    QPainter painter(&image);
    QList<Block> *blocks = map->layout->border->blocks;

    for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
    {
        int x = i * 16;
        int y = j * 16;
        int index = j * 2 + i;
        QImage metatile_image = Metatile::getMetatileImage(blocks->value(index).tile, map->layout->tileset_primary, map->layout->tileset_secondary);
        QPoint metatile_origin = QPoint(x, y);
        painter.drawImage(metatile_origin, metatile_image);
    }

    painter.end();
    setPixmap(QPixmap::fromImage(image));
}

void MovementPermissionsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QPointF pos = event->pos();
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if ((x >= 0 && x < width) && (y >=0 && y < height)) {
        pick(y * width + x);
    }
}
void MovementPermissionsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    updateCurHoveredMetatile(event->pos());
    mousePressEvent(event);
}
void MovementPermissionsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    mousePressEvent(event);
}

void CollisionMetatilesPixmapItem::updateCurHoveredMetatile(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if (x < 0 || x >= width || y < 0 || y >= height) {
        map->clearHoveredCollisionTile();
    } else {
        int collision = y * width + x;
        map->hoveredCollisionTileChanged(collision);
    }
}

int ConnectionPixmapItem::getMinOffset() {
    if (connection->direction == "up" || connection->direction == "down")
        return 1 - (this->pixmap().width() / 16);
    else
        return 1 - (this->pixmap().height() / 16);
}
int ConnectionPixmapItem::getMaxOffset() {
    if (connection->direction == "up" || connection->direction == "down")
        return baseMapWidth - 1;
    else
        return baseMapHeight - 1;
}
QVariant ConnectionPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        QPointF newPos = value.toPointF();

        qreal x, y;
        int newOffset = initialOffset;
        if (connection->direction == "up" || connection->direction == "down") {
            x = round(newPos.x() / 16) * 16;
            newOffset += (x - initialX) / 16;
            newOffset = qMin(newOffset, this->getMaxOffset());
            newOffset = qMax(newOffset, this->getMinOffset());
            x = newOffset * 16;
        }
        else {
            x = initialX;
        }

        if (connection->direction == "right" || connection->direction == "left") {
            y = round(newPos.y() / 16) * 16;
            newOffset += (y - initialY) / 16;
            newOffset = qMin(newOffset, this->getMaxOffset());
            newOffset = qMax(newOffset, this->getMinOffset());
            y = newOffset * 16;
        }
        else {
            y = initialY;
        }

        connection->offset = QString::number(newOffset);
        emit connectionMoved(connection);
        return QPointF(x, y);
    }
    else {
        return QGraphicsItem::itemChange(change, value);
    }
}
void ConnectionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    emit connectionItemSelected(this);
}
void ConnectionPixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) {
    emit connectionItemDoubleClicked(this);
}

void ElevationMetatilesPixmapItem::updateCurHoveredMetatile(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if (x < 0 || x >= width || y < 0 || y >= height) {
        map->clearHoveredElevationTile();
    } else {
        int elevation = y * width + x;
        map->hoveredElevationTileChanged(elevation);
    }
}

void MapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;

        if (map->smart_paths_enabled) {
            paintSmartPath(x, y);
        } else {
            paintNormal(x, y);
        }

        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            map->commit();
        }
        draw();
    }
}

void MapPixmapItem::paintNormal(int x, int y) {
    // Snap the selected position to the top-left of the block boundary.
    // This allows painting via dragging the mouse to tile the painted region.
    int xDiff = x - map->paint_tile_initial_x;
    int yDiff = y - map->paint_tile_initial_y;
    if (xDiff < 0 && xDiff % map->paint_tile_width != 0) xDiff -= map->paint_tile_width;
    if (yDiff < 0 && yDiff % map->paint_tile_height != 0) yDiff -= map->paint_tile_height;

    x = map->paint_tile_initial_x + (xDiff / map->paint_tile_width) * map->paint_tile_width;
    y = map->paint_tile_initial_y + (yDiff / map->paint_tile_height) * map->paint_tile_height;
    for (int i = 0; i < map->paint_tile_width && i + x < map->getWidth(); i++)
    for (int j = 0; j < map->paint_tile_height && j + y < map->getHeight(); j++) {
        int actualX = i + x;
        int actualY = j + y;
        Block *block = map->getBlock(actualX, actualY);
        if (block) {
            block->tile = map->getSelectedBlockIndex(map->paint_tile_index + i + (j * 8));
            map->_setBlock(actualX, actualY, *block);
        }
    }
}

// These are tile offsets from the top-left tile in the 3x3 smart path selection.
// Each entry is for one possibility from the marching squares value for a tile.
// (Marching Squares: https://en.wikipedia.org/wiki/Marching_squares)
QList<int> MapPixmapItem::smartPathTable = QList<int>({
    8 + 1, // 0000
    8 + 1, // 0001
    8 + 1, // 0010
   16 + 0, // 0011
    8 + 1, // 0100
    8 + 1, // 0101
    0 + 0, // 0110
    8 + 0, // 0111
    8 + 1, // 1000
   16 + 2, // 1001
    8 + 1, // 1010
   16 + 1, // 1011
    0 + 2, // 1100
    8 + 2, // 1101
    0 + 1, // 1110
    8 + 1, // 1111
});

#define IS_SMART_PATH_TILE(block) ((map->getDisplayedBlockIndex(block->tile) >= map->paint_tile_index && map->getDisplayedBlockIndex(block->tile) < map->paint_tile_index + 3) \
                                || (map->getDisplayedBlockIndex(block->tile) >= map->paint_tile_index + 8 && map->getDisplayedBlockIndex(block->tile) < map->paint_tile_index + 11) \
                                || (map->getDisplayedBlockIndex(block->tile) >= map->paint_tile_index + 16 && map->getDisplayedBlockIndex(block->tile) < map->paint_tile_index + 19))

void MapPixmapItem::paintSmartPath(int x, int y) {
    // Smart path should never be enabled without a 3x3 block selection.
    if (map->paint_tile_width != 3 || map->paint_tile_height != 3) return;

    // Shift to the middle tile of the smart path selection.
    int openTile = map->paint_tile_index + 8 + 1;

    // Fill the region with the open tile.
    for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++) {
        // Check if in map bounds.
        if (!(i + x < map->getWidth() && i + x >= 0 && j + y < map->getHeight() && j + y >= 0))
            continue;
        int actualX = i + x;
        int actualY = j + y;
        Block *block = map->getBlock(actualX, actualY);
        if (block) {
            block->tile = map->getSelectedBlockIndex(openTile);
            map->_setBlock(actualX, actualY, *block);
        }
    }

    // Go back and resolve the edge tiles
    for (int i = -2; i <= 2; i++)
    for (int j = -2; j <= 2; j++) {
        // Check if in map bounds.
        if (!(i + x < map->getWidth() && i + x >= 0 && j + y < map->getHeight() && j + y >= 0))
            continue;
        // Ignore the corners, which can't possible be affected by the smart path.
        if ((i == -2 && j == -2) || (i == 2 && j == -2) ||
            (i == -2 && j ==  2) || (i == 2 && j ==  2))
            continue;

        // Ignore tiles that aren't part of the smart path set.
        int actualX = i + x;
        int actualY = j + y;
        Block *block = map->getBlock(actualX, actualY);
        if (!block || !IS_SMART_PATH_TILE(block)) {
            continue;
        }

        int id = 0;
        Block *top = map->getBlock(actualX, actualY - 1);
        Block *right = map->getBlock(actualX + 1, actualY);
        Block *bottom = map->getBlock(actualX, actualY + 1);
        Block *left = map->getBlock(actualX - 1, actualY);

        // Get marching squares value, to determine which tile to use.
        if (top && IS_SMART_PATH_TILE(top))
            id += 1;
        if (right && IS_SMART_PATH_TILE(right))
            id += 2;
        if (bottom && IS_SMART_PATH_TILE(bottom))
            id += 4;
        if (left && IS_SMART_PATH_TILE(left))
            id += 8;

        block->tile = map->getSelectedBlockIndex(map->paint_tile_index + smartPathTable[id]);
        map->_setBlock(actualX, actualY, *block);
    }
}

void MapPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;
        map->floodFill(x, y, map->getSelectedBlockIndex(map->paint_tile_index));
        draw();
    }
}

void MapPixmapItem::pick(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = (int)(pos.x()) / 16;
    int y = (int)(pos.y()) / 16;
    Block *block = map->getBlock(x, y);
    if (block) {
        map->paint_tile_index = map->getDisplayedBlockIndex(block->tile);
        map->paint_tile_width = 1;
        map->paint_tile_height = 1;
        emit map->paintTileChanged(map);
    }
}

#define SWAP(a, b) do { if (a != b) { a ^= b; b ^= a; a ^= b; } } while (0)

void MapPixmapItem::select(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = (int)(pos.x()) / 16;
    int y = (int)(pos.y()) / 16;
    if (event->type() == QEvent::GraphicsSceneMousePress) {
        selection_origin = QPoint(x, y);
        selection.clear();
    } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
        if (event->buttons() & Qt::LeftButton) {
            selection.clear();
            selection.append(QPoint(x, y));
        }
    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        if (!selection.isEmpty()) {
            QPoint pos = selection.last();
            int x1 = selection_origin.x();
            int y1 = selection_origin.y();
            int x2 = pos.x();
            int y2 = pos.y();
            if (x1 > x2) SWAP(x1, x2);
            if (y1 > y2) SWAP(y1, y2);
            selection.clear();
            for (int y = y1; y <= y2; y++) {
                for (int x = x1; x <= x2; x++) {
                    selection.append(QPoint(x, y));
                }
            }
            qDebug() << QString("selected (%1, %2) -> (%3, %4)").arg(x1).arg(y1).arg(x2).arg(y2);
        }
    }
}

void MapPixmapItem::draw(bool ignoreCache) {
    if (map) {
        setPixmap(map->render(ignoreCache));
    }
}

void MapPixmapItem::updateCurHoveredTile(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int blockIndex = y * map->getWidth() + x;
    if (x < 0 || x >= map->getWidth() || y < 0 || y >= map->getHeight()) {
        map->clearHoveredTile();
    } else {
        int tile = map->layout->blockdata->blocks->at(blockIndex).tile;
        map->hoveredTileChanged(x, y, tile);
    }
}

void MapPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    updateCurHoveredTile(event->pos());
}
void MapPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    map->clearHoveredTile();
}
void MapPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    map->paint_tile_initial_x = x;
    map->paint_tile_initial_y = y;
    emit mouseEvent(event, this);
}
void MapPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    updateCurHoveredTile(event->pos());
    emit mouseEvent(event, this);
}
void MapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
void CollisionPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}
void CollisionPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void CollisionPixmapItem::draw(bool ignoreCache) {
    if (map) {
        setPixmap(map->renderCollision(ignoreCache));
    }
}

void CollisionPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;
        Block *block = map->getBlock(x, y);
        if (block) {
            if (map->paint_collision >= 0) {
                block->collision = map->paint_collision;
            }
            if (map->paint_elevation >= 0) {
                block->elevation = map->paint_elevation;
            }
            map->_setBlock(x, y, *block);
        }
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            map->commit();
        }
        draw();
    }
}

void CollisionPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;
        bool collision = map->paint_collision >= 0;
        bool elevation = map->paint_elevation >= 0;
        if (collision && elevation) {
            map->floodFillCollisionElevation(x, y, map->paint_collision, map->paint_elevation);
        } else if (collision) {
            map->floodFillCollision(x, y, map->paint_collision);
        } else if (elevation) {
            map->floodFillElevation(x, y, map->paint_elevation);
        }
        draw();
    }
}

void CollisionPixmapItem::pick(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = (int)(pos.x()) / 16;
    int y = (int)(pos.y()) / 16;
    Block *block = map->getBlock(x, y);
    if (block) {
        map->paint_collision = block->collision;
        map->paint_elevation = block->elevation;
        emit map->paintCollisionChanged(map);
    }
}

void DraggablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouse) {
    active = true;
    clicking = true;
    last_x = (mouse->pos().x() + this->pos().x()) / 16;
    last_y = (mouse->pos().y() + this->pos().y()) / 16;
    //qDebug() << QString("(%1, %2)").arg(event->get("x")).arg(event->get("y"));
}

void DraggablePixmapItem::move(int x, int y) {
    event->setX(event->x() + x);
    event->setY(event->y() + y);
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        int x = (mouse->pos().x() + this->pos().x()) / 16;
        int y = (mouse->pos().y() + this->pos().y()) / 16;
        if (x != last_x || y != last_y) {
            clicking = false;
            if (editor->selected_events->contains(this)) {
                for (DraggablePixmapItem *item : *editor->selected_events) {
                    item->move(x - last_x, y - last_y);
                }
            } else {
                move(x - last_x, y - last_y);
            }
            last_x = x;
            last_y = y;
            //qDebug() << QString("(%1, %2)").arg(event->get("x")).arg(event->get("x"));
        }
    }
}

void DraggablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouse) {
    if (clicking) {
        this->editor->selectMapEvent(this, mouse->modifiers() & Qt::ControlModifier);
        this->editor->updateSelectedEvents();
    }
    active = false;
}

QList<DraggablePixmapItem *> *Editor::getObjects() {
    QList<DraggablePixmapItem *> *list = new QList<DraggablePixmapItem *>;
    for (Event *event : map->getAllEvents()) {
        for (QGraphicsItem *child : events_group->childItems()) {
            DraggablePixmapItem *item = (DraggablePixmapItem *)child;
            if (item->event == event) {
                list->append(item);
                break;
            }
        }
    }
    return list;
}

void Editor::redrawObject(DraggablePixmapItem *item) {
    if (item) {
        item->setPixmap(item->event->pixmap);
        if (selected_events && selected_events->contains(item)) {
            QImage image = item->pixmap().toImage();
            QPainter painter(&image);
            painter.setPen(QColor(250, 100, 25));
            painter.drawRect(0, 0, image.width() - 1, image.height() - 1);
            painter.end();
            item->setPixmap(QPixmap::fromImage(image));
        }
    }
}

void Editor::updateSelectedEvents() {
    for (DraggablePixmapItem *item : *(getObjects())) {
        redrawObject(item);
    }
    emit selectedObjectsChanged();
}

void Editor::selectMapEvent(DraggablePixmapItem *object) {
    selectMapEvent(object, false);
}

void Editor::selectMapEvent(DraggablePixmapItem *object, bool toggle) {
    if (selected_events && object) {
        if (selected_events->contains(object)) {
            if (toggle) {
                selected_events->removeOne(object);
            }
        } else {
            if (!toggle) {
                selected_events->clear();
            }
            selected_events->append(object);
        }
        updateSelectedEvents();
    }
}

DraggablePixmapItem* Editor::addNewEvent(QString event_type) {
    if (project && map) {
        Event *event = Event::createNewEvent(event_type, map->name);
        event->put("map_name", map->name);
        map->addEvent(event);
        project->loadEventPixmaps(map->getAllEvents());
        DraggablePixmapItem *object = addMapEvent(event);

        return object;
    }
    return NULL;
}

void Editor::deleteEvent(Event *event) {
    Map *map = project->getMap(event->get("map_name"));
    if (map) {
        map->removeEvent(event);
    }
    //selected_events->removeAll(event);
    //updateSelectedObjects();
}

// dunno how to detect bubbling. QMouseEvent::isAccepted seems to always be true
// check if selected_events changed instead. this has the side effect of deselecting
// when you click on a selected event, since selected_events doesn't change.

QList<DraggablePixmapItem *> selected_events_test;
bool clicking = false;

void Editor::objectsView_onMousePress(QMouseEvent *event) {
    clicking = true;
    selected_events_test = *selected_events;
}

void Editor::objectsView_onMouseMove(QMouseEvent *event) {
    clicking = false;
}

void Editor::objectsView_onMouseRelease(QMouseEvent *event) {
    if (clicking) {
        if (selected_events_test.length()) {
            if (selected_events_test.length() == selected_events->length()) {
                bool deselect = true;
                for (int i = 0; i < selected_events_test.length(); i++) {
                    if (selected_events_test.at(i) != selected_events->at(i)) {
                        deselect = false;
                        break;
                    }
                }
                if (deselect) {
                    selected_events->clear();
                    updateSelectedEvents();
                }
            }
        }
        clicking = false;
    }
}
