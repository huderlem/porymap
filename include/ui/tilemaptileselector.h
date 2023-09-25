#pragma once
#ifndef TILEMAPTILESELECTOR_H
#define TILEMAPTILESELECTOR_H

#include "selectablepixmapitem.h"
#include "paletteutil.h"
#include "imageproviders.h"

#include <memory>
using std::shared_ptr;

enum class TilemapFormat { Plain, BPP_4, BPP_8 };

class TilemapTile {
    unsigned raw_ = 0;
    unsigned id_ = 0;
    bool hFlip_ = false;
    bool vFlip_ = false;
    int palette_ = 0;

protected:
    TilemapTile(unsigned raw, unsigned id, bool hFlip, bool vFlip, int palette) : raw_(raw), id_(id), hFlip_(hFlip), vFlip_(vFlip), palette_(palette) {}
    virtual ~TilemapTile() {}

public:
    TilemapTile()=default;

    TilemapTile(TilemapTile &other) {
        this->raw_ = other.raw();
        this->id_ = other.id();
        this->hFlip_ = other.hFlip();
        this->vFlip_ = other.vFlip();
        this->palette_ = other.palette();
    }

    TilemapTile &operator=(const TilemapTile &other) {
        this->raw_ = other.raw();
        this->id_ = other.id();
        this->hFlip_ = other.hFlip();
        this->vFlip_ = other.vFlip();
        this->palette_ = other.palette();
        return *this;
    }

    virtual void copy(TilemapTile &other) {
        this->raw_ = other.raw();
        this->id_ = other.id();
        this->hFlip_ = other.hFlip();
        this->vFlip_ = other.vFlip();
        this->palette_ = other.palette();
    }

    virtual unsigned raw() const { return this->raw_; }
    virtual unsigned id() const { return this->id_; }
    virtual bool hFlip() const { return this->hFlip_; }
    virtual bool vFlip() const { return this->vFlip_; }
    virtual int palette() const { return this->palette_; }

    virtual void setId(unsigned id) { this->id_ = id; }
    virtual void setHFlip(bool hFlip) { this->hFlip_ = hFlip; }
    virtual void setVFlip(bool vFlip) { this->vFlip_ = vFlip; }
    virtual void setPalette(int palette) { this->palette_ = palette; }

    bool operator==(const TilemapTile& other) {
        return (this->raw() == other.raw());
    }

    virtual QString info() const {
        return QString("Tile: 0x") + QString("%1  ").arg(this->id(), 4, 16, QChar('0')).toUpper();
    }
};

class PlainTile : public TilemapTile {
public:
    PlainTile(unsigned raw) : TilemapTile(raw, raw, false, false, 0) {}

    ~PlainTile() {}

    virtual unsigned raw () const override { return id(); }
};

class BPP4Tile : public TilemapTile {
public:
    BPP4Tile(unsigned raw) : TilemapTile(
            raw,
            raw & 0x3ff, // tileId
            !!(raw & 0x0400), // hFlip
            !!(raw & 0x0800), // vFlip
            (raw & 0xf000) >> 12 // palette
        ) {}

    ~BPP4Tile() {}

    virtual unsigned raw () const override {
        return (id()) | (hFlip() << 10) | (vFlip() << 11) | (palette() << 12);
    }

    virtual QString info() const override {
        return TilemapTile::info() + QString("hFlip: %1  vFlip: %2  palette: %3").arg(this->hFlip()).arg(this->vFlip()).arg(this->palette());
    }
};

class BPP8Tile : public TilemapTile {
public:
    BPP8Tile(unsigned raw) : TilemapTile(
            raw,
            raw & 0x3ff, // tileId
            !!(raw & 0x0400), // hFlip
            !!(raw & 0x0800), // vFlip
            0            // palette
        ) {}

    ~BPP8Tile() {}

    virtual unsigned raw () const override {
        return (id()) | (hFlip() << 10) | (vFlip() << 11);
    }

    virtual QString info() const override {
        return TilemapTile::info() + QString("hFlip: %1  vFlip: %2").arg(this->hFlip()).arg(this->vFlip());
    }
};

class TilemapTileSelector: public SelectablePixmapItem {
    Q_OBJECT
public:
    TilemapTileSelector(QString tilesetFilepath, TilemapFormat format, QString palFilepath): SelectablePixmapItem(8, 8, 1, 1) {
        this->tileset = QImage(tilesetFilepath);
        this->format = format;
        if (this->tileset.format() == QImage::Format::Format_Indexed8 && this->format == TilemapFormat::BPP_4) {
            flattenTo4bppImage(&this->tileset);
        }
        bool err;
        if (!palFilepath.isEmpty()) {
            this->palette = PaletteUtil::parse(palFilepath, &err);
        }
        this->setPixmap(QPixmap::fromImage(this->tileset));
        this->numTilesWide = this->tileset.width() / 8;
        this->selectedTile = 0x00;
        setAcceptHoverEvents(true);
    }
    void draw();
    QImage setPalette(int index);

    int pixelWidth;
    int pixelHeight;

    void select(unsigned tileId);
    unsigned selectedTile = 0;

    void selectVFlip(bool hFlip) { this->tile_hFlip = hFlip; }
    bool tile_hFlip = false;

    void selectHFlip(bool vFlip) { this->tile_vFlip = vFlip; }
    bool tile_vFlip = false;

    void selectPalette(int palette) {
        this->tile_palette = palette;
        this->draw();
    }
    int tile_palette = 0;

    QImage tileset;
    TilemapFormat format = TilemapFormat::Plain;
    QList<QRgb> palette;
    QImage tileImg(shared_ptr<TilemapTile> tile);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);

private:
    int numTilesWide;
    size_t numTiles;
    void updateSelectedTile();
    unsigned getTileId(int x, int y);
    QPoint getTileIdCoords(unsigned);

signals:
    void hoveredTileChanged(unsigned);
    void hoveredTileCleared();
    void selectedTileChanged(unsigned);
};

#endif // TILEMAPTILESELECTOR_H
