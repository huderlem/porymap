#include "history.h"
#include "historyitem.h"
#include "map.h"
#include "imageproviders.h"
#include "scripting.h"

#include <QTime>
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

int Map::getBorderWidth() {
    return layout->border_width.toInt(nullptr, 0);
}

int Map::getBorderHeight() {
    return layout->border_height.toInt(nullptr, 0);
}

bool Map::mapBlockChanged(int i, Blockdata * cache) {
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

bool Map::borderBlockChanged(int i, Blockdata * cache) {
    if (!cache)
        return true;
    if (!layout->border)
        return true;
    if (!cache->blocks)
        return true;
    if (!layout->border->blocks)
        return true;
    if (cache->blocks->length() <= i)
        return true;
    if (layout->border->blocks->length() <= i)
        return true;

    return layout->border->blocks->value(i) != cache->blocks->value(i);
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

QPixmap Map::renderCollision(qreal opacity, bool ignoreCache) {
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
        if (!ignoreCache && layout->cached_collision && !mapBlockChanged(i, layout->cached_collision)) {
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
        painter.setOpacity(opacity);
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

QPixmap Map::render(bool ignoreCache = false, MapLayout * fromLayout) {
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
        if (!ignoreCache && !mapBlockChanged(i, layout->cached_blockdata)) {
            continue;
        }
        changed_any = true;
        Block block = layout->blockdata->blocks->value(i);
        QImage metatile_image = getMetatileImage(
            block.tile,
            fromLayout ? fromLayout->tileset_primary   : layout->tileset_primary,
            fromLayout ? fromLayout->tileset_secondary : layout->tileset_secondary
        );
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

QPixmap Map::renderBorder(bool ignoreCache) {
    bool changed_any = false, border_resized = false;
    int width_ = getBorderWidth();
    int height_ = getBorderHeight();
    if (layout->border_image.isNull()) {
        layout->border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (layout->border_image.width() != width_ * 16 || layout->border_image.height() != height_ * 16) {
        layout->border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        border_resized = true;
    }
    if (!(layout->border && layout->border->blocks)) {
        layout->border_pixmap = layout->border_pixmap.fromImage(layout->border_image);
        return layout->border_pixmap;
    }
    QPainter painter(&layout->border_image);
    for (int i = 0; i < layout->border->blocks->length(); i++) {
        if (!ignoreCache && (!border_resized && !borderBlockChanged(i, layout->cached_border))) {
            continue;
        }

        changed_any = true;
        Block block = layout->border->blocks->value(i);
        uint16_t tile = block.tile;
        QImage metatile_image = getMetatileImage(tile, layout->tileset_primary, layout->tileset_secondary);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        painter.drawImage(QPoint(map_x * 16, map_y * 16), metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBorder();
        layout->border_pixmap = layout->border_pixmap.fromImage(layout->border_image);
    }
    return layout->border_pixmap;
}

QPixmap Map::renderConnection(MapConnection connection, MapLayout * fromLayout) {
    render(true, fromLayout);
    int x, y, w, h;
    if (connection.direction == "up") {
        x = 0;
        y = getHeight() - BORDER_DISTANCE;
        w = getWidth();
        h = BORDER_DISTANCE;
    } else if (connection.direction == "down") {
        x = 0;
        y = 0;
        w = getWidth();
        h = BORDER_DISTANCE;
    } else if (connection.direction == "left") {
        x = getWidth() - BORDER_DISTANCE;
        y = 0;
        w = BORDER_DISTANCE;
        h = getHeight();
    } else if (connection.direction == "right") {
        x = 0;
        y = 0;
        w = BORDER_DISTANCE;
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

void Map::setNewBorderDimensionsBlockdata(int newWidth, int newHeight) {
    int oldWidth = getBorderWidth();
    int oldHeight = getBorderHeight();

    Blockdata* newBlockData = new Blockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockData->addBlock(layout->border->blocks->value(index));
        } else {
            newBlockData->addBlock(0);
        }
    }

    layout->border->copyFrom(newBlockData);
}

void Map::setDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewDimensionsBlockdata(newWidth, newHeight);
    }

    layout->width = QString::number(newWidth);
    layout->height = QString::number(newHeight);

    emit mapChanged(this);
}

void Map::setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewBorderDimensionsBlockdata(newWidth, newHeight);
    }

    layout->border_width = QString::number(newWidth);
    layout->border_height = QString::number(newHeight);

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

void Map::setBlock(int x, int y, Block block, bool enableScriptCallback) {
    int i = y * getWidth() + x;
    if (layout->blockdata && layout->blockdata->blocks && i < layout->blockdata->blocks->size()) {
        Block prevBlock = layout->blockdata->blocks->value(i);
        layout->blockdata->blocks->replace(i, block);
        if (enableScriptCallback) {
            Scripting::cb_MetatileChanged(x, y, prevBlock, block);
        }
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
            setBlock(x, y, *block, true);
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
    bool redraw = false, changed = false;
    HistoryItem *commit = metatileHistory.back();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight()) {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            redraw = true;
        }
        changed = true;
    }
    if (layout->border) {
        layout->border->copyFrom(commit->border);
        if (commit->borderWidth != this->getBorderWidth() || commit->borderHeight != this->getBorderHeight()) {
            this->setBorderDimensions(commit->borderWidth, commit->borderHeight, false);
            redraw = true;
        }
        changed = true;
    }

    if (redraw) {
        emit mapNeedsRedrawing();
    }
    if (changed) {
        emit mapChanged(this);
    }
}

void Map::redo() {
    bool redraw = false, changed = false;
    HistoryItem *commit = metatileHistory.next();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight()) {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            redraw = true;
        }
        changed = true;
    }
    if (layout->border) {
        layout->border->copyFrom(commit->border);
        if (commit->borderWidth != this->getBorderWidth() || commit->borderHeight != this->getBorderHeight()) {
            this->setBorderDimensions(commit->borderWidth, commit->borderHeight, false);
            redraw = true;
        }
        changed = true;
    }

    if (redraw) {
        emit mapNeedsRedrawing();
    }
    if (changed) {
        emit mapChanged(this);
    }
}

void Map::commit() {
    if (!layout) {
        return;
    }

    int layoutWidth = this->getWidth();
    int layoutHeight = this->getHeight();
    int borderWidth = this->getBorderWidth();
    int borderHeight = this->getBorderHeight();

    if (layout->blockdata) {
        HistoryItem *item = metatileHistory.current();
        bool atCurrentHistory = item
                && layout->blockdata->equals(item->metatiles)
                && layout->border->equals(item->border)
                && layoutWidth == item->layoutWidth
                && layoutHeight == item->layoutHeight
                && borderWidth == item->borderWidth 
                && borderHeight == item->borderHeight;
        if (!atCurrentHistory) {
            HistoryItem *commit = new HistoryItem(layout->blockdata->copy(), layout->border->copy(), layoutWidth, layoutHeight, borderWidth, borderHeight);
            metatileHistory.push(commit);
            emit mapChanged(this);
        }
    }
}

void Map::floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    Block *block = getBlock(x, y);
    if (block && (block->collision != collision || block->elevation != elevation)) {
        _floodFillCollisionElevation(x, y, collision, elevation);
        commit();
    }
}

void Map::magicFillCollisionElevation(int initialX, int initialY, uint16_t collision, uint16_t elevation) {
    Block *block = getBlock(initialX, initialY);
    if (block && (block->collision != collision || block->elevation != elevation)) {
        uint old_coll = block->collision;
        uint old_elev = block->elevation;

        for (int y = 0; y < getHeight(); y++) {
            for (int x = 0; x < getWidth(); x++) {
                block = getBlock(x, y);
                if (block && block->collision == old_coll && block->elevation == old_elev) {
                    block->collision = collision;
                    block->elevation = elevation;
                    setBlock(x, y, *block, true);
                }
            }
        }
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
