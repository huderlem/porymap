#include "map.h"

#include <QTime>
#include <QDebug>
#include <QPainter>
#include <QImage>
#include <QRegularExpression>


Map::Map(QObject *parent) : QObject(parent)
{
    paint_tile_index = 1;
    paint_collision = 0;
    paint_elevation = 3;
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

int Map::getSelectedBlockIndex(int index) {
    if (index < layout->tileset_primary->metatiles->length()) {
        return index;
    } else {
        return 0x200 + (index - layout->tileset_primary->metatiles->length());
    }
}

int Map::getDisplayedBlockIndex(int index) {
    if (index < layout->tileset_primary->metatiles->length()) {
        return index;
    } else {
        return index - 0x200 + layout->tileset_primary->metatiles->length();
    }
}

QImage Map::getCollisionMetatileImage(Block block) {
    return getCollisionMetatileImage(block.collision);
}

QImage Map::getCollisionMetatileImage(int collision) {
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);
    QColor color;
    if (collision == 0) {
        color.setGreen(0xff);
    } else if (collision == 1) {
        color.setRed(0xff);
    } else if (collision == 2) {
        color.setBlue(0xff);
    } else if (collision == 3) {
        // black
    }
    metatile_image.fill(color);
    return metatile_image;
}

QImage Map::getElevationMetatileImage(Block block) {
    return getElevationMetatileImage(block.elevation);
}

QImage Map::getElevationMetatileImage(int elevation) {
    QImage metatile_image(16, 16, QImage::Format_RGBA8888);
    QColor color;
    if (elevation < 15) {
        uint saturation = (elevation + 1) * 16 + 15;
        color.setGreen(saturation);
        color.setRed(saturation);
        color.setBlue(saturation);
    } else {
        color.setGreen(0xd0);
        color.setBlue(0xd0);
        color.setRed(0);
    }
    metatile_image.fill(color);
    //QPainter painter(&metatile_image);
    //painter.end();
    return metatile_image;
}

bool Map::blockChanged(int i, Blockdata *cache) {
    if (cache == NULL || cache == nullptr) {
        return true;
    }
    if (layout->blockdata == NULL || layout->blockdata == nullptr) {
        return true;
    }
    if (cache->blocks == NULL || cache->blocks == nullptr) {
        return true;
    }
    if (layout->blockdata->blocks == NULL || layout->blockdata->blocks == nullptr) {
        return true;
    }
    if (cache->blocks->length() <= i) {
        return true;
    }
    if (layout->blockdata->blocks->length() <= i) {
        return true;
    }
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
        QImage metatile_image = Metatile::getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
        QImage collision_metatile_image = getCollisionMetatileImage(block);
        QImage elevation_metatile_image = getElevationMetatileImage(block);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.setOpacity(1);
        painter.drawImage(metatile_origin, metatile_image);

        painter.save();
        if (block.elevation == 15) {
            painter.setOpacity(0.5);
        } else if (block.elevation == 0) {
            painter.setOpacity(0);
        } else {
            painter.setOpacity(1);//(block.elevation / 16.0) * 0.8);
            painter.setCompositionMode(QPainter::CompositionMode_Overlay);
        }
        painter.drawImage(metatile_origin, elevation_metatile_image);
        painter.restore();

        painter.save();
        if (block.collision == 0) {
            painter.setOpacity(0.1);
        } else {
            painter.setOpacity(0.4);
        }
        painter.drawImage(metatile_origin, collision_metatile_image);
        painter.restore();

        painter.save();
        painter.setOpacity(0.6);
        painter.setPen(QColor(255, 255, 255, 192));
        painter.setFont(QFont("Helvetica", 8));
        painter.drawText(QPoint(metatile_origin.x(), metatile_origin.y() + 8), QString("%1").arg(block.elevation));
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
        QImage metatile_image = Metatile::getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
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
        QImage metatile_image = Metatile::getMetatileImage(block.tile, layout->tileset_primary, layout->tileset_secondary);
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

QPixmap Map::renderConnection(Connection connection) {
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

QPixmap Map::renderCollisionMetatiles() {
    int length_ = 4;
    int height_ = 1;
    int width_ = length_ / height_;
    QImage image(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int y = i / width_;
        int x = i % width_;
        QPoint origin(x * 16, y * 16);
        QImage metatile_image = getCollisionMetatileImage(i);
        painter.drawImage(origin, metatile_image);
    }
    drawSelection(paint_collision, width_, 1, 1, &painter);
    painter.end();
    return QPixmap::fromImage(image);
}

QPixmap Map::renderElevationMetatiles() {
    int length_ = 16;
    int height_ = 2;
    int width_ = length_ / height_;
    QImage image(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int y = i / width_;
        int x = i % width_;
        QPoint origin(x * 16, y * 16);
        QImage metatile_image = getElevationMetatileImage(i);
        painter.drawImage(origin, metatile_image);
    }
    drawSelection(paint_elevation, width_, 1, 1, &painter);
    painter.end();
    return QPixmap::fromImage(image);
}

void Map::drawSelection(int i, int w, int selectionWidth, int selectionHeight, QPainter *painter) {
    int x = i % w;
    int y = i / w;
    painter->save();

    QColor penColor = QColor(0xff, 0xff, 0xff);
    painter->setPen(penColor);
    int rectWidth = selectionWidth * 16;
    int rectHeight = selectionHeight * 16;
    painter->drawRect(x * 16, y * 16, rectWidth - 1, rectHeight -1);
    painter->setPen(QColor(0, 0, 0));
    painter->drawRect(x * 16 - 1, y * 16 - 1, rectWidth + 1, rectHeight + 1);
    painter->drawRect(x * 16 + 1, y * 16 + 1, rectWidth - 3, rectHeight - 3);
    painter->restore();
}

QPixmap Map::renderMetatiles() {
    if (!layout->tileset_primary || !layout->tileset_primary->metatiles
     || !layout->tileset_secondary || !layout->tileset_secondary->metatiles) {
        return QPixmap();
    }
    int primary_length = layout->tileset_primary->metatiles->length();
    int length_ = primary_length + layout->tileset_secondary->metatiles->length();
    int width_ = 8;
    int height_ = length_ / width_;
    QImage image(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        int tile = i;
        if (i >= primary_length) {
            tile += 0x200 - primary_length;
        }
        QImage metatile_image = Metatile::getMetatileImage(tile, layout->tileset_primary, layout->tileset_secondary);
        int map_y = i / width_;
        int map_x = i % width_;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    drawSelection(paint_tile_index, width_, paint_tile_width, paint_tile_height, &painter);

    painter.end();
    return QPixmap::fromImage(image);
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
        if (x >= 0 && x < getWidth())
        if (y >= 0 && y < getHeight()) {
            int i = y * getWidth() + x;
            return new Block(layout->blockdata->blocks->value(i));
        }
    }
    return NULL;
}

void Map::_setBlock(int x, int y, Block block) {
    int i = y * getWidth() + x;
    if (layout->blockdata && layout->blockdata->blocks) {
        layout->blockdata->blocks->replace(i, block);
    }
}

void Map::_floodFillCollision(int x, int y, uint collision) {
    QList<QPoint> todo;
    todo.append(QPoint(x, y));
    while (todo.length()) {
            QPoint point = todo.takeAt(0);
            x = point.x();
            y = point.y();
            Block *block = getBlock(x, y);
            if (block == NULL) {
                continue;
            }
            uint old_coll = block->collision;
            if (old_coll == collision) {
                continue;
            }
            block->collision = collision;
            _setBlock(x, y, *block);
            if ((block = getBlock(x + 1, y)) && block->collision == old_coll) {
                todo.append(QPoint(x + 1, y));
            }
            if ((block = getBlock(x - 1, y)) && block->collision == old_coll) {
                todo.append(QPoint(x - 1, y));
            }
            if ((block = getBlock(x, y + 1)) && block->collision == old_coll) {
                todo.append(QPoint(x, y + 1));
            }
            if ((block = getBlock(x, y - 1)) && block->collision == old_coll) {
                todo.append(QPoint(x, y - 1));
            }
    }
}

void Map::_floodFillElevation(int x, int y, uint elevation) {
    QList<QPoint> todo;
    todo.append(QPoint(x, y));
    while (todo.length()) {
            QPoint point = todo.takeAt(0);
            x = point.x();
            y = point.y();
            Block *block = getBlock(x, y);
            if (block == NULL) {
                continue;
            }
            uint old_z = block->elevation;
            if (old_z == elevation) {
                continue;
            }
            Block block_(*block);
            block_.elevation = elevation;
            _setBlock(x, y, block_);
            if ((block = getBlock(x + 1, y)) && block->elevation == old_z) {
                todo.append(QPoint(x + 1, y));
            }
            if ((block = getBlock(x - 1, y)) && block->elevation == old_z) {
                todo.append(QPoint(x - 1, y));
            }
            if ((block = getBlock(x, y + 1)) && block->elevation == old_z) {
                todo.append(QPoint(x, y + 1));
            }
            if ((block = getBlock(x, y - 1)) && block->elevation == old_z) {
                todo.append(QPoint(x, y - 1));
            }
    }
}

void Map::_floodFillCollisionElevation(int x, int y, uint collision, uint elevation) {
    QList<QPoint> todo;
    todo.append(QPoint(x, y));
    while (todo.length()) {
            QPoint point = todo.takeAt(0);
            x = point.x();
            y = point.y();
            Block *block = getBlock(x, y);
            if (block == NULL) {
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
    HistoryItem *commit = history.back();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight())
        {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            emit mapNeedsRedrawing(this);
        }

        emit mapChanged(this);
    }
}

void Map::redo() {
    HistoryItem *commit = history.next();
    if (!commit)
        return;

    if (layout->blockdata) {
        layout->blockdata->copyFrom(commit->metatiles);
        if (commit->layoutWidth != this->getWidth() || commit->layoutHeight != this->getHeight())
        {
            this->setDimensions(commit->layoutWidth, commit->layoutHeight, false);
            emit mapNeedsRedrawing(this);
        }

        emit mapChanged(this);
    }
}

void Map::commit() {
    if (layout->blockdata) {
        HistoryItem *item = history.current();
        bool atCurrentHistory = item
                && layout->blockdata->equals(item->metatiles)
                && this->getWidth() == item->layoutWidth
                && this->getHeight() == item->layoutHeight;
        if (!atCurrentHistory) {
            HistoryItem *commit = new HistoryItem(layout->blockdata->copy(), this->getWidth(), this->getHeight());
            history.push(commit);
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

void Map::floodFillCollision(int x, int y, uint collision) {
    Block *block = getBlock(x, y);
    if (block && block->collision != collision) {
        _floodFillCollision(x, y, collision);
        commit();
    }
}

void Map::floodFillElevation(int x, int y, uint elevation) {
    Block *block = getBlock(x, y);
    if (block && block->elevation != elevation) {
        _floodFillElevation(x, y, elevation);
        commit();
    }
}
void Map::floodFillCollisionElevation(int x, int y, uint collision, uint elevation) {
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
    return !history.isSaved() || !isPersistedToFile || layout->has_unsaved_changes;
}

void Map::hoveredTileChanged(int x, int y, int block) {
    emit statusBarMessage(QString("X: %1, Y: %2, Block: 0x%3")
                          .arg(x)
                          .arg(y)
                          .arg(QString("%1").arg(block, 3, 16, QChar('0')).toUpper()));
}

void Map::clearHoveredTile() {
    emit statusBarMessage(QString(""));
}

void Map::hoveredMetatileChanged(int blockIndex) {
    int tile = getSelectedBlockIndex(blockIndex);
    emit statusBarMessage(QString("Block: 0x%1")
                          .arg(QString("%1").arg(tile, 3, 16, QChar('0')).toUpper()));
}

void Map::clearHoveredMetatile() {
    emit statusBarMessage(QString(""));
}

void Map::hoveredCollisionTileChanged(int collision) {
    emit statusBarMessage(QString("Collision: %1").arg(collision));
}

void Map::clearHoveredCollisionTile() {
    emit statusBarMessage(QString(""));
}

void Map::hoveredElevationTileChanged(int elevation) {
    emit statusBarMessage(QString("Elevation: %1").arg(elevation));
}

void Map::clearHoveredElevationTile() {
    emit statusBarMessage(QString(""));
}
