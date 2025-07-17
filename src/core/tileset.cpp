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
      palettePaths(other.palettePaths),
      metatileLabels(other.metatileLabels),
      palettes(other.palettes),
      palettePreviews(other.palettePreviews),
      m_tilesImage(other.m_tilesImage.copy()),
      m_hasUnsavedTilesImage(other.m_hasUnsavedTilesImage)
{
    for (auto tile : other.m_tiles) {
        m_tiles.append(tile.copy());
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
    m_tilesImage = other.m_tilesImage.copy();
    palettePaths = other.palettePaths;
    metatileLabels = other.metatileLabels;
    palettes = other.palettes;
    palettePreviews = other.palettePreviews;

    m_tiles.clear();
    for (auto tile : other.m_tiles) {
        m_tiles.append(tile.copy());
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

void Tileset::resizeMetatiles(int newNumMetatiles) {
    if (newNumMetatiles < 0) newNumMetatiles = 0;
    while (m_metatiles.length() > newNumMetatiles) {
        delete m_metatiles.takeLast();
    }
    const int numTiles = projectConfig.getNumTilesInMetatile();
    while (m_metatiles.length() < newNumMetatiles) {
        m_metatiles.append(new Metatile(numTiles));
    }
}

int Tileset::maxMetatiles() const {
    return this->is_secondary ? Project::getNumMetatilesSecondary() : Project::getNumMetatilesPrimary();
}

int Tileset::maxTiles() const {
    return this->is_secondary ? Project::getNumTilesSecondary() : Project::getNumTilesPrimary();
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

    if (!label.isEmpty()) {
        IdentifierValidator validator;
        if (!validator.isValid(label))
            return false;
    }

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
    const QString labelPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix);
    return QString("%1%2_").arg(labelPrefix).arg(Tileset::stripPrefix(name));
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

    QList<QList<QRgb>> primaryPalettes;
    if (primaryTileset) {
        primaryPalettes = useTruePalettes ? primaryTileset->palettes : primaryTileset->palettePreviews;
    }
    for (int i = 0; i < Project::getNumPalettesPrimary(); i++) {
        palettes.append(primaryPalettes.value(i));
    }

    QList<QList<QRgb>> secondaryPalettes;
    if (secondaryTileset) {
        secondaryPalettes = useTruePalettes ? secondaryTileset->palettes : secondaryTileset->palettePreviews;
    }
    for (int i = Project::getNumPalettesPrimary(); i < Project::getNumPalettesTotal(); i++) {
        palettes.append(secondaryPalettes.value(i));
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

bool Tileset::appendToHeaders(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
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

bool Tileset::appendToGraphics(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
        return false;
    }

    const QString tilesetDir = this->getExpectedDir();
    const QString tilesPath = QString("%1/tiles%2").arg(tilesetDir).arg(projectConfig.getIdentifier(ProjectIdentifier::tiles_output_extension));
    const QString palettesPath = tilesetDir + "/palettes/";
    const QString palettesExt = projectConfig.getIdentifier(ProjectIdentifier::pals_output_extension);

    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("gTilesetPalettes_%1::\n").arg(friendlyName));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("\t.incbin \"%1%2%3\"\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')).arg(palettesExt));
        dataString.append("\n\t.align 2\n");
        dataString.append(QString("gTilesetTiles_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(tilesPath));
    } else {
        // Append to C file
        dataString.append(QString("const u16 gTilesetPalettes_%1[][16] =\n{\n").arg(friendlyName));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("    INCBIN_U16(\"%1%2%3\"),\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')).arg(palettesExt));
        dataString.append("};\n");
        dataString.append(QString("\nconst u32 gTilesetTiles_%1[] = INCBIN_U32(\"%2\");\n").arg(friendlyName, tilesPath));
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToMetatiles(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
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
    const QString basePath = isSecondary ? projectConfig.getFilePath(ProjectFilePath::data_secondary_tilesets_folders)
                                         : projectConfig.getFilePath(ProjectFilePath::data_primary_tilesets_folders);

    static const QRegularExpression re("([a-z])([A-Z0-9])");
    return basePath + Tileset::stripPrefix(tilesetName).replace(re, "\\1_\\2").toLower();
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

bool Tileset::loadMetatiles() {
    clearMetatiles();

    QFile file(this->metatiles_path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading: %2").arg(this->metatiles_path).arg(file.errorString()));
        return false;
    }

    QByteArray data = file.readAll();
    int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
    int bytesPerMetatile = Tile::sizeInBytes() * tilesPerMetatile;
    int numMetatiles = data.length() / bytesPerMetatile;
    if (numMetatiles > maxMetatiles()) {
        logWarn(QString("%1 metatile count %2 exceeds limit of %3. Additional metatiles will be ignored.")
                        .arg(this->name)
                        .arg(numMetatiles)
                        .arg(maxMetatiles()));
        numMetatiles = maxMetatiles();
    }

    for (int i = 0; i < numMetatiles; i++) {
        auto metatile = new Metatile;
        int index = i * bytesPerMetatile;
        for (int j = 0; j < tilesPerMetatile; j++) {
            uint16_t tileRaw = static_cast<unsigned char>(data[index++]);
            tileRaw |= static_cast<unsigned char>(data[index++]) << 8;
            metatile->tiles.append(Tile(tileRaw));
        }
        m_metatiles.append(metatile);
    }
    return true;
}

bool Tileset::saveMetatiles() {
    QFile file(this->metatiles_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing: %2").arg(this->metatiles_path).arg(file.errorString()));
        return false;
    }

    QByteArray data;
    int numTiles = projectConfig.getNumTilesInMetatile();
    for (const auto &metatile : m_metatiles) {
        for (int i = 0; i < numTiles; i++) {
            uint16_t tile = metatile->tiles.value(i).rawValue();
            data.append(static_cast<char>(tile));
            data.append(static_cast<char>(tile >> 8));
        }
    }
    file.write(data);
    return true;
}

bool Tileset::loadMetatileAttributes() {
    QFile file(this->metatile_attrs_path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading: %2").arg(this->metatile_attrs_path).arg(file.errorString()));
        return false;
    }

    QByteArray data = file.readAll();
    int attrSize = projectConfig.metatileAttributesSize;
    int numMetatiles = m_metatiles.length();
    int numMetatileAttrs = data.length() / attrSize;
    if (numMetatileAttrs > numMetatiles) {
        logWarn(QString("%1 metatile attributes count %2 exceeds metatile count of %3. Additional attributes will be ignored.")
                            .arg(this->name)
                            .arg(numMetatileAttrs)
                            .arg(numMetatiles));
        numMetatileAttrs = numMetatiles;
    } else if (numMetatileAttrs < numMetatiles) {
        logWarn(QString("%1 metatile attributes count %2 is fewer than the metatile count of %3. Missing attributes will default to 0.")
                            .arg(this->name)
                            .arg(numMetatileAttrs)
                            .arg(numMetatiles));
    }

    for (int i = 0; i < numMetatileAttrs; i++) {
        uint32_t attributes = 0;
        for (int j = 0; j < attrSize; j++)
            attributes |= static_cast<unsigned char>(data.at(i * attrSize + j)) << (8 * j);
        m_metatiles.at(i)->setAttributes(attributes);
    }
    return true;
}

bool Tileset::saveMetatileAttributes() {
    QFile file(this->metatile_attrs_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing: %2").arg(this->metatile_attrs_path).arg(file.errorString()));
        return false;
    }

    QByteArray data;
    for (const auto &metatile : m_metatiles) {
        uint32_t attributes = metatile->getAttributes();
        for (int i = 0; i < projectConfig.metatileAttributesSize; i++)
            data.append(static_cast<char>(attributes >> (8 * i)));
    }
    file.write(data);
    return true;
}

bool Tileset::loadTilesImage(QImage *importedImage) {
    QImage image;
    bool imported = false;
    if (importedImage) {
        image = *importedImage;
        imported = true;
    } else if (QFile::exists(this->tilesImagePath)) {
        // No image provided, load from file path.
        image = QImage(this->tilesImagePath).convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
    }

    if (image.isNull()) {
        logWarn(QString("Failed to load tiles image for %1. Using default tiles image.").arg(this->name));
        image = QImage(Tile::pixelWidth(), Tile::pixelHeight(), QImage::Format_Indexed8);
        image.fill(0);
    }

    // Validate image dimensions
    if (image.width() % Tile::pixelWidth() || image.height() % Tile::pixelHeight()) {
        logError(QString("%1 tiles image has invalid dimensions %2x%3. Dimensions must be a multiple of %4x%5.")
                            .arg(this->name)
                            .arg(image.width())
                            .arg(image.height())
                            .arg(Tile::pixelWidth())
                            .arg(Tile::pixelHeight()));
        return false;
    }

    // Validate image contains 16 colors.
    int colorCount = image.colorCount();
    if (colorCount > 16) {
        flattenTo4bppImage(&image);
    } else if (colorCount < 16) {
        QVector<QRgb> colorTable = image.colorTable();
        for (int i = colorTable.length(); i < 16; i++) {
            colorTable.append(0);
        }
        image.setColorTable(colorTable);
    }
    m_tilesImage = image;

    // Cut up the full tiles image into individual tile images.
    m_tiles.clear();
    for (int y = 0; y < image.height(); y += Tile::pixelHeight())
    for (int x = 0; x < image.width(); x += Tile::pixelWidth()) {
        m_tiles.append(image.copy(x, y, Tile::pixelWidth(), Tile::pixelHeight()));
    }

    if (m_tiles.length() > maxTiles()) {
        logWarn(QString("%1 tile count of %2 exceeds limit of %3. Additional tiles will not be displayed.")
                            .arg(this->name)
                            .arg(m_tiles.length())
                            .arg(maxTiles()));

        // Just resize m_tiles so that numTiles() reports the correct tile count.
        // We'll leave m_tilesImage alone (it doesn't get displayed, and we don't want to delete the user's image data).
        m_tiles = m_tiles.mid(0, maxTiles());
    }

    if (imported) {
        // Only set this flag once we've successfully loaded the tiles image.
        m_hasUnsavedTilesImage = true;
    }

    return true;
}

bool Tileset::saveTilesImage() {
    // Only write the tiles image if it was changed.
    // Porymap will only ever change an existing tiles image by importing a new one.
    if (!m_hasUnsavedTilesImage)
        return true;

    if (!m_tilesImage.save(this->tilesImagePath, "PNG")) {
        logError(QString("Failed to save tiles image '%1'").arg(this->tilesImagePath));
        return false;
    }

    m_hasUnsavedTilesImage = false;
    return true;
}

bool Tileset::loadPalettes() {
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
    return true;
}

bool Tileset::savePalettes() {
    bool success = true;
    int numPalettes = qMin(this->palettePaths.length(), this->palettes.length());
    for (int i = 0; i < numPalettes; i++) {
        if (!PaletteUtil::writeJASC(this->palettePaths.at(i), this->palettes.at(i).toVector(), 0, 16))
            success = false;
    }
    return success;
}

bool Tileset::load() {
    bool success = true;
    if (!loadMetatiles()) success = false;
    if (!loadMetatileAttributes()) success = false;
    if (!loadTilesImage()) success = false;
    if (!loadPalettes()) success = false;
    return success;
}

// Because metatile labels are global (and handled by the project) we don't save them here.
bool Tileset::save() {
    bool success = true;
    if (!saveMetatiles()) success = false;
    if (!saveMetatileAttributes()) success = false;
    if (!saveTilesImage()) success = false;
    if (!savePalettes()) success = false;
    return success;
}

QString Tileset::stripPrefix(const QString &fullName) {
    return QString(fullName).replace(projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix), "");
}
