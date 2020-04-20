#include "regionmap.h"
#include "paletteutil.h"
#include "log.h"
#include "config.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QImage>
#include <math.h>

static bool ensureRegionMapFileExists(QString filepath) {
    if (!QFile::exists(filepath)) {
        logError(QString("Region map file does not exist: %1").arg(filepath));
        return false;
    }
    return true;
}

bool RegionMap::init(Project *pro) {
    QString path = pro->root;
    this->project = pro;

    QSize dimensions = porymapConfig.getRegionMapDimensions();
    img_width_ = dimensions.width();
    img_height_ = dimensions.height();

    layout_width_  = img_width_  - this->padLeft - this->padRight;
    layout_height_ = img_height_ - this->padTop  - this->padBottom;

    region_map_bin_path        = path + "/graphics/pokenav/region_map_map.bin";
    region_map_png_path        = path + "/graphics/pokenav/region_map.png";
    region_map_entries_path    = path + "/src/data/region_map/region_map_entries.h";
    region_map_layout_bin_path = path + "/graphics/pokenav/region_map_section_layout.bin";
    city_map_tiles_path        = path + "/graphics/pokenav/zoom_tiles.png";
    bool allFilesExist = ensureRegionMapFileExists(region_map_bin_path)
                      && ensureRegionMapFileExists(region_map_png_path)
                      && ensureRegionMapFileExists(region_map_entries_path)
                      && ensureRegionMapFileExists(region_map_layout_bin_path)
                      && ensureRegionMapFileExists(city_map_tiles_path);

    return allFilesExist
        && readBkgImgBin()
        && readLayout();
}

void RegionMap::save() {
    logInfo("Saving region map data.");
    saveTileImages();
    saveBkgImgBin();
    saveLayout();
    porymapConfig.setRegionMapDimensions(this->img_width_, this->img_height_);
}

void RegionMap::saveTileImages() {
    if (region_map_png_needs_saving) {
        QFile backgroundTileFile(pngPath());
        if (backgroundTileFile.open(QIODevice::ReadOnly)) {
            QByteArray imageData = backgroundTileFile.readAll();
            QImage pngImage = QImage::fromData(imageData);
            this->region_map_png_path = project->root + "/graphics/pokenav/region_map.png";
            pngImage.save(pngPath());

            PaletteUtil parser;
            parser.writeJASC(project->root + "/graphics/pokenav/region_map.pal", pngImage.colorTable(), 0x70, 0x20);
        }
        region_map_png_needs_saving = false;
    }
    if (city_map_png_needs_saving) {
        QFile cityTileFile(cityTilesPath());
        if (cityTileFile.open(QIODevice::ReadOnly)) {
            QByteArray imageData = cityTileFile.readAll();
            QImage cityTilesImage = QImage::fromData(imageData);
            this->city_map_tiles_path = project->root + "/graphics/pokenav/zoom_tiles.png";
            cityTilesImage.save(cityTilesPath());
        }
        city_map_png_needs_saving = false;
    }
}

bool RegionMap::readBkgImgBin() {
    map_squares.clear();
    QFile binFile(region_map_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) {
        logError(QString("Failed to open region map map file %1.").arg(region_map_bin_path));
        return false;
    }

    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    if (mapBinData.size() < img_height_ * img_width_) {
        logError(QString("The region map tilemap at %1 is too small.").arg(region_map_bin_path));
        return false;
    }
    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            RegionMapSquare square;
            square.tile_img_id = mapBinData.at(n + m * img_width_ * 2);
            map_squares.append(square);
        }
    }
    return true;
}

void RegionMap::saveBkgImgBin() {
    QByteArray data(pow(img_width_ * 2, 2),0);

    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            data[n + m * img_width_ * 2] = map_squares[n + m * img_width_].tile_img_id;
        }
    }

    QFile file(region_map_bin_path);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(data);
    file.close();
}

bool RegionMap::readLayout() {
    sMapNames.clear();
    sMapNamesMap.clear();
    mapSecToMapEntry.clear();
    QFile file(region_map_entries_path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Failed to read region map entries file %1").arg(region_map_entries_path));
        return false;
    }

    QMap<QString, QString> *qmap = new QMap<QString, QString>;

    bool mapNamesQualified = false, mapEntriesQualified = false;

    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.contains(QRegularExpression(".*sMapName.*="))) {
            QRegularExpression reBefore("sMapName_(.*)\\[");
            QRegularExpression reAfter("_\\(\"(.*)\"");
            QString const_name = reBefore.match(line).captured(1);
            QString full_name = reAfter.match(line).captured(1);
            sMapNames.append(const_name);
            sMapNamesMap.insert(const_name, full_name);
            if (!mapNamesQualified) {
                project->dataQualifiers.insert("region_map_entries_names", project->getDataQualifiers(line, "sMapName_" + const_name));
                mapNamesQualified = true;
            }
        } else if (line.contains("MAPSEC")) {
            QRegularExpression reBefore("\\[(.*)\\]");
            QRegularExpression reAfter("{(.*)}");
            QStringList entry =  reAfter.match(line).captured(1).remove(" ").split(",");
            QString mapsec = reBefore.match(line).captured(1);
            QString insertion = entry[4].remove("sMapName_");
            qmap->insert(mapsec, sMapNamesMap.value(insertion));
            mapSecToMapEntry[mapsec] = {
            //  x                 y                 width             height            name
                entry[0].toInt(), entry[1].toInt(), entry[2].toInt(), entry[3].toInt(), insertion
            };
        } else if (line.contains("gRegionMapEntries")) {
            if (!mapEntriesQualified) {
                project->dataQualifiers.insert("region_map_entries", project->getDataQualifiers(line, "gRegionMapEntries"));
                mapEntriesQualified = true;
            }
        }
    }
    file.close();

    project->mapSecToMapHoverName = qmap;

    QFile binFile(region_map_layout_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) {
        logError(QString("Failed to read region map layout file %1").arg(region_map_layout_bin_path));
        return false;
    }
    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    for (int y = 0; y < layout_height_; y++) {
        for (int x = 0; x < layout_width_; x++) {
            int i = img_index_(x,y);
            if (i >= map_squares.size()) {
                continue;
            }
            int layoutIndex = layout_index_(x,y);
            if (layoutIndex >= mapBinData.size()) {
                continue;
            }
            uint8_t id = static_cast<uint8_t>(mapBinData.at(layoutIndex));
            map_squares[i].secid = id;
            QString secname = project->mapSectionValueToName.value(id);
            if (secname != "MAPSEC_NONE") {
                map_squares[i].has_map = true;
            }
            map_squares[i].mapsec = secname;
            map_squares[i].map_name = sMapNamesMap.value(mapSecToMapEntry.value(secname).name);
            map_squares[i].x = x;
            map_squares[i].y = y;
        }
    }
    return true;
}

void RegionMap::saveLayout() {
    QString entries_text;
    QString layout_text;

    entries_text += "#ifndef GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n";
    entries_text += "#define GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n\n";

    for (auto sName : sMapNames) {
        entries_text += QString("%1%2u8 sMapName_")
                        .arg(project->dataQualifiers.value("region_map_entries_names").isStatic ? "static " : "")
                        .arg(project->dataQualifiers.value("region_map_entries_names").isConst ? "const " : "")
                      + sName + "[] = _(\"" + sMapNamesMap.value(sName) + "\");\n";
    }

    entries_text += QString("\n%1%2struct RegionMapLocation gRegionMapEntries[] = {\n")
                    .arg(project->dataQualifiers.value("region_map_entries").isStatic ? "static " : "")
                    .arg(project->dataQualifiers.value("region_map_entries").isConst ? "const " : "");

    int longest = 1;
    for (auto sec : project->mapSectionNameToValue.keys()) {
        if (sec.length() > longest) longest = sec.length();
    }

    for (auto sec : project->mapSectionNameToValue.keys()) {
        if (!mapSecToMapEntry.contains(sec) || sec == "MAPSEC_NONE") continue;
        RegionMapEntry entry = mapSecToMapEntry.value(sec);
        entries_text += "    [" + sec + QString("]%1= {").arg(QString(" ").repeated(1 + longest - sec.length()))
            + QString::number(entry.x) + ", " + QString::number(entry.y) + ", " 
            + QString::number(entry.width) + ", " + QString::number(entry.height) + ", sMapName_" + entry.name + "},\n";
    }
    entries_text += "};\n\n#endif // GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n";

    project->saveTextFile(region_map_entries_path, entries_text);

    QByteArray data;
    for (int m = 0; m < layout_height_; m++) {
        for (int n = 0; n < layout_width_; n++) {
            int i = img_index_(n,m);
            data.append(map_squares[i].secid);
        }
    }
    QFile bfile(region_map_layout_bin_path);
    if (!bfile.open(QIODevice::WriteOnly)) return;
    bfile.write(data);
    bfile.close();
}

void RegionMap::saveOptions(int id, QString sec, QString name, int x, int y) {
    resetSquare(id);
    int index = getMapSquareIndex(x + this->padLeft, y + this->padTop);
    if (!sec.isEmpty()) {
        // Validate the input section name.
        if (!project->mapSectionNameToValue.contains(sec)) {
            sec = "MAPSEC_NONE";
            name = QString();
        }
        this->map_squares[index].has_map = sec == "MAPSEC_NONE" ? false : true;
        this->map_squares[index].secid = static_cast<uint8_t>(project->mapSectionNameToValue.value(sec));
        this->map_squares[index].mapsec = sec;
        if (!name.isEmpty()) {
            this->map_squares[index].map_name = name;
            this->project->mapSecToMapHoverName->insert(sec, name);
            QString sName = fixCase(sec);
            sMapNamesMap.insert(sName, name);
            if (!mapSecToMapEntry.keys().contains(sec)) {
                sMapNames.append(sName);
                RegionMapEntry entry(x, y, 1, 1, sName);
                mapSecToMapEntry.insert(sec, entry);
            }
        }
        this->map_squares[index].x = x;
        this->map_squares[index].y = y;
        this->map_squares[index].duplicated = false;
    }
}

void RegionMap::resetSquare(int index) {
    this->map_squares[index].mapsec = "MAPSEC_NONE";
    this->map_squares[index].map_name = QString();
    this->map_squares[index].has_map = false;
    this->map_squares[index].secid = static_cast<uint8_t>(project->mapSectionNameToValue.value("MAPSEC_NONE"));
    this->map_squares[index].has_city_map = false;
    this->map_squares[index].city_map_name = QString();
    this->map_squares[index].duplicated = false;
}

void RegionMap::clearLayout() {
    for (int i = 0; i < map_squares.size(); i++)
        resetSquare(i);
}

void RegionMap::clearImage() {
    for (int i = 0; i < map_squares.size(); i++)
        this->map_squares[i].tile_img_id = 0x00;
}

void RegionMap::replaceSectionId(unsigned oldId, unsigned newId) {
    for (auto &square : map_squares) {
        if (square.secid == oldId) {
            square.has_map = false;
            square.secid = newId;
            QString secname = project->mapSectionValueToName.value(newId);
            if (secname != "MAPSEC_NONE") square.has_map = true;
            square.mapsec = secname;
            square.map_name = sMapNamesMap.value(mapSecToMapEntry.value(secname).name);
        }
    }
}

void RegionMap::resize(int newWidth, int newHeight) {
    QVector<RegionMapSquare> new_squares;

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            RegionMapSquare square;
            if (x < img_width_ && y < img_height_) {
                square = map_squares[getMapSquareIndex(x, y)];
            } else if (x < newWidth - this->padRight && y < newHeight - this->padBottom) {
                square.tile_img_id = 0;
                square.x = x;
                square.y = y;
                square.mapsec = "MAPSEC_NONE";
            } else {
                square.tile_img_id = 0;
            }
            new_squares.append(square);
        }
    }
    this->map_squares = new_squares;
    this->img_width_ = newWidth;
    this->img_height_ = newHeight;
    this->layout_width_ = newWidth - this->padLeft - this->padRight;
    this->layout_height_ = newHeight - this->padTop - this->padBottom;
}

QVector<uint8_t> RegionMap::getTiles() {
    QVector<uint8_t> tileIds;
    for (auto square : map_squares) {
        tileIds.append(square.tile_img_id);
    }
    return tileIds;
}

void RegionMap::setTiles(QVector<uint8_t> tileIds) {
    if (tileIds.size() != map_squares.size()) return;

    int i = 0;
    for (uint8_t tileId : tileIds) {
        map_squares[i].tile_img_id = tileId;
        i++;
    }
}

// Layout coords to image index.
int RegionMap::img_index_(int x, int y) {
    return ((x + this->padLeft) + (y + this->padTop) * img_width_);
}

// Layout coords to layout index.
int RegionMap::layout_index_(int x, int y) {
    return (x + y * layout_width_);
}

int RegionMap::width() {
    return this->img_width_;
}

int RegionMap::height() {
    return this->img_height_;
}

QSize RegionMap::imgSize() {
    return QSize(img_width_ * 8, img_height_ * 8);
}

unsigned RegionMap::getTileId(int x, int y) {
    return map_squares.at(x + y * img_width_).tile_img_id;
}

QString RegionMap::pngPath() {
    return this->region_map_png_path;
}

void RegionMap::setTemporaryPngPath(QString path) {
    this->region_map_png_path = path;
    this->region_map_png_needs_saving = true;
}

QString RegionMap::cityTilesPath() {
    return this->city_map_tiles_path;
}

void RegionMap::setTemporaryCityTilesPath(QString path) {
    this->city_map_tiles_path = path;
    this->city_map_png_needs_saving = true;
}

// From x, y of image.
int RegionMap::getMapSquareIndex(int x, int y) {
    int index = (x + y * img_width_);
    return index < map_squares.length() ? index : 0;
}

// For turning a MAPSEC_NAME into a unique identifier sMapName-style variable.
// CAPS_WITH_UNDERSCORE to CamelCase
QString RegionMap::fixCase(QString caps) {
    bool big = true;
    QString camel;

    for (auto ch : caps.remove(QRegularExpression("({.*})")).remove("MAPSEC")) {
        if (ch == '_' || ch == ' ') {
            big = true;
            continue;
        }
        if (big) {
            camel += ch.toUpper();
            big = false;
        }
        else camel += ch.toLower();
    }
    return camel;
}

void RegionMapEntry::setX(const int val) {
    this->x = val;
}

void RegionMapEntry::setY(int val) {
    this->y = val;
}

void RegionMapEntry::setWidth(int val) {
    this->width = val;
}

void RegionMapEntry::setHeight(int val) {
    this->height = val;
}
