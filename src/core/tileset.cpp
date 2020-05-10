#include "tileset.h"
#include "metatile.h"
#include "project.h"
#include "log.h"
#include "config.h"

#include <QPainter>
#include <QImage>

Tileset::Tileset()
{

}

Tileset* Tileset::copy() {
    Tileset *copy = new Tileset;
    copy->name = this->name;
    copy->is_compressed = this->is_compressed;
    copy->is_secondary = this->is_secondary;
    copy->padding = this->padding;
    copy->tiles_label = this->tiles_label;
    copy->palettes_label = this->palettes_label;
    copy->metatiles_label = this->metatiles_label;
    copy->metatiles_path = this->metatiles_path;
    copy->callback_label = this->callback_label;
    copy->metatile_attrs_label = this->metatile_attrs_label;
    copy->metatile_attrs_path = this->metatile_attrs_path;
    copy->tilesImage = this->tilesImage.copy();
    copy->tilesImagePath = this->tilesImagePath;
    for (int i = 0; i < this->palettePaths.length(); i++) {
        copy->palettePaths.append(this->palettePaths.at(i));
    }
    copy->tiles = new QList<QImage>;
    for (QImage tile : *this->tiles) {
        copy->tiles->append(tile.copy());
    }
    copy->metatiles = new QList<Metatile*>;
    for (Metatile *metatile : *this->metatiles) {
        copy->metatiles->append(metatile->copy());
    }
    copy->palettes = new QList<QList<QRgb>>;
    for (QList<QRgb> palette : *this->palettes) {
        QList<QRgb> copyPalette;
        for (QRgb color : palette) {
            copyPalette.append(color);
        }
        copy->palettes->append(copyPalette);
    }
    copy->palettePreviews = new QList<QList<QRgb>>;
    for (QList<QRgb> palette : *this->palettePreviews) {
        QList<QRgb> copyPalette;
        for (QRgb color : palette) {
            copyPalette.append(color);
        }
        copy->palettePreviews->append(copyPalette);
    }
    return copy;
}

Tileset* Tileset::getBlockTileset(int metatile_index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatile_index < Project::getNumMetatilesPrimary()) {
        return primaryTileset;
    } else {
        return secondaryTileset;
    }
}

Metatile* Tileset::getMetatile(int index, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getBlockTileset(index, primaryTileset, secondaryTileset);
    int local_index = Metatile::getBlockIndex(index);
    if (!tileset || !tileset->metatiles) {
        return nullptr;
    }
    Metatile *metatile = tileset->metatiles->value(local_index, nullptr);
    return metatile;
}

bool Tileset::metatileIsValid(uint16_t metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatileId >= Project::getNumMetatilesTotal())
        return false;

    if (metatileId < Project::getNumMetatilesPrimary() && metatileId >= primaryTileset->metatiles->length())
        return false;

    if (metatileId < Project::getNumMetatilesTotal() && metatileId >= Project::getNumMetatilesPrimary() + secondaryTileset->metatiles->length())
        return false;

    return true;
}

QList<QList<QRgb>> Tileset::getBlockPalettes(Tileset *primaryTileset, Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QList<QRgb>> palettes;
    auto primaryPalettes = useTruePalettes ? primaryTileset->palettes : primaryTileset->palettePreviews;
    for (int i = 0; i < Project::getNumPalettesPrimary(); i++) {
        palettes.append(primaryPalettes->at(i));
    }
    auto secondaryPalettes = useTruePalettes ? secondaryTileset->palettes : secondaryTileset->palettePreviews;
    for (int i = Project::getNumPalettesPrimary(); i < Project::getNumPalettesTotal(); i++) {
        palettes.append(secondaryPalettes->at(i));
    }
    return palettes;
}

QList<QRgb> Tileset::getPalette(int paletteId, Tileset *primaryTileset, Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QRgb> paletteTable;
    Tileset *tileset = paletteId < Project::getNumPalettesPrimary()
            ? primaryTileset
            : secondaryTileset;
    auto palettes = useTruePalettes ? tileset->palettes : tileset->palettePreviews;
    for (int i = 0; i < palettes->at(paletteId).length(); i++) {
        paletteTable.append(palettes->at(paletteId).at(i));
    }
    return paletteTable;
}

bool Tileset::appendToHeaders(QString headerFile, QString friendlyName){
    QFile file(headerFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(headerFile));
        return false;
    }
    QString dataString = "\n\t.align 2\n";
    dataString.append(QString("%1::\n").arg(this->name));
    dataString.append(QString("\t.byte %1 @ is compressed\n").arg(this->is_compressed));
    dataString.append(QString("\t.byte %1 @ is secondary\n").arg(this->is_secondary));
    dataString.append(QString("\t.2byte %1\n").arg(this->padding));
    dataString.append(QString("\t.4byte gTilesetTiles_%1\n").arg(friendlyName));
    dataString.append(QString("\t.4byte gTilesetPalettes_%1\n").arg(friendlyName));
    dataString.append(QString("\t.4byte gMetatiles_%1\n").arg(friendlyName));
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        dataString.append("\t.4byte NULL\n");
        dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
    } else {
        dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
        dataString.append("\t.4byte NULL\n");
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToGraphics(QString graphicsFile, QString friendlyName, bool primary) {
    QString primaryString = primary ? "primary" : "secondary";
    QFile file(graphicsFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(graphicsFile));
        return false;
    }
    QString dataString = "\n\t.align 2\n";
    dataString.append(QString("gTilesetPalettes_%1::\n").arg(friendlyName));
    for(int i = 0; i < Project::getNumPalettesTotal(); ++i) {
        QString paletteString;
        paletteString.sprintf("%02d.gbapal", i);
        dataString.append(QString("\t.incbin \"data/tilesets/%1/%2/palettes/%3\"\n").arg(primaryString, friendlyName.toLower(), paletteString));

    }
    dataString.append("\n\t.align 2\n");
    dataString.append(QString("gTilesetTiles_%1::\n").arg(friendlyName));
    dataString.append(QString("\t.incbin \"data/tilesets/%1/%2/tiles.4bpp.lz\"\n").arg(primaryString, friendlyName.toLower()));
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToMetatiles(QString metatileFile, QString friendlyName, bool primary) {
    QString primaryString = primary ? "primary" : "secondary";
    QFile file(metatileFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(metatileFile));
        return false;
    }
    QString dataString = "\n\t.align 1\n";
    dataString.append(QString("gMetatiles_%1::\n").arg(friendlyName));
    dataString.append(QString("\t.incbin \"data/tilesets/%1/%2/metatiles.bin\"\n").arg(primaryString, friendlyName.toLower()));
    dataString.append(QString("\n\t.align 1\n"));
    dataString.append(QString("gMetatileAttributes_%1::\n").arg(friendlyName));
    dataString.append(QString("\t.incbin \"data/tilesets/%1/%2/metatile_attributes.bin\"").arg(primaryString, friendlyName.toLower()));
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}
