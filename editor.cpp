#include "editor.h"

Editor::Editor()
{

}

void Editor::saveProject() {
    if (project) {
        project->saveAllMaps();
    }
}

void Editor::save() {
    if (project && map) {
        project->saveMap(map);
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
    map_item->draw();
    map_item->setVisible(true);
    map_item->setEnabled(true);
    collision_item->setVisible(false);
    objects_group->setVisible(false);
}

void Editor::setEditingCollision() {
    current_view = collision_item;
    collision_item->draw();
    collision_item->setVisible(true);
    map_item->setVisible(false);
    objects_group->setVisible(false);
}

void Editor::setEditingObjects() {
    objects_group->setVisible(true);
    map_item->setVisible(true);
    map_item->setEnabled(false);
    collision_item->setVisible(false);
}

void Editor::setMap(QString map_name) {
    if (map_name.isNull()) {
        return;
    }
    map = project->getMap(map_name);
    displayMap();
}

void Editor::displayMap() {
    scene = new QGraphicsScene;

    map_item = new MapPixmapItem(map);
    map_item->draw();
    scene->addItem(map_item);

    collision_item = new CollisionPixmapItem(map);
    collision_item->draw();
    scene->addItem(collision_item);

    objects_group = new QGraphicsItemGroup;
    scene->addItem(objects_group);

    map_item->setVisible(false);
    collision_item->setVisible(false);
    objects_group->setVisible(false);

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

    project->loadObjectPixmaps(map->object_events);
    for (int i = 0; i < map->object_events.length(); i++) {
        ObjectEvent *object_event = map->object_events.value(i);
        DraggablePixmapItem *object = new DraggablePixmapItem(object_event);
        objects_group->addToGroup(object);
    }
    objects_group->setFiltersChildEvents(false);
}

void Editor::displayMapConnections() {
    for (Connection *connection : map->connections) {
        if (connection->direction == "dive" || connection->direction == "emerge") {
            continue;
        }
        Map *connected_map = project->getMap(connection->map_name);
        QPixmap pixmap = connected_map->renderConnection(*connection);
        int offset = connection->offset.toInt(nullptr, 0);
        int x, y;
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
        item->setZValue(-1);
        scene->addItem(item);
    }
}


void MetatilesPixmapItem::draw() {
    setPixmap(map->renderMetatiles());
}

void MetatilesPixmapItem::pick(uint tile) {
    map->paint_tile = tile;
    draw();
}

void MetatilesPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = ((int)pos.x()) / 16;
    int y = ((int)pos.y()) / 16;
    //qDebug() << QString("(%1, %2)").arg(x).arg(y);
    int width = pixmap().width() / 16;
    int height = pixmap().height() / 16;
    if ((x >= 0 && x < width) && (y >=0 && y < height)) {
        pick(y * width + x);
    }
}
void MetatilesPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    mousePressEvent(event);
}


void MapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    if (map) {
        QPointF pos = event->pos();
        int x = (int)(pos.x()) / 16;
        int y = (int)(pos.y()) / 16;
        Block *block = map->getBlock(x, y);
        if (block) {
            block->tile = map->paint_tile;
            map->setBlock(x, y, *block);
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

void MapPixmapItem::draw() {
    setPixmap(map->render());
}

void MapPixmapItem::undo() {
    map->undo();
    draw();
}

void MapPixmapItem::redo() {
    map->redo();
    draw();
}

void MapPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    active = true;
    if (event->button() == Qt::RightButton) {
        right_click = true;
        floodFill(event);
    } else {
        right_click = false;
        paint(event);
    }
}
void MapPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (active) {
        if (right_click) {
            floodFill(event);
        } else {
            paint(event);
        }
    }
}
void MapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    active = false;
}

void CollisionPixmapItem::draw() {
    setPixmap(map->renderCollision());
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
            map->setBlock(x, y, *block);
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

void DraggablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouse) {
    active = true;
    last_x = mouse->pos().x() / 16;
    last_y = mouse->pos().y() / 16;
    qDebug() << event->x_ + ", " + event->y_;
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        int x = mouse->pos().x() / 16;
        int y = mouse->pos().y() / 16;
        if (x != last_x || y != last_y) {
            event->setX(event->x() + x - last_x);
            event->setY(event->y() + y - last_y);
            update();
        }
    }
}

void DraggablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouse) {
    active = false;
}
