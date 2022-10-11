#include "tileset.h"
#include "metatile.h"
#include "project.h"
#include "log.h"
#include "config.h"

#include <QPainter>
#include <QImage>


Tileset::Tileset(const Tileset &other)
    : name(other.name),
      is_secondary(other.is_secondary),
      tiles_label(other.tiles_label),
      palettes_label(other.palettes_label),
      metatiles_label(other.metatiles_label),
      metatiles_path(other.metatiles_path),
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
    is_secondary = other.is_secondary;
    tiles_label = other.tiles_label;
    palettes_label = other.palettes_label;
    metatiles_label = other.metatiles_label;
    metatiles_path = other.metatiles_path;
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

    if (paletteId < 0 || paletteId >= palettes.length()){
        logError(QString("Invalid tileset palette id '%1' requested.").arg(paletteId));
        return paletteTable;
    }

    for (int i = 0; i < palettes.at(paletteId).length(); i++) {
        paletteTable.append(palettes.at(paletteId).at(i));
    }
    return paletteTable;
}

bool Tileset::appendToHeaders(QString root, QString friendlyName, bool usingAsm) {
    QString headersFile = root + "/" + (usingAsm ? projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm)
                                              : projectConfig.getFilePath(ProjectFilePath::tilesets_headers));
    QFile file(headersFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(headersFile));
        return false;
    }
    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("%1::\n").arg(this->name));
        dataString.append("\t.byte TRUE @ is compressed\n");
        dataString.append(QString("\t.byte %1 @ is secondary\n").arg(this->is_secondary));
        dataString.append("\t.2byte 0 @ padding\n");
        dataString.append(QString("\t.4byte gTilesetTiles_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gTilesetPalettes_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gMetatiles_%1\n").arg(friendlyName));
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            dataString.append("\t.4byte NULL @ animation callback\n");
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
        } else {
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
            dataString.append("\t.4byte NULL @ animation callback\n");
        }
    } else {
        // Append to C file
        dataString.append(QString("const struct Tileset %1 =\n{\n").arg(this->name));
        if (projectConfig.getTilesetsHaveIsCompressed()) dataString.append("    .isCompressed = TRUE,\n");
        dataString.append(QString("    .isSecondary = %1,\n").arg(this->is_secondary));
        dataString.append(QString("    .tiles = gTilesetTiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .palettes = gTilesetPalettes_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatiles = gMetatiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatileAttributes = gMetatileAttributes_%1,\n").arg(friendlyName));
        if (projectConfig.getTilesetsHaveCallback()) dataString.append("    .callback = NULL,\n");
        dataString.append("};\n");
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToGraphics(QString root, QString friendlyName, bool usingAsm) {
    QString graphicsFile = root + "/" + (usingAsm ? projectConfig.getFilePath(ProjectFilePath::tilesets_graphics_asm)
                                                  : projectConfig.getFilePath(ProjectFilePath::tilesets_graphics));
    QFile file(graphicsFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(graphicsFile));
        return false;
    }

    const QString tilesetDir = this->getExpectedDir();
    const QString tilesPath = tilesetDir + "/tiles.4bpp.lz";
    const QString palettesPath = tilesetDir + "/palettes/";

    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("gTilesetPalettes_%1::\n").arg(friendlyName));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("\t.incbin \"%1%2.gbapal\"\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')));
        dataString.append("\n\t.align 2\n");
        dataString.append(QString("gTilesetTiles_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(tilesPath));
    } else {
        // Append to C file
        dataString.append(QString("const u16 gTilesetPalettes_%1[][16] =\n{\n").arg(friendlyName));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("    INCBIN_U16(\"%1%2.gbapal\"),\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')));
        dataString.append("};\n");
        dataString.append(QString("\nconst u32 gTilesetTiles_%1[] = INCBIN_U32(\"%2\");\n").arg(friendlyName, tilesPath));
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToMetatiles(QString root, QString friendlyName, bool usingAsm) {
    QString metatileFile = root + "/" + (usingAsm ? projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles_asm)
                                                  : projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles));
    QFile file(metatileFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(metatileFile));
        return false;
    }

    const QString tilesetDir = this->getExpectedDir();
    const QString metatilesPath = tilesetDir + "/metatiles.bin";
    const QString metatileAttrsPath = tilesetDir + "/metatile_attributes.bin";

    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 1\n");
        dataString.append(QString("gMetatiles_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(metatilesPath));
        dataString.append(QString("\n\t.align 1\n"));
        dataString.append(QString("gMetatileAttributes_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(metatileAttrsPath));
    } else {
        // Append to C file
        dataString.append(QString("const u16 gMetatiles_%1[] = INCBIN_U16(\"%2\");\n").arg(friendlyName, metatilesPath));
        QString attrSize = (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) ? "32" : "16";
        dataString.append(QString("const u%1 gMetatileAttributes_%2[] = INCBIN_U%1(\"%3\");\n").arg(attrSize, friendlyName, metatileAttrsPath));
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

// The path where Porymap expects a Tileset's graphics assets to be stored (but not necessarily where they actually are)
// Example: for gTileset_DepartmentStore, returns "data/tilesets/secondary/department_store"
QString Tileset::getExpectedDir()
{
    return Tileset::getExpectedDir(this->name, ParseUtil::gameStringToBool(this->is_secondary));
}

QString Tileset::getExpectedDir(QString tilesetName, bool isSecondary)
{
    QRegularExpression re("([a-z])([A-Z0-9])");
    const QString category = isSecondary ? "secondary" : "primary";
    const QString basePath = projectConfig.getFilePath(ProjectFilePath::data_tilesets_folders) + category + "/";
    return basePath + tilesetName.replace("gTileset_", "").replace(re, "\\1_\\2").toLower();
}

// Get the expected positions of the members in struct Tileset.
// Used when parsing asm tileset data, or C tileset data that's missing initializers.
QHash<int, QString> Tileset::getHeaderMemberMap(bool usingAsm)
{
     // The asm header has a padding field that needs to be skipped
    int paddingOffset = usingAsm ? 1 : 0;

    // The position of metatileAttributes changes between games
    bool isPokefirered = (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered);
    int metatileAttrPosition = (isPokefirered ? 6 : 5) + paddingOffset;

    auto map = QHash<int, QString>();
    map.insert(1, "isSecondary");
    map.insert(2 + paddingOffset, "tiles");
    map.insert(3 + paddingOffset, "palettes");
    map.insert(4 + paddingOffset, "metatiles");
    map.insert(metatileAttrPosition, "metatileAttributes");
    return map;
}
