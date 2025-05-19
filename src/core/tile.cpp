#include "tile.h"
#include "project.h"
#include "bitpacker.h"

// Upper limit for raw value (i.e., uint16_t max).
const uint16_t Tile::maxValue = 0xFFFF;

// At the moment these are fixed, and not exposed to the user.
// We're only using them for convenience when converting between raw values.
// The actual job of clamping Tile's members to correct values is handled by the widths in the bit field.
const BitPacker bitsTileId = BitPacker(0x03FF);
const BitPacker bitsXFlip = BitPacker(0x0400);
const BitPacker bitsYFlip = BitPacker(0x0800);
const BitPacker bitsPalette = BitPacker(0xF000);

 Tile::Tile() :
        tileId(0),
        xflip(false),
        yflip(false),
        palette(0)
    {  }

 Tile::Tile(uint16_t tileId, uint16_t xflip, uint16_t yflip, uint16_t palette) :
        tileId(tileId),
        xflip(xflip),
        yflip(yflip),
        palette(palette)
    {  }

 Tile::Tile(uint16_t raw) :
        tileId(bitsTileId.unpack(raw)),
        xflip(bitsXFlip.unpack(raw)),
        yflip(bitsYFlip.unpack(raw)),
        palette(bitsPalette.unpack(raw))
    {  }

uint16_t Tile::rawValue() const {
    return bitsTileId.pack(this->tileId)
         | bitsXFlip.pack(this->xflip)
         | bitsYFlip.pack(this->yflip)
         | bitsPalette.pack(this->palette);
}

Qt::Orientations Tile::orientation() const {
    return Util::getOrientation(this->xflip, this->yflip);
}

void Tile::flip(QImage *image) const {
    if (!image) return;

    // QImage::flip was introduced in 6.9.0 to replace QImage::mirrored.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
    image->flip(this->orientation());
#else
    *image = image->mirrored(this->xflip, this->yflip);
#endif

}

int Tile::getIndexInTileset(int tileId) {
    if (tileId < Project::getNumTilesPrimary()) {
        return tileId;
    } else {
        return tileId - Project::getNumTilesPrimary();
    }
}
