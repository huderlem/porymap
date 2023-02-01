#pragma once
#ifndef MAPLAYOUT_H
#define MAPLAYOUT_H

#include "blockdata.h"
#include "tileset.h"
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QUndoStack>

class Map;
class LayoutPixmapItem;
class CollisionPixmapItem;
class BorderMetatilesPixmapItem;

class Layout : public QObject {
    Q_OBJECT
public:
    Layout() {}

    static QString layoutConstantFromName(QString mapName);

    /// !TODO
    /* NEW */
    QList<Map *> maps;

    QString id;
    QString name;

    int width;
    int height;
    int border_width;
    int border_height;

    QString border_path;
    QString blockdata_path;

    QString tileset_primary_label;
    QString tileset_secondary_label;

    Tileset *tileset_primary = nullptr;
    Tileset *tileset_secondary = nullptr;

    Blockdata blockdata;

    QImage image;
    QPixmap pixmap;
    QImage border_image;
    QPixmap border_pixmap;
    QImage collision_image;
    QPixmap collision_pixmap;

    Blockdata border;
    Blockdata cached_blockdata;
    Blockdata cached_collision;
    Blockdata cached_border;
    struct {
        Blockdata blocks;
        QSize mapDimensions;
        Blockdata border;
        QSize borderDimensions;
    } lastCommitBlocks; // to track map changes

    QList<int> metatileLayerOrder;
    QList<float> metatileLayerOpacity;

    LayoutPixmapItem *layoutItem = nullptr;
    CollisionPixmapItem *collisionItem = nullptr;
    BorderMetatilesPixmapItem *borderItem = nullptr;

    QUndoStack editHistory;

public:
    int getWidth();
    int getHeight();
    int getBorderWidth();
    int getBorderHeight();

    bool isWithinBounds(int x, int y) {
        return (x >= 0 && x < this->getWidth() && y >= 0 && y < this->getHeight());
    }

    bool getBlock(int x, int y, Block *out);
    void setBlock(int x, int y, Block block, bool enableScriptCallback = false);
    void setBlockdata(Blockdata blockdata, bool enableScriptCallback = false);

    void setDimensions(int newWidth, int newHeight, bool setNewBlockdata = true, bool enableScriptCallback = false);
    void setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata = true, bool enableScriptCallback = false);

    void cacheBlockdata();
    void cacheCollision();
    void clearBorderCache();
    void cacheBorder();

    bool layoutBlockChanged(int i, const Blockdata &cache);

    uint16_t getBorderMetatileId(int x, int y);
    void setBorderMetatileId(int x, int y, uint16_t metatileId, bool enableScriptCallback = false);
    void setBorderBlockData(Blockdata blockdata, bool enableScriptCallback = false);

    void floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void _floodFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);
    void magicFillCollisionElevation(int x, int y, uint16_t collision, uint16_t elevation);

    QPixmap render(bool ignoreCache = false, Layout *fromLayout = nullptr, QRect bounds = QRect(0, 0, -1, -1));
    QPixmap renderCollision(bool ignoreCache);
    // QPixmap renderConnection(MapConnection, Layout *);
    QPixmap renderBorder(bool ignoreCache = false);

    void setLayoutItem(LayoutPixmapItem *item) { layoutItem = item; }
    void setCollisionItem(CollisionPixmapItem *item) { collisionItem = item; }
    void setBorderItem(BorderMetatilesPixmapItem *item) { borderItem = item; }

private:
    void setNewDimensionsBlockdata(int newWidth, int newHeight);
    void setNewBorderDimensionsBlockdata(int newWidth, int newHeight);

signals:
    void layoutChanged(Layout *layout);
    void modified();
    void layoutDimensionsChanged(const QSize &size);
    void needsRedrawing();
};

using MapLayout = Layout;

#endif // MAPLAYOUT_H
