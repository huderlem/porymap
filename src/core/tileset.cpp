#include "tileset.h"
#include "metatile.h"
#include "project.h"
#include "log.h"
#include "config.h"

#include <QPainter>
#include <QImage>


Tileset::Tileset(const Tileset &other)
    : name(other.name),
      is_compressed(other.is_compressed),
      is_secondary(other.is_secondary),
      padding(other.padding),
      tiles_label(other.tiles_label),
      palettes_label(other.palettes_label),
      metatiles_label(other.metatiles_label),
      metatiles_path(other.metatiles_path),
      callback_label(other.callback_label),
      metatile_attrs_label(other.metatile_attrs_label),
      metatile_attrs_path(other.metatile_attrs_path),
      tilesImagePath(other.tilesImagePath),
      tilesImage(other.tilesImage),
      palettePaths(other.palettePaths),
      tiles(other.tiles),
      palettes(other.palettes),
      palettePreviews(other.palettePreviews)
{
    for (auto *metatile : other.metatiles) {
        metatiles.append(new Metatile(*metatile));
    }
}

Tileset &Tileset::operator=(const Tileset &other) {
    name = other.name;
    is_compressed = other.is_compressed;
    is_secondary = other.is_secondary;
    padding = other.padding;
    tiles_label = other.tiles_label;
    palettes_label = other.palettes_label;
    metatiles_label = other.metatiles_label;
    metatiles_path = other.metatiles_path;
    callback_label = other.callback_label;
    metatile_attrs_label = other.metatile_attrs_label;
    metatile_attrs_path = other.metatile_attrs_path;
    tilesImagePath = other.tilesImagePath;
    tilesImage = other.tilesImage;
    palettePaths = other.palettePaths;
    tiles = other.tiles;
    palettes = other.palettes;
    palettePreviews = other.palettePreviews;

    metatiles.clear();
    for (auto *metatile : other.metatiles) {
        metatiles.append(new Metatile(*metatile));
    }

    return *this;
}

Tileset* Tileset::getTileTileset(int tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (tileId < Project::getNumTilesPrimary()) {
        return primaryTileset;
    } else if (tileId < Project::getNumTilesTotal()) {
        return secondaryTileset;
    } else {
        return nullptr;
    }
}

Tileset* Tileset::getMetatileTileset(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatileId < Project::getNumMetatilesPrimary()) {
        return primaryTileset;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        return secondaryTileset;
    } else {
        return nullptr;
    }
}

Metatile* Tileset::getMetatile(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getMetatileTileset(metatileId, primaryTileset, secondaryTileset);
    int index = Metatile::getIndexInTileset(metatileId);
    if (!tileset) {
        return nullptr;
    }
    return tileset->metatiles.value(index, nullptr);
}

bool Tileset::metatileIsValid(uint16_t metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatileId >= Project::getNumMetatilesTotal())
        return false;

    if (metatileId < Project::getNumMetatilesPrimary() && metatileId >= primaryTileset->metatiles.length())
        return false;

    if (metatileId >= Project::getNumMetatilesPrimary() + secondaryTileset->metatiles.length())
        return false;

    return true;
}

QList<QList<QRgb>> Tileset::getBlockPalettes(Tileset *primaryTileset, Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QList<QRgb>> palettes;
    auto primaryPalettes = useTruePalettes ? primaryTileset->palettes : primaryTileset->palettePreviews;
    for (int i = 0; i < Project::getNumPalettesPrimary(); i++) {
        palettes.append(primaryPalettes.at(i));
    }
    auto secondaryPalettes = useTruePalettes ? secondaryTileset->palettes : secondaryTileset->palettePreviews;
    for (int i = Project::getNumPalettesPrimary(); i < Project::getNumPalettesTotal(); i++) {
        palettes.append(secondaryPalettes.at(i));
    }
    return palettes;
}

QList<QRgb> Tileset::getPalette(int paletteId, Tileset *primaryTileset, Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QRgb> paletteTable;
    Tileset *tileset = paletteId < Project::getNumPalettesPrimary()
            ? primaryTileset
            : secondaryTileset;
    auto palettes = useTruePalettes ? tileset->palettes : tileset->palettePreviews;
    for (int i = 0; i < palettes.at(paletteId).length(); i++) {
        paletteTable.append(palettes.at(paletteId).at(i));
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
        QString paletteString = QString("%1.gbapal").arg(i, 2, 10, QLatin1Char('0'));
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
    dataString.append(QString("\t.incbin \"data/tilesets/%1/%2/metatile_attributes.bin\"\n").arg(primaryString, friendlyName.toLower()));
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}
