#include "editor.h"
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>

Editor::Editor()
{
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
        ((MapPixmapItem*)current_view)->undo();
    }
}

void Editor::redo() {
    if (current_view) {
        ((MapPixmapItem*)current_view)->redo();
    }
}

void Editor::setEditingMap() {
    current_view = map_item;
    if (map_item) {
        map_item->draw();
        map_item->setVisible(true);
        map_item->setEnabled(true);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (objects_group) {
        objects_group->setVisible(false);
    }
}

void Editor::setEditingCollision() {
    current_view = collision_item;
    if (collision_item) {
        collision_item->draw();
        collision_item->setVisible(true);
    }
    if (map_item) {
        map_item->setVisible(false);
    }
    if (objects_group) {
        objects_group->setVisible(false);
    }
}

void Editor::setEditingObjects() {
    current_view = map_item;
    if (objects_group) {
        objects_group->setVisible(true);
    }
    if (map_item) {
        map_item->setVisible(true);
        map_item->setEnabled(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
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
        updateSelectedObjects();
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
    scene = new QGraphicsScene;

    map_item = new MapPixmapItem(map);
    connect(map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(mouseEvent_map(QGraphicsSceneMouseEvent*,MapPixmapItem*)));

    map_item->draw();
    scene->addItem(map_item);

    collision_item = new CollisionPixmapItem(map);
    connect(collision_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)),
            this, SLOT(mouseEvent_collision(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)));

    collision_item->draw();
    scene->addItem(collision_item);

    objects_group = new EventGroup;
    scene->addItem(objects_group);

    if (map_item) {
        map_item->setVisible(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (objects_group) {
        objects_group->setVisible(false);
    }

    int tw = 16;
    int th = 16;
    scene->setSceneRect(
        -6 * tw,
        -6 * th,
        map_item->pixmap().width() + 12 * tw,
        map_item->pixmap().height() + 12 * th
    );

    displayMetatiles();
    displayCollisionMetatiles();
    displayElevationMetatiles();
    displayMapObjects();
    displayMapConnections();
    displayMapBorder();
    displayMapGrid();
}

void Editor::displayMetatiles() {
    scene_metatiles = new QGraphicsScene;
    metatiles_item = new MetatilesPixmapItem(map);
    metatiles_item->draw();
    scene_metatiles->addItem(metatiles_item);
}

void Editor::displayCollisionMetatiles() {
    scene_collision_metatiles = new QGraphicsScene;
    collision_metatiles_item = new CollisionMetatilesPixmapItem(map);
    collision_metatiles_item->draw();
    scene_collision_metatiles->addItem(collision_metatiles_item);
}

void Editor::displayElevationMetatiles() {
    scene_elevation_metatiles = new QGraphicsScene;
    elevation_metatiles_item = new ElevationMetatilesPixmapItem(map);
    elevation_metatiles_item->draw();
    scene_elevation_metatiles->addItem(elevation_metatiles_item);
}

void Editor::displayMapObjects() {
    for (QGraphicsItem *child : objects_group->childItems()) {
        objects_group->removeFromGroup(child);
    }

    QList<Event *> events = map->getAllEvents();
    project->loadObjectPixmaps(events);
    for (Event *event : events) {
        addMapObject(event);
    }
    //objects_group->setFiltersChildEvents(false);
    objects_group->setHandlesChildEvents(false);

    emit objectsChanged();
}

DraggablePixmapItem *Editor::addMapObject(Event *event) {
    DraggablePixmapItem *object = new DraggablePixmapItem(event);
    object->editor = this;
    objects_group->addToGroup(object);
    return object;
}

void Editor::displayMapConnections() {
    for (Connection *connection : map->connections) {
        if (connection->direction == "dive" || connection->direction == "emerge") {
            continue;
        }
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
    }
}

void Editor::displayMapBorder() {
    QPixmap pixmap = map->renderBorder();
    for (int y = -6; y < map->getHeight() + 6; y += 2)
    for (int x = -6; x < map->getWidth() + 6; x += 2) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
        item->setX(x * 16);
        item->setY(y * 16);
        item->setZValue(-2);
        scene->addItem(item);
    }
}

void Editor::displayMapGrid() {
    int pixelWidth = map->getWidth() * 16;
    int pixelHeight = map->getHeight() * 16;
    for (int i = 0; i <= map->getWidth(); i++) {
        int x = i * 16;
        QGraphicsLineItem *line = scene->addLine(x, 0, x, pixelHeight);
        line->setVisible(gridToggleCheckbox->isChecked());
        connect(gridToggleCheckbox, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
    for (int j = 0; j <= map->getHeight(); j++) {
        int y = j * 16;
        QGraphicsLineItem *line = scene->addLine(0, y, pixelWidth, y);
        line->setVisible(gridToggleCheckbox->isChecked());
        connect(gridToggleCheckbox, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
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
    updateSelection(event->pos());
}
void MetatilesPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    updateCurHoveredMetatile(event->pos());
    updateSelection(event->pos());
}
void MetatilesPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    updateSelection(event->pos());
}
void MetatilesPixmapItem::updateSelection(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if ((x >= 0 && x < width) && (y >=0 && y < height)) {
        int baseTileX = x < map->paint_metatile_initial_x ? x : map->paint_metatile_initial_x;
        int baseTileY = y < map->paint_metatile_initial_y ? y : map->paint_metatile_initial_y;
        map->paint_tile = baseTileY * 8 + baseTileX;
        map->paint_tile_width = abs(map->paint_metatile_initial_x - x) + 1;
        map->paint_tile_height = abs(map->paint_metatile_initial_y - y) + 1;
        emit map->paintTileChanged(map);
    }
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
                block->tile = map->paint_tile + i + (j * 8);
                map->_setBlock(actualX, actualY, *block);
            }
        }
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            map->commit();
        }
        draw();
    }
}

void MapPixmapItem::floodFill(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;
        map->floodFill(x, y, map->paint_tile);
        draw();
    }
}

void MapPixmapItem::pick(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = (int)(pos.x()) / 16;
    int y = (int)(pos.y()) / 16;
    Block *block = map->getBlock(x, y);
    if (block) {
        map->paint_tile = block->tile;
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

void MapPixmapItem::draw() {
    if (map) {
        setPixmap(map->render());
    }
}

void MapPixmapItem::undo() {
    if (map) {
        map->undo();
        draw();
    }
}

void MapPixmapItem::redo() {
    if (map) {
        map->redo();
        draw();
    }
}

void MapPixmapItem::updateCurHoveredTile(QPointF pos) {
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    int blockIndex = y * map->getWidth() + x;
    if (x < 0 || x >= map->getWidth() || y < 0 || y >= map->getHeight()) {
        map->clearHoveredTile();
    } else {
        int tile = map->blockdata->blocks->at(blockIndex).tile;
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

void CollisionPixmapItem::draw() {
    if (map) {
        setPixmap(map->renderCollision());
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
        this->editor->selectMapObject(this, mouse->modifiers() & Qt::ControlModifier);
        this->editor->updateSelectedObjects();
    }
    active = false;
}

QList<DraggablePixmapItem *> *Editor::getObjects() {
    QList<DraggablePixmapItem *> *list = new QList<DraggablePixmapItem *>;
    for (Event *event : map->getAllEvents()) {
        for (QGraphicsItem *child : objects_group->childItems()) {
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

void Editor::updateSelectedObjects() {
    for (DraggablePixmapItem *item : *(getObjects())) {
        redrawObject(item);
    }
    emit selectedObjectsChanged();
}

void Editor::selectMapObject(DraggablePixmapItem *object) {
    selectMapObject(object, false);
}

void Editor::selectMapObject(DraggablePixmapItem *object, bool toggle) {
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
        updateSelectedObjects();
    }
}

DraggablePixmapItem* Editor::addNewEvent() {
    return addNewEvent("object");
}

DraggablePixmapItem* Editor::addNewEvent(QString event_type) {
    if (project && map) {
        Event *event = new Event;
        event->put("map_name", map->name);
        event->put("event_type", event_type);
        map->addEvent(event);
        project->loadObjectPixmaps(map->getAllEvents());
        DraggablePixmapItem *object = addMapObject(event);

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
                    updateSelectedObjects();
                }
            }
        }
        clicking = false;
    }
}
