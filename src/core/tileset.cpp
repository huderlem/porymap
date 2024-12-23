#include "tileset.h"
#include "metatile.h"
#include "project.h"
#include "log.h"
#include "config.h"
#include "imageproviders.h"
#include "validator.h"

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
      tilesImage(other.tilesImage.copy()),
      palettePaths(other.palettePaths),
      metatileLabels(other.metatileLabels),
      palettes(other.palettes),
      palettePreviews(other.palettePreviews),
      m_hasUnsavedTilesImage(other.m_hasUnsavedTilesImage)
{
    for (auto tile : other.tiles) {
        tiles.append(tile.copy());
    }

    for (auto *metatile : other.m_metatiles) {
        m_metatiles.append(new Metatile(*metatile));
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
    tilesImage = other.tilesImage.copy();
    palettePaths = other.palettePaths;
    metatileLabels = other.metatileLabels;
    palettes = other.palettes;
    palettePreviews = other.palettePreviews;

    tiles.clear();
    for (auto tile : other.tiles) {
        tiles.append(tile.copy());
    }

    clearMetatiles();
    for (auto *metatile : other.m_metatiles) {
        m_metatiles.append(new Metatile(*metatile));
    }

    return *this;
}

Tileset::~Tileset() {
    clearMetatiles();
}

void Tileset::clearMetatiles() {
    qDeleteAll(m_metatiles);
    m_metatiles.clear();
}

void Tileset::setMetatiles(const QList<Metatile*> &metatiles) {
    clearMetatiles();
    m_metatiles = metatiles;
}

void Tileset::addMetatile(Metatile* metatile) {
    m_metatiles.append(metatile);
}

void Tileset::resizeMetatiles(unsigned int newNumMetatiles) {
    while (m_metatiles.length() > newNumMetatiles) {
        delete m_metatiles.takeLast();
    }
    const int numTiles = projectConfig.getNumTilesInMetatile();
    while (m_metatiles.length() < newNumMetatiles) {
        m_metatiles.append(new Metatile(numTiles));
    }
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
    if (!tileset) {
        return nullptr;
    }
    int index = Metatile::getIndexInTileset(metatileId);
    return tileset->m_metatiles.value(index, nullptr);
}

// Metatile labels are stored per-tileset. When looking for a metatile label, first search in the tileset
// that the metatile belongs to. If one isn't found, search in the other tileset. Labels coming from the
// tileset that the metatile does not belong to are shared and cannot be edited via Porymap.
Tileset* Tileset::getMetatileLabelTileset(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *mainTileset = nullptr;
    Tileset *alternateTileset = nullptr;
    if (metatileId < Project::getNumMetatilesPrimary()) {
        mainTileset = primaryTileset;
        alternateTileset = secondaryTileset;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        mainTileset = secondaryTileset;
        alternateTileset = primaryTileset;
    }

    if (mainTileset && !mainTileset->metatileLabels.value(metatileId).isEmpty()) {
        return mainTileset;
    } else if (alternateTileset && !alternateTileset->metatileLabels.value(metatileId).isEmpty()) {
        return alternateTileset;
    }
    return nullptr;
}

// Return the pair of possible metatile labels for the specified metatile.
// "owned" is the label for the tileset to which the metatile belongs.
// "shared" is the label for the tileset to which the metatile does not belong.
MetatileLabelPair Tileset::getMetatileLabelPair(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels;
    QString primaryMetatileLabel = primaryTileset ? primaryTileset->metatileLabels.value(metatileId) : "";
    QString secondaryMetatileLabel = secondaryTileset ? secondaryTileset->metatileLabels.value(metatileId) : "";

    if (metatileId < Project::getNumMetatilesPrimary()) {
        labels.owned = primaryMetatileLabel;
        labels.shared = secondaryMetatileLabel;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        labels.owned = secondaryMetatileLabel;
        labels.shared = primaryMetatileLabel;
    }
    return labels;
}

// If the metatile has a label in the tileset it belongs to, return that label.
// If it doesn't, and the metatile has a label in the other tileset, return that label.
// Otherwise return an empty string.
QString Tileset::getMetatileLabel(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, primaryTileset, secondaryTileset);
    return !labels.owned.isEmpty() ? labels.owned : labels.shared;
}

// Just get the "owned" metatile label, i.e. the one for the tileset that the metatile belongs to.
QString Tileset::getOwnedMetatileLabel(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, primaryTileset, secondaryTileset);
    return labels.owned;
}

bool Tileset::setMetatileLabel(int metatileId, QString label, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getMetatileTileset(metatileId, primaryTileset, secondaryTileset);
    if (!tileset)
        return false;

    IdentifierValidator validator;
    if (!validator.isValid(label))
        return false;

    tileset->metatileLabels[metatileId] = label;
    return true;
}

QString Tileset::getMetatileLabelPrefix()
{
    return Tileset::getMetatileLabelPrefix(this->name);
}

QString Tileset::getMetatileLabelPrefix(const QString &name)
{
    // Default is "gTileset_Name" --> "METATILE_Name_"
    const QString tilesetPrefix = projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix);
    const QString labelPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix);
    return QString("%1%2_").arg(labelPrefix).arg(QString(name).replace(tilesetPrefix, ""));
}

bool Tileset::metatileIsValid(uint16_t metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    if (metatileId >= Project::getNumMetatilesTotal())
        return false;

    if (metatileId < Project::getNumMetatilesPrimary() && metatileId >= primaryTileset->numMetatiles())
        return false;

    if (metatileId >= Project::getNumMetatilesPrimary() + secondaryTileset->numMetatiles())
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
    QString isSecondaryStr = this->is_secondary ? "TRUE" : "FALSE";
    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("%1::\n").arg(this->name));
        dataString.append("\t.byte TRUE @ is compressed\n");
        dataString.append(QString("\t.byte %1 @ is secondary\n").arg(isSecondaryStr));
        dataString.append("\t.2byte 0 @ padding\n");
        dataString.append(QString("\t.4byte gTilesetTiles_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gTilesetPalettes_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gMetatiles_%1\n").arg(friendlyName));
        if (projectConfig.baseGameVersion == BaseGameVersion::pokefirered) {
            dataString.append("\t.4byte NULL @ animation callback\n");
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
        } else {
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
            dataString.append("\t.4byte NULL @ animation callback\n");
        }
    } else {
        // Append to C file
        dataString.append(QString("const struct Tileset %1 =\n{\n").arg(this->name));
        if (projectConfig.tilesetsHaveIsCompressed) dataString.append("    .isCompressed = TRUE,\n");
        dataString.append(QString("    .isSecondary = %1,\n").arg(isSecondaryStr));
        dataString.append(QString("    .tiles = gTilesetTiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .palettes = gTilesetPalettes_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatiles = gMetatiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatileAttributes = gMetatileAttributes_%1,\n").arg(friendlyName));
        if (projectConfig.tilesetsHaveCallback) dataString.append("    .callback = NULL,\n");
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
        QString numBits = QString::number(projectConfig.metatileAttributesSize * 8);
        dataString.append(QString("const u%1 gMetatileAttributes_%2[] = INCBIN_U%1(\"%3\");\n").arg(numBits, friendlyName, metatileAttrsPath));
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
    return Tileset::getExpectedDir(this->name, this->is_secondary);
}

QString Tileset::getExpectedDir(QString tilesetName, bool isSecondary)
{
    static const QRegularExpression re("([a-z])([A-Z0-9])");
    const QString category = isSecondary ? "secondary" : "primary";
    const QString basePath = projectConfig.getFilePath(ProjectFilePath::data_tilesets_folders) + category + "/";
    const QString prefix = projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix);
    return basePath + tilesetName.replace(prefix, "").replace(re, "\\1_\\2").toLower();
}

// Get the expected positions of the members in struct Tileset.
// Used when parsing asm tileset data, or C tileset data that's missing initializers.
QHash<int, QString> Tileset::getHeaderMemberMap(bool usingAsm)
{
     // The asm header has a padding field that needs to be skipped
    int paddingOffset = usingAsm ? 1 : 0;

    // The position of metatileAttributes changes between games
    bool isPokefirered = (projectConfig.baseGameVersion == BaseGameVersion::pokefirered);
    int metatileAttrPosition = (isPokefirered ? 6 : 5) + paddingOffset;

    auto map = QHash<int, QString>();
    map.insert(1, "isSecondary");
    map.insert(2 + paddingOffset, "tiles");
    map.insert(3 + paddingOffset, "palettes");
    map.insert(4 + paddingOffset, "metatiles");
    map.insert(metatileAttrPosition, "metatileAttributes");
    return map;
}

void Tileset::loadMetatiles() {
    clearMetatiles();

    QFile metatiles_file(this->metatiles_path);
    if (!metatiles_file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading.").arg(this->metatiles_path));
        return;
    }

    QByteArray data = metatiles_file.readAll();
    int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
    int bytesPerMetatile = 2 * tilesPerMetatile;
    int num_metatiles = data.length() / bytesPerMetatile;
    for (int i = 0; i < num_metatiles; i++) {
        auto metatile = new Metatile;
        int index = i * bytesPerMetatile;
        for (int j = 0; j < tilesPerMetatile; j++) {
            uint16_t tileRaw = static_cast<unsigned char>(data[index++]);
            tileRaw |= static_cast<unsigned char>(data[index++]) << 8;
            metatile->tiles.append(Tile(tileRaw));
        }
        m_metatiles.append(metatile);
    }
}

void Tileset::saveMetatiles() {
    QFile metatiles_file(this->metatiles_path);
    if (!metatiles_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing.").arg(this->metatiles_path));
        return;
    }

    QByteArray data;
    int numTiles = projectConfig.getNumTilesInMetatile();
    for (const auto &metatile : m_metatiles) {
        for (int i = 0; i < numTiles; i++) {
            uint16_t tile = metatile->tiles.at(i).rawValue();
            data.append(static_cast<char>(tile));
            data.append(static_cast<char>(tile >> 8));
        }
    }
    metatiles_file.write(data);
}

void Tileset::loadMetatileAttributes() {
    QFile attrs_file(this->metatile_attrs_path);
    if (!attrs_file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading.").arg(this->metatile_attrs_path));
        return;
    }

    QByteArray data = attrs_file.readAll();
    int attrSize = projectConfig.metatileAttributesSize;
    int numMetatiles = m_metatiles.length();
    int numMetatileAttrs = data.length() / attrSize;
    if (numMetatiles != numMetatileAttrs) {
        logWarn(QString("Metatile count %1 does not match metatile attribute count %2 in %3").arg(numMetatiles).arg(numMetatileAttrs).arg(this->name));
    }

    for (int i = 0; i < qMin(numMetatiles, numMetatileAttrs); i++) {
        uint32_t attributes = 0;
        for (int j = 0; j < attrSize; j++)
            attributes |= static_cast<unsigned char>(data.at(i * attrSize + j)) << (8 * j);
        m_metatiles.at(i)->setAttributes(attributes);
    }
}

void Tileset::saveMetatileAttributes() {
    QFile attrs_file(this->metatile_attrs_path);
    if (!attrs_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing.").arg(this->metatile_attrs_path));
        return;
    }

    QByteArray data;
    for (const auto &metatile : m_metatiles) {
        uint32_t attributes = metatile->getAttributes();
        for (int i = 0; i < projectConfig.metatileAttributesSize; i++)
            data.append(static_cast<char>(attributes >> (8 * i)));
    }
    attrs_file.write(data);
}

void Tileset::loadTilesImage(QImage *importedImage) {
    QImage image;
    if (importedImage) {
        image = *importedImage;
        m_hasUnsavedTilesImage = true;
    } else if (QFile::exists(this->tilesImagePath)) {
        // No image provided, load from file path.
        image = QImage(this->tilesImagePath).convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
    } else {
        // Use default image
        image = QImage(8, 8, QImage::Format_Indexed8);
    }

    // Validate image contains 16 colors.
    int colorCount = image.colorCount();
    if (colorCount > 16) {
        flattenTo4bppImage(&image);
    } else if (colorCount < 16) {
        QVector<QRgb> colorTable = image.colorTable();
        for (int i = colorTable.length(); i < 16; i++) {
            colorTable.append(Qt::black);
        }
        image.setColorTable(colorTable);
    }

    QList<QImage> tiles;
    int w = 8;
    int h = 8;
    for (int y = 0; y < image.height(); y += h)
    for (int x = 0; x < image.width(); x += w) {
        QImage tile = image.copy(x, y, w, h);
        tiles.append(tile);
    }
    this->tilesImage = image;
    this->tiles = tiles;
}

void Tileset::saveTilesImage() {
    // Only write the tiles image if it was changed.
    // Porymap will only ever change an existing tiles image by importing a new one.
    if (!m_hasUnsavedTilesImage)
        return;

    if (!this->tilesImage.save(this->tilesImagePath, "PNG")) {
        logError(QString("Failed to save tiles image '%1'").arg(this->tilesImagePath));
        return;
    }

    m_hasUnsavedTilesImage = false;
}

void Tileset::loadPalettes() {
    this->palettes.clear();
    this->palettePreviews.clear();

    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        QList<QRgb> palette;
        QString path = this->palettePaths.value(i);
        if (!path.isEmpty()) {
            bool error = false;
            palette = PaletteUtil::parse(path, &error);
            if (error) palette.clear();
        }
        if (palette.isEmpty()) {
            // Either the palette failed to load, or no palette exists.
            // We expect tilesets to have a certain number of palettes,
            // so fill this palette with dummy colors.
            for (int j = 0; j < 16; j++) {
                palette.append(qRgb(j * 16, j * 16, j * 16));
            }
        }
        this->palettes.append(palette);
        this->palettePreviews.append(palette);
    }
}

void Tileset::savePalettes() {
    int numPalettes = qMin(this->palettePaths.length(), this->palettes.length());
    for (int i = 0; i < numPalettes; i++) {
        PaletteUtil::writeJASC(this->palettePaths.at(i), this->palettes.at(i).toVector(), 0, 16);
    }
}

void Tileset::load() {
    loadMetatiles();
    loadMetatileAttributes();
    loadTilesImage();
    loadPalettes();
}

// Because metatile labels are global (and handled by the project) we don't save them here.
void Tileset::save() {
    saveMetatiles();
    saveMetatileAttributes();
    saveTilesImage();
    savePalettes();
}
