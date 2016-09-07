#include "map.h"

#include <QTime>
#include <QDebug>
#include <QPainter>

Map::Map(QObject *parent) : QObject(parent)
{
    cached_blockdata = new Blockdata;
    cached_collision = new Blockdata;
    cached_border = new Blockdata;
    paint_tile = 1;
    paint_collision = 0;
    paint_elevation = 3;
}

int Map::getWidth() {
    return width.toInt(nullptr, 0);
}

int Map::getHeight() {
    return height.toInt(nullptr, 0);
}

Tileset* Map::getBlockTileset(uint metatile_index) {
    uint primary_size = 0x200;//tileset_primary->metatiles->length();
    if (metatile_index < primary_size) {
        return tileset_primary;
    } else {
        return tileset_secondary;
    }
}

QImage Map::getMetatileTile(uint tile) {
    uint primary_size = 0x200;//tileset_primary->metatiles->length();
    if (tile < primary_size) {
        return tileset_primary->tiles->value(tile);
    } else {
        return tileset_secondary->tiles->value(tile - primary_size);
    }
}

Metatile* Map::getMetatile(uint index) {
    uint primary_size = 0x200;//tileset_primary->metatiles->length();
    if (index < primary_size) {
        return tileset_primary->metatiles->value(index);
    } else {
        //qDebug() << QString("secondary tileset: %1").arg(index - primary_size, 0, 16);
        return tileset_secondary->metatiles->value(index - primary_size);
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

QImage Map::getMetatileImage(uint tile) {

    QImage metatile_image(16, 16, QImage::Format_RGBA8888);

    Metatile* metatile = getMetatile(tile);
    if (metatile == NULL) {
        metatile_image.fill(0xffffffff);
        return metatile_image;
    }

    Tileset* blockTileset = getBlockTileset(tile);

    for (int layer = 0; layer < 2; layer++)
    for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++) {
        //qDebug() << QString("x=%1 y=%2 layer=%3").arg(x).arg(y).arg(layer);
        Tile tile = metatile->tiles->value((y * 2) + x + (layer * 4));
        QImage tile_image = getMetatileTile(tile.tile);
        QList<QRgb> palette = blockTileset->palettes->value(tile.palette);
        for (int j = 0; j < palette.length(); j++) {
            tile_image.setColor(j, palette.value(j));
        }
        //QVector<QRgb> vector = palette.toVector();
        //tile_image.setColorTable(vector);
        if (layer > 0) {
            QColor color(tile_image.color(15));
            color.setAlpha(0);
            tile_image.setColor(15, color.rgba());
        }
        QPainter metatile_painter(&metatile_image);
        QPoint origin = QPoint(x*8, y*8);
        metatile_painter.drawImage(origin, tile_image.mirrored(tile.xflip == 1, tile.yflip == 1));
        metatile_painter.end();
    }

    return metatile_image;
}

bool Map::blockChanged(int i, Blockdata *cache) {
    if (cache->blocks->length() <= i) {
        return true;
    }
    return blockdata->blocks->value(i) != cache->blocks->value(i);
}

void Map::cacheBorder() {
    cached_border = new Blockdata;
    for (int i = 0; i < border->blocks->length(); i++) {
        Block block = border->blocks->value(i);
        cached_border->blocks->append(block);
    }
}

void Map::cacheBlockdata() {
    cached_blockdata = new Blockdata;
    for (int i = 0; i < blockdata->blocks->length(); i++) {
        Block block = blockdata->blocks->value(i);
        cached_blockdata->blocks->append(block);
    }
}

void Map::cacheCollision() {
    cached_collision = new Blockdata;
    for (int i = 0; i < blockdata->blocks->length(); i++) {
        Block block = blockdata->blocks->value(i);
        cached_collision->blocks->append(block);
    }
}

QPixmap Map::renderCollision() {
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
    QPainter painter(&collision_image);
    for (int i = 0; i < blockdata->blocks->length(); i++) {
        if (!blockChanged(i, cached_collision)) {
            continue;
        }
        changed_any = true;
        Block block = blockdata->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile);
        QImage collision_metatile_image = getCollisionMetatileImage(block);
        QImage elevation_metatile_image = getElevationMetatileImage(block);
        int map_y = i / width_;
        int map_x = i % width_;
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

QPixmap Map::render() {
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
    QPainter painter(&image);
    for (int i = 0; i < blockdata->blocks->length(); i++) {
        if (!blockChanged(i, cached_blockdata)) {
            continue;
        }
        changed_any = true;
        Block block = blockdata->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile);
        int map_y = i / width_;
        int map_x = i % width_;
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
    if (border_image.isNull()) {
        border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    QPainter painter(&border_image);
    for (int i = 0; i < border->blocks->length(); i++) {
        if (!blockChanged(i, cached_border)) {
            continue;
        }
        changed_any = true;
        Block block = border->blocks->value(i);
        QImage metatile_image = getMetatileImage(block.tile);
        int map_y = i / width_;
        int map_x = i % width_;
        painter.drawImage(QPoint(map_x * 16, map_y * 16), metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBorder();
        border_pixmap = border_pixmap.fromImage(border_image);
    }
    return border_pixmap;
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
    drawSelection(paint_collision, width_, &painter);
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
    drawSelection(paint_elevation, width_, &painter);
    painter.end();
    return QPixmap::fromImage(image);
}

void Map::drawSelection(int i, int w, QPainter *painter) {
    int x = i % w;
    int y = i / w;
    painter->save();
    painter->setPen(QColor(0xff, 0xff, 0xff));
    painter->drawRect(x * 16, y * 16, 15, 15);
    painter->setPen(QColor(0, 0, 0));
    painter->drawRect(x * 16 - 1, y * 16 - 1, 17, 17);
    painter->drawRect(x * 16 + 1, y * 16 + 1, 13, 13);
    painter->restore();
}

QPixmap Map::renderMetatiles() {
    int primary_length = tileset_primary->metatiles->length();
    int length_ = primary_length + tileset_secondary->metatiles->length();
    int width_ = 8;
    int height_ = length_ / width_;
    QImage image(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
    QPainter painter(&image);
    for (int i = 0; i < length_; i++) {
        uint tile = i;
        if (i >= primary_length) {
            tile += 0x200 - primary_length;
        }
        QImage metatile_image = getMetatileImage(tile);
        int map_y = i / width_;
        int map_x = i % width_;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, metatile_image);
    }

    drawSelection(paint_tile, width_, &painter);

    painter.end();
    return QPixmap::fromImage(image);
}

Block* Map::getBlock(int x, int y) {
    if (x >= 0 && x < getWidth())
    if (y >= 0 && y < getHeight()) {
        int i = y * getWidth() + x;
        return new Block(blockdata->blocks->value(i));
    }
    return NULL;
}

void Map::_setBlock(int x, int y, Block block) {
    int i = y * getWidth() + x;
    blockdata->blocks->replace(i, block);
}

void Map::_floodFill(int x, int y, uint tile) {
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
            uint old_tile = block->tile;
            if (old_tile == tile) {
                continue;
            }
            block->tile = tile;
            _setBlock(x, y, *block);
            if ((block = getBlock(x + 1, y)) && block->tile == old_tile) {
                todo.append(QPoint(x + 1, y));
            }
            if ((block = getBlock(x - 1, y)) && block->tile == old_tile) {
                todo.append(QPoint(x - 1, y));
            }
            if ((block = getBlock(x, y + 1)) && block->tile == old_tile) {
                todo.append(QPoint(x, y + 1));
            }
            if ((block = getBlock(x, y - 1)) && block->tile == old_tile) {
                todo.append(QPoint(x, y - 1));
            }
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
    Blockdata *commit = history.pop();
    if (commit != NULL) {
        blockdata->copyFrom(commit);
    }
}

void Map::redo() {
    Blockdata *commit = history.next();
    if (commit != NULL) {
        blockdata->copyFrom(commit);
    }
}

void Map::commit() {
    Blockdata* commit = blockdata->copy();
    history.push(commit);
}

void Map::setBlock(int x, int y, Block block) {
    Block *old_block = getBlock(x, y);
    if (old_block && (*old_block) != block) {
        _setBlock(x, y, block);
        commit();
    }
}

void Map::floodFill(int x, int y, uint tile) {
    Block *block = getBlock(x, y);
    if (block && block->tile != tile) {
        _floodFill(x, y, tile);
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
