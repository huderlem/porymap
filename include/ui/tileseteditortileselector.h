#ifndef TILESETEDITORTILESELECTOR_H
#define TILESETEDITORTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "tileset.h"

class TilesetEditorTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilesetEditorTileSelector(Tileset *primaryTileset, Tileset *secondaryTileset)
        : SelectablePixmapItem(16, 16, Metatile::tileWidth(), Metatile::tileWidth()) {
        this->primaryTileset = primaryTileset;
        this->secondaryTileset = secondaryTileset;
        this->numTilesWide = 16;
        this->paletteId = 0;
        this->xFlip = false;
        this->yFlip = false;
        this->paletteChanged = false;
        setAcceptHoverEvents(true);
    }
    QSize getSelectionDimensions() const override;
    void setMaxSelectionSize(int width, int height) override;
    void draw() override;
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
    bool showDivider = false;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;

private:
    QPixmap basePixmap;
    bool externalSelection;
    int externalSelectionWidth;
    int externalSelectionHeight;
    QList<Tile> externalSelectedTiles;
    QList<int> externalSelectedPos;
    QPoint prevCellPos = QPoint(-1,-1);

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
    QImage buildImage(int tileIdStart, int numTiles);
    void updateBasePixmap();
    void drawUnused();

signals:
    void hoveredTileChanged(uint16_t);
    void hoveredTileCleared();
    void selectedTilesChanged();
};

#endif // TILESETEDITORTILESELECTOR_H
