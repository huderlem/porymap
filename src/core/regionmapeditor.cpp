#include "regionmapeditor.h"

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

void RegionMap::init(Project *pro) {
    QString path = pro->root;
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

    readBkgImgBin();
    readLayout(pro->mapConstantsToMapNames);
    readCityMaps();

    //tryGetMap();

    //saveBkgImgBin();
    //saveLayout();
    test(pro->mapConstantsToMapNames);
}

// as of now, this needs to be called first because it initializes all the
// RegionMapSquare s in the list
// TODO: if the tileId is not valid for the provided image, make sure it does not crash
void RegionMap::readBkgImgBin() {
    QFile binFile(region_map_bin_path);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    QByteArray mapBinData = binFile.readAll();
    binFile.close();

    // the two is because lines are skipped for some reason
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

// done
void RegionMap::readLayout(QMap<QString, QString> *qmap) {
    QFile file(region_map_layout_path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QMap<QString, QString> * abbr = new QMap<QString, QString>;

    QString line, text;
    QStringList *captured = new QStringList;

    QTextStream in(&file);
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("#define")) {
            QStringList split = line.split(QRegularExpression("\\s+"));
            abbr->insert(split[2].replace("MAPSEC_",""), split[1]);
        } else {
            text += line.remove(" ");
        }
    }
    QRegularExpression re("{(.*?)}");
    *captured = re.match(text).captured(1).split(",");
    captured->removeAll({});

    // replace abbreviations with names
    for (int i = 0; i < captured->length(); i++) {
        QString value = (*captured)[i];
        if (value.startsWith("R(")) {// routes are different
            captured->replace(i, QString("ROUTE_%1").arg(value.mid(2,3)));
        } else {
            captured->replace(i, abbr->key(value));
        }
    }

    // TODO: improve this?
    for (int m = 0, i = 0; m < layout_height_; m++) {
        for (int n = 0; n < layout_width_; n++) {
            i = img_index_(n,m);
            QString secname = (*captured)[layout_index_(n,m)];
            if (secname != "NOTHING") map_squares[i].has_map = true;
            map_squares[i].map_name = qmap->value(mapSecToMapConstant(secname));
            map_squares[i].x = n;
            map_squares[i].y = m;
        }
    }
    mapname_abbr = abbr;
    layout_map_names = captured;
    file.close();
}

// does it matter that it doesn't save in order?
// do i need to use a QList<Pair> ??
void RegionMap::saveLayout() {
    //
    QString layout_text = "";
    QString mapsec      = "MAPSEC_";
    QString define      = "#define ";
    QString array_start = "static const u8 sRegionMapLayout[] =\n{";
    QString array_close = "\n};\n";
    QString tab         = "    ";

    for (QString key : mapname_abbr->keys()) {
        layout_text += define + mapname_abbr->value(key) + tab + mapsec + key + "\n";
    }

    layout_text += "\n" + array_start;// +  + array_close;//oops

    //qDebug() << *layout_map_names;
    int cnt = 0;
    for (QString s : *layout_map_names) {
        //
        if (!(cnt % layout_width_)) {
            layout_text += "\n" + tab;
        }
        if (s.startsWith("ROUTE_")) {
            layout_text += QString("R(%1)").arg(s.replace("ROUTE_","")) + ", ";
        } else {
            layout_text += mapname_abbr->value(s) + ", ";
        }
        cnt++;
    }

    //layout_text.
    layout_text += array_close;
    
    QFile file(region_map_layout_path);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(layout_text.toUtf8());
    file.close();
}

void RegionMap::readCityMaps() {
    //
    //for (int m = 0; m < layout_height_; m++) {
    //    QString tester;
    //    for (int n = 0; n < layout_width_; n++) {
    //        tester += (QString::number(img_index_(n,m)).rightJustified(3, '.') + " ");
    //    }
    //    qDebug() << tester;
    //}
    //for (auto map : map_squares) {
    for (int map = 0; map < map_squares.size(); map++) {
        //
        if (map_squares[map].has_map) {
            //
            if (ruby_city_maps_.contains(map_squares[map].map_name)) {
                map_squares[map].has_city_map = true;
                //map_squares[map].city_map_name = ruby_city_maps_.value(map_squares[map].map_name)[0].tilemap;
                QList<struct CityMapPosition> city_maps = ruby_city_maps_.value(map_squares[map].map_name);
                for (auto city_map : city_maps) {
                    //
                    if (city_map.x == map_squares[map].x
                     && city_map.y == map_squares[map].y)
                        //
                        map_squares[map].city_map_name = city_map.tilemap;
                }
            }
        }
    }
}

//done
QString RegionMap::newAbbr(QString mapname) {
    QString abbr;
    QStringList words = mapname.split("_");

    if (words.length() == 1) {
        abbr = (words[0] + "_____X").left(6);
    } else {
        abbr = (words.front() + "___X").left(4) + "_" + words.back().at(0);
    }

    // to guarantee unique abbreviations (up to 14)
    QString extra_chars = "23456789BCDEF";
    int count = 0;
    while ((*mapname_abbr).values().contains(abbr)) {
        abbr.replace(5,1,extra_chars[count]);
        count++;
    }
    return abbr;
}

// layout coords to image index
int RegionMap::img_index_(int x, int y) {
    return ((x + 1) + (y + 2) * img_width_);
}

// layout coords to layout index
int RegionMap::layout_index_(int x, int y) {
    return (x + y * layout_width_);
}

// img coords to layout index?
// img coords to img index?

void RegionMap::test(QMap<QString, QString>* qmap) {
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
            //if (qmap->contains(mapSecToMapConstant(square.map_name)))
            //    extras += qmap->value(mapSecToMapConstant(square.map_name)) + " ";
            //else
            //    extras += "nothing ";
        }

        QPixmap png(region_map_png_path);
        //png.load(region_map_png_path);
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
    //qDebug() << x << y;
    //return 0;
}

// sidenote: opening the map from MAPSEC_x will not always be right
// there needs to be a mapsections to mapname QMap
// otherwie, look for the first map with right substring
// mapConstantsToMapNames [MAP_ROUTE106] = "Route106"
// eg. SOUTHERN_ISLAND -> MAP_SOUTHERN_ISLAND -> SouthernIsland(n) -> SouthernIsland_Exterior
//     MT_CHIMNEY      -> MAP_MT_CHIMNEY      -> MtChimney(y)
//     ROUTE_101       -> MAP_ROUTE101        -> Route101(y)
// (or synchronize these for consistency in the repos :: underscore / no underscore)

// TODO: change debugs to logs
void RegionMap::save() {
    // 
    qDebug() << "saving region map image tilemap at" << region_map_bin_path << "\n"
             ;//<< "saving region map layout at" << region_map_layout_path << "\n";

    saveBkgImgBin();
}

































