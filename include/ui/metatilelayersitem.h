#ifndef METATILELAYERSITEM_H
#define METATILELAYERSITEM_H

#include "tileset.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class MetatileLayersItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    MetatileLayersItem(Metatile *metatile, Tileset *primaryTileset, Tileset *secondaryTileset) {
        this->metatile = metatile;
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
    }
    void draw();
    void setTilesets(Tileset*, Tileset*);
    void setMetatile(Metatile*);
private:
    Metatile* metatile;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
signals:
    void tileChanged(int);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
};

#endif // METATILELAYERSITEM_H
