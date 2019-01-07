#include "regionmap.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

#include <QImage>



// TODO: add logging / config stuff

// IN: ROUTE_101 ... OUT: MAP_ROUTE101
// eg. SOUTHERN_ISLAND -> MAP_SOUTHERN_ISLAND -> SouthernIsland(n) -> SouthernIsland_Exterior
//     MT_CHIMNEY      -> MAP_MT_CHIMNEY      -> MtChimney(y)
//     ROUTE_101       -> MAP_ROUTE101        -> Route101(y)
// TODO: move this maybe? would I be able to call it from this file if it was in map.cpp?
QString RegionMap::mapSecToMapConstant(QString mapSectionName) {
    //
    QString mapConstantName = "MAP_";
    QString sectionNameTemp = mapSectionName.replace("ROUTE_","ROUTE");
    mapConstantName += sectionNameTemp;
    return mapConstantName;
}

// TODO: verify these are in the correct order
// also TODO: read this from the project somehow
QMap<QString, QList<struct CityMapPosition>> RegionMap::ruby_city_maps_ = QMap<QString, QList<struct CityMapPosition>>({
    {"LavaridgeTown", {
        {"lavaridge_0.bin", 5, 3},
    }},
    {"FallarborTown", {
        {"fallarbor_0.bin", 3, 0},
    }},
    {"FortreeCity", {
        {"fortree_0.bin", 12, 0},
    }},
    {"SlateportCity", {
        {"slateport_0.bin", 8, 10},
        {"slateport_1.bin", 8, 11},
    }},
    {"RustboroCity", {
        {"rustboro_0.bin", 0, 5},
        {"rustboro_1.bin", 0, 6},
    }},
    {"PacifidlogTown", {
        {"pacifidlog_0.bin", 17, 10},
    }},
    {"MauvilleCity", {
        {"mauville_0.bin", 8, 6},
        {"mauville_1.bin", 9, 6},
    }},
    {"OldaleTown", {
        {"oldale_0.bin", 4, 9},
    }},
    {"LilycoveCity", {
        {"lilycove_0.bin", 18, 3},
        {"lilycove_1.bin", 19, 3},
    }},
    {"LittlerootTown", {
        {"littleroot_0.bin", 4, 11},
    }},
    {"DewfordTown", {
        {"dewford_0.bin", 2, 14},
    }},
    {"SootopolisCity", {
        {"sootopolis_0.bin", 21, 7},
    }},
    {"EverGrandeCity", {
        {"ever_grande_0.bin", 27, 8},
        {"ever_grande_1.bin", 27, 9},
    }},
    {"VerdanturfTown", {
        {"verdanturf_0.bin", 4, 6},
    }},
    {"MossdeepCity", {
        {"mossdeep_0.bin", 24, 5},
        {"mossdeep_1.bin", 25, 5},
    }},
    {"PetalburgCity", {
        {"petalburg_0.bin", 1, 9},
    }},
});

// TODO: add version arg to this from Editor Setings
void RegionMap::init(Project *pro) {
    QString path = pro->root;
    this->project = pro;
    //
    // TODO: in the future, allow these to be adjustable (and save values)
    // possibly use a config file?
    layout_width_ = 28;
    layout_height_ = 15;

    img_width_ = layout_width_ + 4;
    img_height_ = layout_height_ + 5;

    //city_map_squares_path = QString();
    temp_path = path;// delete this
    region_map_bin_path = path + "/graphics/pokenav/region_map_map.bin";
    region_map_png_path = path + "/graphics/pokenav/region_map.png";
    region_map_layout_path = path + "/src/data/region_map_layout.h";
    region_map_entries_path = path + "/src/data/region_map/region_map_entries.h";
    region_map_layout_bin_path = path + "/graphics/pokenav/region_map_section_layout.bin";
    region_map_city_map_tiles_path = path + "/graphics/pokenav/zoom_tiles.png";// TODO: rename png to map_squares in pokeemerald

    readBkgImgBin();
    readLayout();
    readCityMaps();
}

// as of now, this needs to be called first because it initializes all the
// `RegionMapSquare`s in the list
// TODO: if the tileId is not valid for the provided image, make sure it does not crash
void RegionMap::readBkgImgBin() {
    QFile binFile(region_map_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    // the two multiplier is because lines are skipped for some reason
    // (maybe that is because there could be multiple layers?)
    // background image is also 32x20
    for (int m = 0; m < img_height_; m++) {
        for (int n = 0; n < img_width_; n++) {
            RegionMapSquare square;// = 
            square.tile_img_id = mapBinData.at(n + m * img_width_ * 2);
            map_squares.append(square);
        }
    }
}

void RegionMap::saveBkgImgBin() {
    QByteArray data(4096,0);// use a constant here?  maybe read the original size?

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

// TODO: reorganize this into project? the i/o stuff. use regionMapSections
void RegionMap::readLayout() {
    //
    QFile file(region_map_entries_path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QString line;
    // TODO: put these in Project, and keep in order
    //QMap<QString, QString> sMapNames;// {"sMapName_LittlerootTown" : "LITTLEROOT{NAME_END} TOWN"}
    //QMap<QString, QString> mapSecToMapName;// {"MAPSEC_LITTLEROOT_TOWN" : "LITTLEROOT{NAME_END} TOWN"}
    //QList<> mapSecToMapEntry;// {"MAPSEC_LITTLEROOT_TOWN" : }

    // new map ffor mapSecToMapHoverName
    QMap<QString, QString> *qmap = new QMap<QString, QString>;

    QTextStream in(&file);
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("static const u8")) {
            QRegularExpression reBefore("sMapName_(.*)\\[");
            QRegularExpression reAfter("_\\(\"(.*)\"");
            QString const_name = reBefore.match(line).captured(1);
            QString full_name = reAfter.match(line).captured(1);
            sMapNames.insert(const_name, full_name);
        } else if (line.contains("MAPSEC")) {
            QRegularExpression reBefore("\\[(.*)\\]");
            QRegularExpression reAfter("{(.*)}");
            QStringList entry =  reAfter.match(line).captured(1).remove(" ").split(",");
            QString mapsec = reBefore.match(line).captured(1);
            QString insertion = entry[4].remove("sMapName_");
            qmap->insert(mapsec, sMapNames[insertion]);
            // can make this a map, the order doesn't really matter
            mapSecToMapEntry[mapsec] = 
            //   x                 y                 width             height            name
                {entry[0].toInt(), entry[1].toInt(), entry[2].toInt(), entry[3].toInt(), insertion}
            ;
            // ^ when loading this info to city maps, loop over mapSecToMapEntry and
            // add x and y map sqyare when width or height >1
            // indexOf because mapsecs is just a qstringlist
            //text += line.remove(" ");
        }
    }
    file.close();

    project->mapSecToMapHoverName = qmap;

    QFile binFile(region_map_layout_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;
    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    // TODO: improve this?
    for (int m = 0; m < layout_height_; m++) {
        for (int n = 0; n < layout_width_; n++) {
            int i = img_index_(n,m);
            map_squares[i].secid = static_cast<uint8_t>(mapBinData.at(layout_index_(n,m)));
            QString secname = (*(project->regionMapSections))[static_cast<uint8_t>(mapBinData.at(layout_index_(n,m)))];
            if (secname != "MAPSEC_NONE") map_squares[i].has_map = true;
            map_squares[i].mapsec = secname;
            map_squares[i].map_name = sMapNames.value(mapSecToMapEntry.value(secname).name);
            map_squares[i].x = n;
            map_squares[i].y = m;
        }
    }
}

/// saves:
// region_map_entries_path
// region_map_layout_bin_path (layout as tilemap instead of how it is in ruby)
// done
// TODO: consider keeping QMaps in order
void RegionMap::saveLayout() {
    QString entries_text;
    QString layout_text;

    entries_text += "#ifndef GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n";
    entries_text += "#define GUARD_DATA_REGION_MAP_REGION_MAP_ENTRIES_H\n\n";

    // note: this doesn't necessarily keep order because it is a QMap
    for (auto it : this->project->mapSecToMapHoverName->keys()) {
        entries_text += "static const u8 sMapName_" + fix_case(it) + "[] = _(\"" + this->project->mapSecToMapHoverName->value(it) + "\");\n";
    }

    entries_text += "\nconst struct RegionMapLocation gRegionMapEntries[] = {\n";

    for (auto sec : mapSecToMapEntry.keys()) {
        struct RegionMapEntry entry = mapSecToMapEntry.value(sec);
        entries_text += "    [" + sec + "] = {" + QString::number(entry.x) + ", " + QString::number(entry.y) + ", " 
            +  QString::number(entry.width) + ", " + QString::number(entry.height) + ", sMapName_" + fix_case(sec) + "},\n";//entry.name
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

// beyond broken
void RegionMap::readCityMaps() {
    //
    for (int map = 0; map < map_squares.size(); map++) {
        //
        if (map_squares[map].has_map) {
            //
            if (ruby_city_maps_.contains(map_squares[map].map_name)) {
                //map_squares[map].has_city_map = true;
                //map_squares[map].city_map_name = ruby_city_maps_.value(map_squares[map].map_name)[0].tilemap;
                QList<struct CityMapPosition> city_maps = ruby_city_maps_.value(map_squares[map].map_name);
                for (auto city_map : city_maps) {
                    //
                    if (city_map.x == map_squares[map].x
                     && city_map.y == map_squares[map].y)
                        //
                        map_squares[map].has_city_map = true;
                        map_squares[map].city_map_name = city_map.tilemap;
                }
            }
        }
    }
}

// layout coords to image index
int RegionMap::img_index_(int x, int y) {
    return ((x + 1) + (y + 2) * img_width_);
}

// layout coords to layout index
int RegionMap::layout_index_(int x, int y) {
    return (x + y * layout_width_);
}

void RegionMap::test() {
    //
    bool debug_rmap = false;

    if (debug_rmap) {
        for (auto square : map_squares) {
            qDebug() << "(" << square.x << "," << square.y << ")"
                     << square.tile_img_id
                     << square.has_map
                     << square.map_name
                     << square.has_city_map
                     << square.city_map_name
                     ;
        }

        QPixmap png(region_map_png_path);
        qDebug() << "png num 8x8 tiles" << QString("0x%1").arg((png.width()/8) * (png.height() / 8), 2, 16, QChar('0'));
    }
}

int RegionMap::width() {
    return this->img_width_;
}

int RegionMap::height() {
    return this->img_height_;
}

// TODO: remove +2 and put elsewhere
QSize RegionMap::imgSize() {
    return QSize(img_width_ * 8 + 2, img_height_ * 8 + 2);
}

// TODO: rename to getTileIdAt()?
unsigned RegionMap::getTileId(int x, int y) {
    //
    return map_squares[x + y * img_width_].tile_img_id;
}

// TODO: change debugs to logs
void RegionMap::save() {
    // 
    qDebug() << "saving region map image tilemap at" << region_map_bin_path << "\n"
             << "saving region map layout at" << region_map_layout_path << "\n"
            ;

    saveBkgImgBin();
    saveLayout();
}

// save Options (temp)
void RegionMap::saveOptions(int index, QString sec, QString name, int x, int y) {
    //
    // TODO:req need to reindex in city_maps if changing x and y
    // TODO: save [sec] sMapName_ properly
    // so instead of taking index, maybe go by img_index_(x,y)
    if (!sec.isEmpty()) {
        this->map_squares[index].has_map = true;
        this->map_squares[index].secid = static_cast<uint8_t>(project->regionMapSections->indexOf(sec));
    }
    this->map_squares[index].mapsec = sec;
    if (!name.isEmpty()) {
        this->map_squares[index].map_name = name;// TODO: display in editor with this map & remove this field
        this->project->mapSecToMapHoverName->insert(sec, name);
    }
    this->map_squares[index].x = x;
    this->map_squares[index].y = y;
}

// from x, y of image
// TODO: make sure this returns a valid index
int RegionMap::getMapSquareIndex(int x, int y) {
    //
    int index = (x + y * img_width_);
    return index < map_squares.length() - 1 ? index : 0;
}

// For turning a MAPSEC_NAME into a unique identifier sMapName-style variable.
QString RegionMap::fix_case(QString caps) {
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
































