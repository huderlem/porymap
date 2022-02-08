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
    // delete all associated events
    while (!ownedEvents.isEmpty()) {
        Event *last = ownedEvents.takeLast();
        if (last) delete last;
    }
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

bool Map::mapBlockChanged(int i, const Blockdata &cache) {
    if (cache.length() <= i)
        return true;
    if (layout->blockdata.length() <= i)
        return true;

    return layout->blockdata.at(i) != cache.at(i);
}

bool Map::borderBlockChanged(int i, const Blockdata &cache) {
    if (cache.length() <= i)
        return true;
    if (layout->border.length() <= i)
        return true;

    return layout->border.at(i) != cache.at(i);
}

void Map::cacheBorder() {
    layout->cached_border.clear();
    for (const auto &block : layout->border)
        layout->cached_border.append(block);
}

void Map::cacheBlockdata() {
    layout->cached_blockdata.clear();
    for (const auto &block : layout->blockdata)
        layout->cached_blockdata.append(block);
}

void Map::cacheCollision() {
    layout->cached_collision.clear();
    for (const auto &block : layout->blockdata)
        layout->cached_collision.append(block);
}

QPixmap Map::renderCollision(qreal opacity, bool ignoreCache) {
    bool changed_any = false;
    int width_ = getWidth();
    int height_ = getHeight();
    if (collision_image.isNull() || collision_image.width() != width_ * 16 || collision_image.height() != height_ * 16) {
        collision_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (layout->blockdata.isEmpty() || !width_ || !height_) {
        collision_pixmap = collision_pixmap.fromImage(collision_image);
        return collision_pixmap;
    }
    QPainter painter(&collision_image);
    for (int i = 0; i < layout->blockdata.length(); i++) {
        if (!ignoreCache && !mapBlockChanged(i, layout->cached_collision)) {
            continue;
        }
        changed_any = true;
        Block block = layout->blockdata.at(i);
        QImage metatile_image = getMetatileImage(block.metatileId, layout->tileset_primary, layout->tileset_secondary, metatileLayerOrder, metatileLayerOpacity);
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
    if (image.isNull() || image.width() != width_ * 16 || image.height() != height_ * 16) {
        image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (layout->blockdata.isEmpty() || !width_ || !height_) {
        pixmap = pixmap.fromImage(image);
        return pixmap;
    }

    QPainter painter(&image);
    for (int i = 0; i < layout->blockdata.length(); i++) {
        if (!ignoreCache && !mapBlockChanged(i, layout->cached_blockdata)) {
            continue;
        }
        changed_any = true;
        Block block = layout->blockdata.at(i);
        QImage metatile_image = getMetatileImage(
            block.metatileId,
            fromLayout ? fromLayout->tileset_primary   : layout->tileset_primary,
            fromLayout ? fromLayout->tileset_secondary : layout->tileset_secondary,
            metatileLayerOrder,
            metatileLayerOpacity
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
    if (layout->border.isEmpty()) {
        layout->border_pixmap = layout->border_pixmap.fromImage(layout->border_image);
        return layout->border_pixmap;
    }
    QPainter painter(&layout->border_image);
    for (int i = 0; i < layout->border.length(); i++) {
        if (!ignoreCache && (!border_resized && !borderBlockChanged(i, layout->cached_border))) {
            continue;
        }

        changed_any = true;
        Block block = layout->border.at(i);
        uint16_t metatileId = block.metatileId;
        QImage metatile_image = getMetatileImage(metatileId, layout->tileset_primary, layout->tileset_secondary, metatileLayerOrder, metatileLayerOpacity);
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

    Blockdata newBlockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockdata.append(layout->blockdata.value(index));
        } else {
            newBlockdata.append(0);
        }
    }

    layout->blockdata = newBlockdata;
}

void Map::setNewBorderDimensionsBlockdata(int newWidth, int newHeight) {
    int oldWidth = getBorderWidth();
    int oldHeight = getBorderHeight();

    Blockdata newBlockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockdata.append(layout->border.value(index));
        } else {
            newBlockdata.append(0);
        }
    }

    layout->border = newBlockdata;
}

void Map::setDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewDimensionsBlockdata(newWidth, newHeight);
    }

    int oldWidth = layout->width.toInt();
    int oldHeight = layout->height.toInt();
    layout->width = QString::number(newWidth);
    layout->height = QString::number(newHeight);

    if (oldWidth != newWidth || oldHeight != newHeight) {
        Scripting::cb_MapResized(oldWidth, oldHeight, newWidth, newHeight);
    }

    emit mapChanged(this);
    emit mapDimensionsChanged(QSize(getWidth(), getHeight()));
}

void Map::setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewBorderDimensionsBlockdata(newWidth, newHeight);
    }

    layout->border_width = QString::number(newWidth);
    layout->border_height = QString::number(newHeight);

    emit mapChanged(this);
}

bool Map::getBlock(int x, int y, Block *out) {
    if (isWithinBounds(x, y)) {
        int i = y * getWidth() + x;
        *out = layout->blockdata.value(i);
        return true;
    }
    return false;
}

void Map::setBlock(int x, int y, Block block, bool enableScriptCallback) {
    int i = y * getWidth() + x;
    if (i < layout->blockdata.size()) {
        Block prevBlock = layout->blockdata.at(i);
        layout->blockdata.replace(i, block);
        if (enableScriptCallback) {
            Scripting::cb_MetatileChanged(x, y, prevBlock, block);
        }
    }
}

void Map::setBlockdata(Blockdata blockdata) {
    int width = getWidth();
    int size = qMin(blockdata.size(), layout->blockdata.size());
    for (int i = 0; i < size; i++) {
        Block prevBlock = layout->blockdata.at(i);
        Block newBlock = blockdata.at(i);
        if (prevBlock != newBlock) {
            layout->blockdata.replace(i, newBlock);
            Scripting::cb_MetatileChanged(i % width, i / width, prevBlock, newBlock);
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
        Block block;
        if (!getBlock(x, y, &block)) {
            continue;
        }

        uint old_coll = block.collision;
        uint old_elev = block.elevation;
        if (old_coll == collision && old_elev == elevation) {
            continue;
        }

        block.collision = collision;
        block.elevation = elevation;
        setBlock(x, y, block, true);
        if (getBlock(x + 1, y, &block) && block.collision == old_coll && block.elevation == old_elev) {
            todo.append(QPoint(x + 1, y));
        }
        if (getBlock(x - 1, y, &block) && block.collision == old_coll && block.elevation == old_elev) {
            todo.append(QPoint(x - 1, y));
        }
        if (getBlock(x, y + 1, &block) && block.collision == old_coll && block.elevation == old_elev) {
            todo.append(QPoint(x, y + 1));
        }
        if (getBlock(x, y - 1, &block) && block.collision == old_coll && block.elevation == old_elev) {
            todo.append(QPoint(x, y - 1));
        }
    }
}

void Map::floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    Block block;
    if (getBlock(x, y, &block) && (block.collision != collision || block.elevation != elevation)) {
        _floodFillCollisionElevation(x, y, collision, elevation);
    }
}

void Map::magicFillCollisionElevation(int initialX, int initialY, uint16_t collision, uint16_t elevation) {
    Block block;
    if (getBlock(initialX, initialY, &block) && (block.collision != collision || block.elevation != elevation)) {
        uint old_coll = block.collision;
        uint old_elev = block.elevation;

        for (int y = 0; y < getHeight(); y++) {
            for (int x = 0; x < getWidth(); x++) {
                if (getBlock(x, y, &block) && block.collision == old_coll && block.elevation == old_elev) {
                    block.collision = collision;
                    block.elevation = elevation;
                    setBlock(x, y, block, true);
                }
            }
        }
    }
}

QList<Event *> Map::getAllEvents() const {
    QList<Event *> all_events;
    for (const auto &event_list : events) {
        all_events << event_list;
    }
    return all_events;
}

QStringList Map::eventScriptLabels(const QString &event_group_type) const {
    QStringList scriptLabels;
    if (event_group_type.isEmpty()) {
        for (const auto *event : getAllEvents())
            scriptLabels << event->get("script_label");
    } else {
        for (const auto *event : events.value(event_group_type))
            scriptLabels << event->get("script_label");
    }

    scriptLabels.removeAll("");
    scriptLabels.removeDuplicates();
    if (scriptLabels.contains("0x0"))
        scriptLabels.move(scriptLabels.indexOf("0x0"), scriptLabels.count() - 1);
    if (scriptLabels.contains("NULL"))
        scriptLabels.move(scriptLabels.indexOf("NULL"), scriptLabels.count() - 1);

    return scriptLabels;
}

void Map::removeEvent(Event *event) {
    for (QString key : events.keys()) {
        events[key].removeAll(event);
    }
}

void Map::addEvent(Event *event) {
    events[event->get("event_group_type")].append(event);
    if (!ownedEvents.contains(event)) ownedEvents.append(event);
}

bool Map::hasUnsavedChanges() {
    return !editHistory.isClean() || hasUnsavedDataChanges || !isPersistedToFile;
}

bool Map::isWithinBounds(int x, int y) {
    return (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight());
}
