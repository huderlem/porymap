#ifndef CITYMAPPIXMAPITEM_H
#define CITYMAPPIXMAPITEM_H

#include "tilemaptileselector.h"
#include <QGraphicsPixmapItem>
#include <QByteArray>

class CityMapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    CityMapPixmapItem(QString fname, TilemapTileSelector *tile_selector) {
        this->file = fname;
        this->tile_selector = tile_selector;
        setAcceptHoverEvents(true);
        init();
    }
    TilemapTileSelector *tile_selector;

    QString file;

    // TODO: make private and use access functions
    int width;
    int height;

    QByteArray data;
    
    void init();
    void save();
    void create(QString);
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void draw();
    int getIndexAt(int, int);

//private:
//    int width;
//    int height;

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, CityMapPixmapItem *);
    void hoveredRegionMapTileChanged(int x, int y);
    void hoveredRegionMapTileCleared();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
};

#endif // CITYMAPPIXMAPITEM_H
