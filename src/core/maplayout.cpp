#include "maplayout.h"

#include <QRegularExpression>

#include "scripting.h"
#include "imageproviders.h"
#include "utility.h"

QList<int> Layout::s_globalMetatileLayerOrder;
QList<float> Layout::s_globalMetatileLayerOpacity;

Layout::Layout(const Layout &other) : Layout() {
    copyFrom(&other);
}

Layout *Layout::copy() const {
    Layout *layout = new Layout;
    layout->copyFrom(this);
    return layout;
}

void Layout::copyFrom(const Layout *other) {
    this->id = other->id;
    this->name = other->name;
    this->width = other->width;
    this->height = other->height;
    this->border_width = other->border_width;
    this->border_height = other->border_height;
    this->border_path = other->border_path;
    this->blockdata_path = other->blockdata_path;
    this->tileset_primary_label = other->tileset_primary_label;
    this->tileset_secondary_label = other->tileset_secondary_label;
    this->tileset_primary = other->tileset_primary;
    this->tileset_secondary = other->tileset_secondary;
    this->blockdata = other->blockdata;
    this->border = other->border;
    this->customData = other->customData;
}

QString Layout::layoutConstantFromName(const QString &name) {
    return projectConfig.getIdentifier(ProjectIdentifier::define_layout_prefix) + Util::toDefineCase(name);
}

Layout::Settings Layout::settings() const {
    Layout::Settings settings;
    settings.id = this->id;
    settings.name = this->name;
    settings.width = this->width;
    settings.height = this->height;
    settings.borderWidth = this->border_width;
    settings.borderHeight = this->border_height;
    settings.primaryTilesetLabel = this->tileset_primary_label;
    settings.secondaryTilesetLabel = this->tileset_secondary_label;
    return settings;
}

bool Layout::isWithinBounds(int x, int y) const {
    return (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight());
}

bool Layout::isWithinBounds(const QPoint &pos) const {
    return isWithinBounds(pos.x(), pos.y());
}

bool Layout::isWithinBounds(const QRect &rect) const {
    return rect.left() >= 0 && rect.right() < this->getWidth() && rect.top() >= 0 && rect.bottom() < this->getHeight();
}

bool Layout::isWithinBorderBounds(int x, int y) const {
    return (x >= 0 && x < this->getBorderWidth() && y >= 0 && y < this->getBorderHeight());
}

// Calculate the distance away from the layout's edge that we need to start drawing border blocks.
// We need to fulfill two requirements here:
// - We should draw enough to fill the player's in-game view
// - The value should be some multiple of the border's dimension
//   (otherwise the border won't be positioned the same as it would in-game).
int Layout::getBorderDrawDistance(int dimension, qreal minimum) {
    if (dimension >= minimum)
        return dimension;

    // Get first multiple of dimension >= the minimum
    return dimension * qCeil(minimum / qMax(dimension, 1));
}
QMargins Layout::getBorderMargins() const {
    QMargins minimum = Project::getMetatileViewDistance();
    QMargins distance;
    distance.setTop(getBorderDrawDistance(this->border_height, minimum.top()));
    distance.setBottom(getBorderDrawDistance(this->border_height, minimum.bottom()));
    distance.setLeft(getBorderDrawDistance(this->border_width, minimum.left()));
    distance.setRight(getBorderDrawDistance(this->border_width, minimum.right()));
    return distance;
}

// Get a rectangle that represents (in pixels) the layout's map area and the visible area of its border.
// At maximum, this is equal to the map size plus the border margins.
// If the border is large (and so beyond player the view) it may be smaller than that.
QRect Layout::getVisibleRect() const {
    QRect area = QRect(0, 0, this->width * 16, this->height * 16);
    return area += (Project::getMetatileViewDistance() * 16);
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

bool Layout::layoutBlockChanged(int i, const Blockdata &curData, const Blockdata &cache) {
    if (cache.length() <= i)
        return true;
    if (curData.length() <= i)
        return true;

    return curData.at(i) != cache.at(i);
}

uint16_t Layout::getBorderMetatileId(int x, int y) {
    int i = y * getBorderWidth() + x;
    return this->border[i].metatileId();
}

void Layout::setBorderMetatileId(int x, int y, uint16_t metatileId, bool enableScriptCallback) {
    int i = y * getBorderWidth() + x;
    if (i < this->border.size()) {
        uint16_t prevMetatileId = this->border[i].metatileId();
        this->border[i].setMetatileId(metatileId);
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
                Scripting::cb_BorderMetatileChanged(i % width, i / width, prevBlock.metatileId(), newBlock.metatileId());
        }
    }
}

void Layout::setDimensions(int newWidth, int newHeight, bool setNewBlockdata) {
    if (setNewBlockdata) {
        setNewDimensionsBlockdata(newWidth, newHeight);
    }
    this->width = newWidth;
    this->height = newHeight;
    emit dimensionsChanged(QSize(this->width, this->height));
}

void Layout::adjustDimensions(const QMargins &margins, bool setNewBlockdata) {
    int oldWidth = this->width;
    int oldHeight = this->height;
    this->width = oldWidth + margins.left() + margins.right();
    this->height = oldHeight + margins.top() + margins.bottom();

    if (setNewBlockdata) {
        // Fill new blockdata
        Blockdata newBlockdata;
        for (int y = 0; y < this->height; y++)
        for (int x = 0; x < this->width; x++) {
            if ((x < margins.left()) || (x >= this->width - margins.right()) || (y < margins.top()) || (y >= this->height - margins.bottom())) {
                newBlockdata.append(0);
            } else {
                int index = (y - margins.top()) * oldWidth + (x - margins.left());
                newBlockdata.append(this->blockdata.value(index));
            }
        }
        this->blockdata = newBlockdata;
    }

    Scripting::cb_MapResized(oldWidth, oldHeight, margins);
    emit dimensionsChanged(QSize(this->width, this->height));
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

        uint old_coll = block.collision();
        uint old_elev = block.elevation();
        if (old_coll == collision && old_elev == elevation) {
            continue;
        }

        block.setCollision(collision);
        block.setElevation(elevation);
        setBlock(x, y, block, true);
        if (getBlock(x + 1, y, &block) && block.collision() == old_coll && block.elevation() == old_elev) {
            todo.append(QPoint(x + 1, y));
        }
        if (getBlock(x - 1, y, &block) && block.collision() == old_coll && block.elevation()== old_elev) {
            todo.append(QPoint(x - 1, y));
        }
        if (getBlock(x, y + 1, &block) && block.collision() == old_coll && block.elevation() == old_elev) {
            todo.append(QPoint(x, y + 1));
        }
        if (getBlock(x, y - 1, &block) && block.collision() == old_coll && block.elevation() == old_elev) {
            todo.append(QPoint(x, y - 1));
        }
    }
}

void Layout::floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation) {
    Block block;
    if (getBlock(x, y, &block) && (block.collision() != collision || block.elevation() != elevation)) {
        _floodFillCollisionElevation(x, y, collision, elevation);
    }
}

void Layout::magicFillCollisionElevation(int initialX, int initialY, uint16_t collision, uint16_t elevation) {
    Block block;
    if (getBlock(initialX, initialY, &block) && (block.collision() != collision || block.elevation() != elevation)) {
        uint old_coll = block.collision();
        uint old_elev = block.elevation();

        for (int y = 0; y < getHeight(); y++) {
            for (int x = 0; x < getWidth(); x++) {
                if (getBlock(x, y, &block) && block.collision() == old_coll && block.elevation() == old_elev) {
                    block.setCollision(collision);
                    block.setElevation(elevation);
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
    if (this->image.isNull() || this->image.width() != width_ * 16 || this->image.height() != height_ * 16) {
        this->image = QImage(width_ * 16, height_ * 16, QImage::Format_RGBA8888);
        changed_any = true;
    }
    if (this->blockdata.isEmpty() || !width_ || !height_) {
        this->pixmap = this->pixmap.fromImage(this->image);
        return this->pixmap;
    }

    // There are a lot of external changes that can invalidate a general metatile image cache.
    // However, during a single pass at rendering the layout there shouldn't be any changes to
    // the tiles, tileset palettes, or metatile layer order/opacity, and layouts often have
    // many repeated metatile IDs, so we create a cache for each request to render the layout.
    QHash<uint16_t, QImage> imageCache;

    QPainter painter(&this->image);
    for (int i = 0; i < this->blockdata.length(); i++) {
        if (!ignoreCache && !layoutBlockChanged(i, this->blockdata, this->cached_blockdata)) {
            continue;
        }
        int map_y = width_ ? i / width_ : 0;
        int map_x = width_ ? i % width_ : 0;
        if (bounds.isValid() && !bounds.contains(map_x, map_y)) {
            continue;
        }

        uint16_t metatileId = this->blockdata.at(i).metatileId();
        QImage metatileImage;
        if (imageCache.contains(metatileId)) {
            metatileImage = imageCache.value(metatileId);
        } else {
            metatileImage = getMetatileImage(
                metatileId,
                fromLayout ? fromLayout->tileset_primary   : this->tileset_primary,
                fromLayout ? fromLayout->tileset_secondary : this->tileset_secondary,
                metatileLayerOrder(),
                metatileLayerOpacity()
            );
            imageCache.insert(metatileId, metatileImage);
        }

        QPoint metatileOrigin = QPoint(map_x * 16, map_y * 16);
        painter.drawImage(metatileOrigin, metatileImage);
        changed_any = true;
    }
    painter.end();
    if (changed_any) {
        cacheBlockdata();
        this->pixmap = this->pixmap.fromImage(this->image);
    }

    return this->pixmap;
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
        if (!ignoreCache && !layoutBlockChanged(i, this->blockdata, this->cached_collision)) {
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
        if (!ignoreCache && (!border_resized && !layoutBlockChanged(i, this->border, this->cached_border))) {
            continue;
        }

        changed_any = true;
        Block block = this->border.at(i);
        uint16_t metatileId = block.metatileId();
        QImage metatile_image = getMetatileImage(metatileId, this);
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

QPixmap Layout::getLayoutItemPixmap() {
    return this->layoutItem ? this->layoutItem->pixmap() : QPixmap();
}

bool Layout::hasUnsavedChanges() const {
    return !this->editHistory.isClean() || this->hasUnsavedDataChanges || !this->newFolderPath.isEmpty();
}

bool Layout::save(const QString &root) {
    if (!this->newFolderPath.isEmpty()) {
        // Layout directory doesn't exist yet, create it now.
        const QString fullPath = QString("%1/%2").arg(root).arg(this->newFolderPath);
        if (!QDir::root().mkpath(fullPath)) {
            logError(QString("Failed to create directory for new layout: '%1'").arg(fullPath));
            return false;
        }
        this->newFolderPath = QString();
    }

    bool success = true;
    if (!saveBorder(root)) success = false;
    if (!saveBlockdata(root)) success = false;
    if (!success)
        return false;

    this->editHistory.setClean();
    this->hasUnsavedDataChanges = false;
    return true;
}

bool Layout::saveBorder(const QString &root) {
    QString path = QString("%1/%2").arg(root).arg(this->border_path);
    return writeBlockdata(path, this->border);
}

bool Layout::saveBlockdata(const QString &root) {
    QString path = QString("%1/%2").arg(root).arg(this->blockdata_path);
    return writeBlockdata(path, this->blockdata);
}

bool Layout::writeBlockdata(const QString &path, const Blockdata &blockdata) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Failed to write '%1' for %2: %3").arg(path).arg(this->name).arg(file.errorString()));
        return false;
    }

    QByteArray data = blockdata.serialize();
    file.write(data);
    return true;
}

bool Layout::loadBorder(const QString &root) {
    if (this->border_path.isEmpty()) {
        logError(QString("Failed to load border for %1: no path specified.").arg(this->name));
        return false;
    }

    QString error;
    QString path = QString("%1/%2").arg(root).arg(this->border_path);
    auto blockdata = readBlockdata(path, &error);
    if (!error.isEmpty()) {
        logError(QString("Failed to load border for %1 from '%2': %3").arg(this->name).arg(path).arg(error));
        return false;
    }

    // 0 is an expected border width/height that should be handled, GF used it for the RS layouts in FRLG
    if (this->border_width <= 0) {
        this->border_width = DEFAULT_BORDER_WIDTH;
    }
    if (this->border_height <= 0) {
        this->border_height = DEFAULT_BORDER_HEIGHT;
    }

    this->border = blockdata;
    this->lastCommitBlocks.border = blockdata;
    this->lastCommitBlocks.borderDimensions = QSize(this->border_width, this->border_height);

    int expectedSize = this->border_width * this->border_height;
    if (this->border.count() != expectedSize) {
        logWarn(QString("%1 border blockdata length %2 does not match dimensions %3x%4 (should be %5). Resizing border blockdata.")
                .arg(this->name)
                .arg(this->border.count())
                .arg(this->border_width)
                .arg(this->border_height)
                .arg(expectedSize));
        this->border.resize(expectedSize);
    }
    return true;
}

bool Layout::loadBlockdata(const QString &root) {
    if (this->blockdata_path.isEmpty()) {
        logError(QString("Failed to load blockdata for %1: no path specified.").arg(this->name));
        return false;
    }

    QString error;
    QString path = QString("%1/%2").arg(root).arg(this->blockdata_path);
    auto blockdata = readBlockdata(path, &error);
    if (!error.isEmpty()) {
        logError(QString("Failed to load blockdata for %1 from '%2': %3").arg(this->name).arg(path).arg(error));
        return false;
    }

    this->blockdata = blockdata;
    this->lastCommitBlocks.blocks = blockdata;
    this->lastCommitBlocks.layoutDimensions = QSize(this->width, this->height);

    int expectedSize = this->width * this->height;
    if (expectedSize <= 0) {
        logError(QString("Failed to load blockdata for %1: invalid dimensions %2x%3").arg(this->name).arg(this->width).arg(this->height));
        return false;
    }
    if (this->blockdata.count() != expectedSize) {
        logWarn(QString("%1 blockdata length %2 does not match dimensions %3x%4 (should be %5). Resizing blockdata.")
                .arg(this->name)
                .arg(this->blockdata.count())
                .arg(this->width)
                .arg(this->height)
                .arg(expectedSize));
        this->blockdata.resize(expectedSize);
    }
    return true;
}

Blockdata Layout::readBlockdata(const QString &path, QString *error) {
    Blockdata blockdata;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        for (int i = 0; (i + 1) < data.length(); i += 2) {
            uint16_t word = static_cast<uint16_t>((data[i] & 0xff) + ((data[i + 1] & 0xff) << 8));
            blockdata.append(word);
        }
    } else {
        if (error) *error = file.errorString();
    }

    return blockdata;
}
