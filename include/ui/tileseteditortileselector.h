#ifndef TILESETEDITORTILESELECTOR_H
#define TILESETEDITORTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class TilesetEditorTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorTileSelector(Tileset *primaryTileset, Tileset *secondaryTileset, bool isTripleLayer)
        : SelectablePixmapItem(16, 16, isTripleLayer ? 6 : 4, 2) {
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->numTilesWide = 16;
        this->paletteId = 0;
        this->xFlip = false;
        this->yFlip = false;
        this->paletteChanged = false;
        setAcceptHoverEvents(true);
    }
    QPoint getSelectionDimensions();
    void draw();
    void select(uint16_t metatileId);
    void highlight(uint16_t metatileId);
    void setTilesets(Tileset*, Tileset*);
    void setPaletteId(int);
    void setTileFlips(bool, bool);
    QList<Tile> getSelectedTiles();
    void setExternalSelection(int, int, QList<Tile>, QList<int>);
    QPoint getTileCoordsOnWidget(uint16_t);
    QImage buildPrimaryTilesIndexedImage();
    QImage buildSecondaryTilesIndexedImage();

    QVector<uint16_t> usedTiles;
    bool showUnused = false;

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
    QList<int> externalSelectedPos;

    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QList<uint16_t> selectedTiles;
    int numTilesWide;
    int paletteId;
    bool xFlip;
    bool yFlip;
    bool paletteChanged;
    void updateSelectedTiles();
    uint16_t getTileId(int x, int y);
    QPoint getTileCoords(uint16_t);
    QList<QRgb> getCurPaletteTable();
    QList<Tile> buildSelectedTiles(int, int, QList<Tile>);

    void drawUnused();

signals:
    void hoveredTileChanged(uint16_t);
    void hoveredTileCleared();
    void selectedTilesChanged();
};

#endif // TILESETEDITORTILESELECTOR_H
