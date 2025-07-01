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
    Layout() {};
    Layout(const Layout &other);

    static QString layoutConstantFromName(const QString &name);

    bool hasUnsavedDataChanges = false;

    QString id;
    QString name;
    QString newFolderPath;

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

    QJsonObject customData;

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
        QSize layoutDimensions;
        Blockdata border;
        QSize borderDimensions;
    } lastCommitBlocks; // to track map changes

    void setMetatileLayerOrder(const QList<int> &layerOrder) { m_metatileLayerOrder = layerOrder; }
    QList<int> metatileLayerOrder() const;
    static void setDefaultMetatileLayerOrder(const QList<int> &layerOrder) { s_defaultMetatileLayerOrder = layerOrder; }
    static QList<int> defaultMetatileLayerOrder();

    void setMetatileLayerOpacity(const QList<float> &layerOpacity) { m_metatileLayerOpacity = layerOpacity; }
    QList<float> metatileLayerOpacity() const;
    static void setDefaultMetatileLayerOpacity(const QList<float> &layerOpacity) { s_defaultMetatileLayerOpacity = layerOpacity; }
    static QList<float> defaultMetatileLayerOpacity();

    LayoutPixmapItem *layoutItem = nullptr;
    CollisionPixmapItem *collisionItem = nullptr;
    BorderMetatilesPixmapItem *borderItem = nullptr;

    QUndoStack editHistory;

    // to simplify new layout settings transfer between functions
    struct Settings {
        QString id;
        QString name;
        // The name of a new layout's folder in `data/layouts/` is not always the same as the layout's name
        // (e.g. the majority of the default layouts use the name of their associated map).
        QString folderName;
        int width;
        int height;
        int borderWidth;
        int borderHeight;
        QString primaryTilesetLabel;
        QString secondaryTilesetLabel;
    };
    Settings settings() const;

    Layout *copy() const;
    void copyFrom(const Layout *other);

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getBorderWidth() const { return border_width; }
    int getBorderHeight() const { return border_height; }
    QMargins getBorderMargins() const;
    QRect getVisibleRect() const;

    bool isWithinBounds(int x, int y) const;
    bool isWithinBounds(const QPoint &pos) const;
    bool isWithinBounds(const QRect &rect) const;
    bool isWithinBorderBounds(int x, int y) const;

    bool getBlock(int x, int y, Block *out);
    void setBlock(int x, int y, Block block, bool enableScriptCallback = false);
    void setBlockdata(Blockdata blockdata, bool enableScriptCallback = false);

    void adjustDimensions(const QMargins &margins, bool setNewBlockdata = true);
    void setDimensions(int newWidth, int newHeight, bool setNewBlockdata = true);
    void setBorderDimensions(int newWidth, int newHeight, bool setNewBlockdata = true, bool enableScriptCallback = false);

    void cacheBlockdata();
    void cacheCollision();
    void clearBorderCache();
    void cacheBorder();

    bool hasUnsavedChanges() const;

    bool save(const QString &root);
    bool saveBorder(const QString &root);
    bool saveBlockdata(const QString &root);

    bool loadBorder(const QString &root);
    bool loadBlockdata(const QString &root);

    bool layoutBlockChanged(int i, const Blockdata &curData, const Blockdata &cache);

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

    QPixmap getLayoutItemPixmap();

    void setLayoutItem(LayoutPixmapItem *item) { layoutItem = item; }
    void setCollisionItem(CollisionPixmapItem *item) { collisionItem = item; }
    void setBorderItem(BorderMetatilesPixmapItem *item) { borderItem = item; }

    bool metatileIsValid(uint16_t metatileId) { return Tileset::metatileIsValid(metatileId, this->tileset_primary, this->tileset_secondary); }

private:
    void setNewDimensionsBlockdata(int newWidth, int newHeight);
    void setNewBorderDimensionsBlockdata(int newWidth, int newHeight);
    bool writeBlockdata(const QString &path, const Blockdata &blockdata) const;
    static Blockdata readBlockdata(const QString &path, QString *error);

    static int getBorderDrawDistance(int dimension, qreal minimum);

    QList<int> m_metatileLayerOrder;
    QList<float> m_metatileLayerOpacity;
    static QList<int> s_defaultMetatileLayerOrder;
    static QList<float> s_defaultMetatileLayerOpacity;

signals:
    void dimensionsChanged(const QSize &size);
    void needsRedrawing();
};

#endif // MAPLAYOUT_H
