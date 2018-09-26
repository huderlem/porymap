#include "history.h"
#include "historyitem.h"
#include "map.h"
#include "project.h"
#include "imageproviders.h"

#include <QTime>
#include <QDebug>
#include <QPainter>
#include <QImage>
#include <QRegularExpression>


Map::Map(QObject *parent) : QObject(parent)
{
}

void Map::setName(QString mapName) {
    name = mapName;
    constantName = mapConstantFromName(mapName);
}

QString Map::mapConstantFromName(QString mapName) {
    // Transform map names of the form 'GraniteCave_B1F` into map constants like 'MAP_GRANITE_CAVE_B1F'.
    QString nameWithUnderscores = mapName.replace(QRegularExpression("([a-z])([A-Z])"), "\\1_\\2");
    QString withMapAndUppercase = "MAP_" + nameWithUnderscores.toUpper();
    QString constantName = withMapAndUppercase.replace(QRegularExpression("_+"), "_");

    // Handle special cases.
    // SSTidal needs to be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

QString Map::objectEventsLabelFromName(QString mapName)
{
    return QString("%1_EventObjects").arg(mapName);
}

QString Map::warpEventsLabelFromName(QString mapName)
{
    return QString("%1_MapWarps").arg(mapName);
}

QString Map::coordEventsLabelFromName(QString mapName)
{
    return QString("%1_MapCoordEvents").arg(mapName);
}

QString Map::bgEventsLabelFromName(QString mapName)
{
    return QString("%1_MapBGEvents").arg(mapName);
}

int Map::getWidth() {
    return layout->width.toInt(nullptr, 0);
}

int Map::getHeight() {
    return layout->height.toInt(nullptr, 0);
}

bool Map::blockChanged(int i, Blockdata *cache) {
    if (!cache)
        return true;
    if (!layout->blockdata)
        return true;
    if (!cache->blocks)
        return true;
    if (!layout->blockdata->blocks)
        return true;
    if (cache->blocks->length() <= i)
        return true;
    if (layout->blockdata->blocks->length() <= i)
        return true;

    return layout->blockdata->blocks->value(i) != cache->blocks->value(i);
}

void Map::cacheBorder() {
    if (layout->cached_border) delete layout->cached_border;
    layout->cached_border = new Blockdata;
    if (layout->border && layout->border->blocks) {
        for (int i = 0; i < layout->border->blocks->length(); i++) {
            Block block = layout->border->blocks->value(i);
            layout->cached_border->blocks->append(block);
        }
    }
}

void Map::cacheBlockdata() {
    if (layout->cached_blockdata) delete layout->cached_blockdata;
    layout->cached_blockdata = new Blockdata;
    if (layout->blockdata && layout->blockdata->blocks) {
        for (int i = 0; i < layout->blockdata->blocks->length(); i++) {
            Block block = layout->blockdata->blocks->value(i);
            layout->cached_blockdata->blocks->append(block);
        }
    }
}

void Map::cacheCollision() {
    if (layout->cached_collision) delete layout->cached_collision;
    layout->cached_collision = new Blockdata;
    if (layout->blockdata && layout->blockdata->blocks) {
        for (int i = 0; i < layout->blockdata->blocks->length(); i++) {
            Block block = layout->blockdata->blocks->value(i);
            layout->cached_collision->blocks->append(block);
        }
    }
}

QPixmap Map::renderCollision(bool ignoreCache) {
    bool changed_any = false;
    int width_ = getWidth();
    int height_ = getHeight();
    if (
            collision_image.isNull()
            || collision_image.width() != width_ * 16
            || collision_image.height() != height_ * 16
    ) {
        collision_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (!(layout->blockdata && layout->blockdata->blocks && width_ && height_)) {
        collision_pixmap = collision_pixmap.fromImage(collision_image);
        return collision_pixmap;
    }
    QPainter painter(&collision_image);
    for (int i = 0; i < layout->blockdata->blocks->length(); i++) {
        if (!ignoreCache && layout->cached_collision && !blockChanged(i, layout->cached_collision)) {
            continue;
        }
        changed_any = true;
        Block block = layout->blockdata->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
        QImage collision_metatile_image = getCollisionMetatileImage(block);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.setOpacity(1);
        painter.drawImage(metatile_origin, metatile_image);
        painter.save();
        painter.setOpacity(0.55);
        painter.drawImage(metatile_origin, collision_metatile_image);
        painter.restore();
    }
    painter.end();
    cacheCollision();
    if (changed_any) {
        collision_pixmap = collision_pixmap.fromImage(collision_image);
    }
    return collision_pixmap;
}

QPixmap Map::render(bool ignoreCache = false) {
    bool changed_any = false;
    int width_ = getWidth();
    int height_ = getHeight();
    if (
            image.isNull()
            || image.width() != width_ * 16
            || image.height() != height_ * 16
    ) {
        image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (!(layout->blockdata && layout->blockdata->blocks && width_ && height_)) {
        pixmap = pixmap.fromImage(image);
        return pixmap;
    }

    QPainter painter(&image);
    for (int i = 0; i < layout->blockdata->blocks->length(); i++) {
        if (!ignoreCache && !blockChanged(i, layout->cached_blockdata)) {
            continue;
        }
        changed_any = true;
        Block block = layout->blockdata->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBlockdata();
        pixmap = pixmap.fromImage(image);
    }

    return pixmap;
}

QPixmap Map::renderBorder() {
    bool changed_any = false;
    int width_ = 2;
    int height_ = 2;
    if (layout->border_image.isNull()) {
        layout->border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (!(layout->border && layout->border->blocks)) {
        layout->border_pixmap = layout->border_pixmap.fromImage(layout->border_image);
        return layout->border_pixmap;
    }
    QPainter painter(&layout->border_image);
    for (int i = 0; i < layout->border->blocks->length(); i++) {
        if (!blockChanged(i, layout->cached_border)) {
            continue;
        }
        changed_any = true;
        Block block = layout->border->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
        int map_y = i / width_;
        int map_x = i % width_;
        painter.drawImage(QPoint(map_x * 16, map_y * 16), metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBorder();
        layout->border_pixmap = layout->border_pixmap.fromImage(layout->border_image);
    }
    return layout->border_pixmap;
}

QPixmap Map::renderConnection(MapConnection connection) {
    render();
    int x, y, w, h;
    if (connection.direction == "up") {
        x = 0;
        y = getHeight() - 6;
        w = getWidth();
        h = 6;
    } else if (connection.direction == "down") {
        x = 0;
        y = 0;
        w = getWidth();
        h = 6;
    } else if (connection.direction == "left") {
        x = getWidth() - 6;
        y = 0;
        w = 6;
        h = getHeight();
    } else if (connection.direction == "right") {
        x = 0;
        y = 0;
        w = 6;
        h = getHeight();
    } else {
        // this should not happen
        x = 0;
        y = 0;
        w = getWidth();
        h = getHeight();
    }
    QImage connection_image = image.copy(x * 16, y * 16, w * 16, h * 16);
    //connection_image = connection_image.convertToFormat(QImage::Format_Grayscale8);
    return QPixmap::fromImage(connection_image);
}

void Map::setNewDimensionsBlockdata(int newWidth, int newHeight) {
    int oldWidth = getWidth();
    int oldHeight = getHeight();

    Blockdata* newBlockData = new Blockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockData->addBlock(layout->blockdata->blocks->value(index));
        } else {
            newBlockData->addBlock(0);
        }
    }

    layout->blockdata->copyFrom(newBlockData);
}

void Map::setDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewDimensionsBlockdata(newWidth, newHeight);
    }

    layout->width = QString::number(newWidth);
    layout->height = QString::number(newHeight);

    emit mapChanged(this);
}

Block* Map::getBlock(int x, int y) {
    if (layout->blockdata && layout->blockdata->blocks) {
        if (x >= 0 && x < getWidth() && y >= 0 && y < getHeight()) {
            int i = y * getWidth() + x;
            return new Block(layout->blockdata->blocks->value(i));
        }
    }
    return nullptr;
}

void Map::_setBlock(int x, int y, Block block) {
    int i = y * getWidth() + x;
    if (layout->blockdata && layout->blockdata->blocks) {
        layout->blockdata->blocks->replace(i, block);
    }
}

void Map::_floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    QList<QPoint> todo;
    todo.append(QPoint(x, y));
    while (todo.length()) {
            QPoint point = todo.takeAt(0);
            x = point.x();
            y = point.y();
            Block *block = getBlock(x, y);
            if (!block) {
                continue;
            }

            uint old_coll = block->collision;
            uint old_elev = block->elevation;
            if (old_coll == collision && old_elev == elevation) {
                continue;
            }

            block->collision = collision;
            block->elevation = elevation;
            _setBlock(x, y, *block);
            if ((block = getBlock(x + 1, y)) && block->collision == old_coll && block->elevation == old_elev) {
                todo.append(QPoint(x + 1, y));
            }
            if ((block = getBlock(x - 1, y)) && block->collision == old_coll && block->elevation == old_elev) {
                todo.append(QPoint(x - 1, y));
            }
            if ((block = getBlock(x, y + 1)) && block->collision == old_coll && block->elevation == old_elev) {
                todo.append(QPoint(x, y + 1));
            }
            if ((block = getBlock(x, y - 1)) && block->collision == old_coll && block->elevation == old_elev) {
                todo.append(QPoint(x, y - 1));
            }
    }
}


void Map::undo() {
    HistoryItem *commit = metatileHistory.back();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight())
        {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            emit mapNeedsRedrawing();
        }

        emit mapChanged(this);
    }
}

void Map::redo() {
    HistoryItem *commit = metatileHistory.next();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight())
        {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            emit mapNeedsRedrawing();
        }

        emit mapChanged(this);
    }
}

void Map::commit() {
    if (layout->blockdata) {
        HistoryItem *item = metatileHistory.current();
        bool atCurrentHistory = item
                && layout->blockdata->equals(item->metatiles)
                && this->getWidth() == item->layoutWidth
                && this->getHeight() == item->layoutHeight;
        if (!atCurrentHistory) {
            HistoryItem *commit = new HistoryItem(layout->blockdata->copy(), this->getWidth(), this->getHeight());
            metatileHistory.push(commit);
            emit mapChanged(this);
        }
    }
}

void Map::setBlock(int x, int y, Block block) {
    Block *old_block = getBlock(x, y);
    if (old_block && (*old_block) != block) {
        _setBlock(x, y, block);
        commit();
    }
}

void Map::floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    Block *block = getBlock(x, y);
    if (block && (block->collision != collision || block->elevation != elevation)) {
        _floodFillCollisionElevation(x, y, collision, elevation);
        commit();
    }
}

QList<Event *> Map::getAllEvents() {
    QList<Event*> all;
    for (QList<Event*> list : events.values()) {
        all += list;
    }
    return all;
}

void Map::removeEvent(Event *event) {
    for (QString key : events.keys()) {
        events[key].removeAll(event);
    }
}

void Map::addEvent(Event *event) {
    events[event->get("event_group_type")].append(event);
}

bool Map::hasUnsavedChanges() {
    return !metatileHistory.isSaved() || !isPersistedToFile || layout->has_unsaved_changes;
}
