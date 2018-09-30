#ifndef TILESETEDITORTILESELECTOR_H
#define TILESETEDITORTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class TilesetEditorTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorTileSelector(Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16, 1, 1) {
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->numTilesWide = 16;
        this->paletteNum = 0;
        setAcceptHoverEvents(true);
    }
    void draw();
    void select(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*);
    void setPaletteNum(int);
    uint16_t getSelectedTile();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    uint16_t selectedTile;
    int numTilesWide;
    int paletteNum;
    void updateSelectedTile();
    uint16_t getTileId(int x, int y);
    QPoint getTileCoords(uint16_t);
    QList<QRgb> getCurPaletteTable();

signals:
    void hoveredTileChanged(uint16_t);
    void hoveredTileCleared();
    void selectedTileChanged(uint16_t);
};

#endif // TILESETEDITORTILESELECTOR_H
