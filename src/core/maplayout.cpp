#include "maplayout.h"

#include <QRegularExpression>

#include "scripting.h"
#include "imageproviders.h"

QString Layout::layoutConstantFromName(QString mapName) {
    // Transform map names of the form 'GraniteCave_B1F` into layout constants like 'LAYOUT_GRANITE_CAVE_B1F'.
    static const QRegularExpression caseChange("([a-z])([A-Z])");
    QString nameWithUnderscores = mapName.replace(caseChange, "\\1_\\2");
    QString withMapAndUppercase = "LAYOUT_" + nameWithUnderscores.toUpper();
    static const QRegularExpression underscores("_+");
    QString constantName = withMapAndUppercase.replace(underscores, "_");

    // Handle special cases.
    // SSTidal should be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

int Layout::getWidth() {
    return width;
}

int Layout::getHeight() {
    return height;
}

int Layout::getBorderWidth() {
    return border_width;
}

int Layout::getBorderHeight() {
    return border_height;
}

bool Layout::isWithinBounds(int x, int y) {
    return (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight());
}

bool Layout::isWithinBorderBounds(int x, int y) {
    return (x >= 0 && x < this->getBorderWidth() && y >= 0 && y < this->getBorderHeight());
}

bool Layout::getBlock(int x, int y, Block *out) {
    if (isWithinBounds(x, y)) {
        int i = y * getWidth() + x;
        *out = this->blockdata.value(i);
        return true;
    }
    return false;
}

void Layout::setBlock(int x, int y, Block block, bool enableScriptCallback) {
    if (!isWithinBounds(x, y)) return;
    int i = y * getWidth() + x;
    if (i < this->blockdata.size()) {
        Block prevBlock = this->blockdata.at(i);
        this->blockdata.replace(i, block);
        if (enableScriptCallback) {
            Scripting::cb_MetatileChanged(x, y, prevBlock, block);
        }
    }
}

void Layout::setBlockdata(Blockdata newBlockdata, bool enableScriptCallback) {
    int width = getWidth();
    int size = qMin(newBlockdata.size(), this->blockdata.size());
    for (int i = 0; i < size; i++) {
        Block prevBlock = this->blockdata.at(i);
        Block newBlock = newBlockdata.at(i);
        if (prevBlock != newBlock) {
            this->blockdata.replace(i, newBlock);
            if (enableScriptCallback)
                Scripting::cb_MetatileChanged(i % width, i / width, prevBlock, newBlock);
        }
    }
}

void Layout::clearBorderCache() {
    this->cached_border.clear();
}

void Layout::cacheBorder() {
    this->cached_border.clear();
    for (const auto &block : this->border)
        this->cached_border.append(block);
}

void Layout::cacheBlockdata() {
    this->cached_blockdata.clear();
    for (const auto &block : this->blockdata)
        this->cached_blockdata.append(block);
}

void Layout::cacheCollision() {
    this->cached_collision.clear();
    for (const auto &block : this->blockdata)
        this->cached_collision.append(block);
}

bool Layout::layoutBlockChanged(int i, const Blockdata &cache) {
    if (cache.length() <= i)
        return true;
    if (this->blockdata.length() <= i)
        return true;

    return this->blockdata.at(i) != cache.at(i);
}

uint16_t Layout::getBorderMetatileId(int x, int y) {
    int i = y * getBorderWidth() + x;
    return this->border[i].metatileId;
}

void Layout::setBorderMetatileId(int x, int y, uint16_t metatileId, bool enableScriptCallback) {
    int i = y * getBorderWidth() + x;
    if (i < this->border.size()) {
        uint16_t prevMetatileId = this->border[i].metatileId;
        this->border[i].metatileId = metatileId;
        if (prevMetatileId != metatileId && enableScriptCallback) {
            Scripting::cb_BorderMetatileChanged(x, y, prevMetatileId, metatileId);
        }
    }
}

void Layout::setBorderBlockData(Blockdata newBlockdata, bool enableScriptCallback) {
    int width = getBorderWidth();
    int size = qMin(newBlockdata.size(), this->border.size());
    for (int i = 0; i < size; i++) {
        Block prevBlock = this->border.at(i);
        Block newBlock = newBlockdata.at(i);
        if (prevBlock != newBlock) {
            this->border.replace(i, newBlock);
            if (enableScriptCallback)
                Scripting::cb_BorderMetatileChanged(i % width, i / width, prevBlock.metatileId, newBlock.metatileId);
        }
    }
}

void Layout::setDimensions(int newWidth, int newHeight, bool setNewBlockdata, bool enableScriptCallback) {
    if (setNewBlockdata) {
        setNewDimensionsBlockdata(newWidth, newHeight);
    }

    int oldWidth = this->width;
    int oldHeight = this->height;
    this->width = newWidth;
    this->height = newHeight;

    if (enableScriptCallback && (oldWidth != newWidth || oldHeight != newHeight)) {
        Scripting::cb_MapResized(oldWidth, oldHeight, newWidth, newHeight);
    }

    emit layoutChanged(this);
    emit layoutDimensionsChanged(QSize(getWidth(), getHeight()));
}

void Layout::setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata, bool enableScriptCallback) {
    if (setNewBlockdata) {
        setNewBorderDimensionsBlockdata(newWidth, newHeight);
    }

    int oldWidth = this->border_width;
    int oldHeight = this->border_height;
    this->border_width = newWidth;
    this->border_height = newHeight;

    if (enableScriptCallback && (oldWidth != newWidth || oldHeight != newHeight)) {
        Scripting::cb_BorderResized(oldWidth, oldHeight, newWidth, newHeight);
    }

    emit layoutChanged(this);
}

void Layout::setNewDimensionsBlockdata(int newWidth, int newHeight) {
    int oldWidth = getWidth();
    int oldHeight = getHeight();

    Blockdata newBlockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockdata.append(this->blockdata.value(index));
        } else {
            newBlockdata.append(0);
        }
    }

    this->blockdata = newBlockdata;
}

void Layout::setNewBorderDimensionsBlockdata(int newWidth, int newHeight) {
    int oldWidth = getBorderWidth();
    int oldHeight = getBorderHeight();

    Blockdata newBlockdata;

    for (int y = 0; y < newHeight; y++)
    for (int x = 0; x < newWidth; x++) {
        if (x < oldWidth && y < oldHeight) {
            int index = y * oldWidth + x;
            newBlockdata.append(this->border.value(index));
        } else {
            newBlockdata.append(0);
        }
    }

    this->border = newBlockdata;
}

void Layout::_floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
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

void Layout::floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    Block block;
    if (getBlock(x, y, &block) && (block.collision != collision || block.elevation != elevation)) {
        _floodFillCollisionElevation(x, y, collision, elevation);
    }
}

void Layout::magicFillCollisionElevation(int initialX, int initialY, uint16_t collision, uint16_t elevation) {
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

QPixmap Layout::render(bool ignoreCache, Layout *fromLayout, QRect bounds) {
    bool changed_any = false;
    int width_ = getWidth();
    int height_ = getHeight();
    if (image.isNull() || image.width() != width_ * 16 || image.height() != height_ * 16) {
        image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (this->blockdata.isEmpty() || !width_ || !height_) {
        pixmap = pixmap.fromImage(image);
        return pixmap;
    }

    QPainter painter(&image);
    for (int i = 0; i < this->blockdata.length(); i++) {
        if (!ignoreCache && !layoutBlockChanged(i, this->cached_blockdata)) {
            continue;
        }
        changed_any = true;
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        if (bounds.isValid() && !bounds.contains(map_x, map_y)) {
            continue;
        }
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        Block block = this->blockdata.at(i);
        QImage metatile_image = getMetatileImage(
            block.metatileId,
            fromLayout ? fromLayout->tileset_primary   : this->tileset_primary,
            fromLayout ? fromLayout->tileset_secondary : this->tileset_secondary,
            metatileLayerOrder,
            metatileLayerOpacity
        );
        painter.drawImage(metatile_origin, metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBlockdata();
        pixmap = pixmap.fromImage(image);
    }

    return pixmap;
}

QPixmap Layout::renderCollision(bool ignoreCache) {
    bool changed_any = false;
    int width_ = getWidth();
    int height_ = getHeight();
    if (collision_image.isNull() || collision_image.width() != width_ * 16 || collision_image.height() != height_ * 16) {
        collision_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (this->blockdata.isEmpty() || !width_ || !height_) {
        collision_pixmap = collision_pixmap.fromImage(collision_image);
        return collision_pixmap;
    }
    QPainter painter(&collision_image);
    for (int i = 0; i < this->blockdata.length(); i++) {
        if (!ignoreCache && !layoutBlockChanged(i, this->cached_collision)) {
            continue;
        }
        changed_any = true;
        Block block = this->blockdata.at(i);
        QImage collision_metatile_image = getCollisionMetatileImage(block);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        QPoint metatile_origin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatile_origin, collision_metatile_image);
    }
    painter.end();
    cacheCollision();
    if (changed_any) {
        collision_pixmap = collision_pixmap.fromImage(collision_image);
    }
    return collision_pixmap;
}

QPixmap Layout::renderBorder(bool ignoreCache) {
    bool changed_any = false, border_resized = false;
    int width_ = getBorderWidth();
    int height_ = getBorderHeight();
    if (this->border_image.isNull()) {
        this->border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (this->border_image.width() != width_ * 16 || this->border_image.height() != height_ * 16) {
        this->border_image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        border_resized = true;
    }
    if (this->border.isEmpty()) {
        this->border_pixmap = this->border_pixmap.fromImage(this->border_image);
        return this->border_pixmap;
    }
    QPainter painter(&this->border_image);
    for (int i = 0; i < this->border.length(); i++) {
        if (!ignoreCache && (!border_resized && !layoutBlockChanged(i, this->cached_border))) {
            continue;
        }

        changed_any = true;
        Block block = this->border.at(i);
        uint16_t metatileId = block.metatileId;
        QImage metatile_image = getMetatileImage(metatileId, this->tileset_primary, this->tileset_secondary, metatileLayerOrder, metatileLayerOpacity);
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        painter.drawImage(QPoint(map_x * 16, map_y * 16), metatile_image);
    }
    painter.end();
    if (changed_any) {
        cacheBorder();
        this->border_pixmap = this->border_pixmap.fromImage(this->border_image);
    }
    return this->border_pixmap;
}
