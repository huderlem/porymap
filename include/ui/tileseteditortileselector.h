#ifndef TILESETEDITORTILESELECTOR_H
#define TILESETEDITORTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class TilesetEditorTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorTileSelector(Tileset *primaryTileset, Tileset *secondaryTileset): SelectablePixmapItem(16, 16, 4, 2) {
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->numTilesWide = 16;
        this->paletteId = 0;
        this->xFlip = false;
        this->yFlip = false;
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    void select(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*);
    void setPaletteId(int);
    void setTileFlips(bool, bool);
    QList<Tile> getSelectedTiles();
    void setExternalSelection(int, int, QList<Tile>);
    QPoint getTileCoordsOnWidget(uint16_t);
    QImage buildPrimaryTilesIndexedImage();
    QImage buildSecondaryTilesIndexedImage();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    bool externalSelection;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<Tile> externalSelectedTiles;

    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QList<uint16_t> selectedTiles;
    int numTilesWide;
    int paletteId;
    bool xFlip;
    bool yFlip;
    void updateSelectedTiles();
    uint16_t getTileId(int x, int y);
    QPoint getTileCoords(uint16_t);
    QList<QRgb> getCurPaletteTable();

signals:
    void hoveredTileChanged(uint16_t);
    void hoveredTileCleared();
    void selectedTilesChanged();
};

#endif // TILESETEDITORTILESELECTOR_H
