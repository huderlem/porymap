#include "regionmap.h"
#include "regionmapeditor.h"
#include "paletteutil.h"
#include "project.h"
#include "log.h"
#include "config.h"
#include "regionmapeditcommands.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QImage>
#include <math.h>

using std::make_shared;



RegionMap::RegionMap(Project *project) : 
    section_prefix(projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix)),
    default_map_section(project->getEmptyMapsecName())
{
    this->project = project;
}

bool RegionMap::loadMapData(poryjson::Json data) {
    poryjson::Json::object mapObject = data.object_items();

    this->alias = mapObject["alias"].string_value();

    poryjson::Json tilemapJson = mapObject["tilemap"];
    poryjson::Json layoutJson = mapObject["layout"];

    this->tilemap.clear();
    this->layout_layers.clear();
    this->layouts.clear();

    return loadTilemap(tilemapJson) && loadLayout(layoutJson);
}

int RegionMap::tilemapBytes() {
    // bytes per tile multiplier
    int multiplier = 1;
    
    switch (tilemap_format) {
        case TilemapFormat::Plain:
            multiplier = 1;
            break;
        case TilemapFormat::BPP_4:
            multiplier = 2;
            break;
        case TilemapFormat::BPP_8:
            multiplier = 2;
            break;
    }

    return tilemapSize() * multiplier;
}

bool RegionMap::loadTilemap(poryjson::Json tilemapJson) {
    bool errored = false;

    poryjson::Json::object tilemapObject = tilemapJson.object_items();

    this->tilemap_width = tilemapObject["width"].int_value();
    this->tilemap_height = tilemapObject["height"].int_value();


    QString tilemapFormat = tilemapObject["format"].string_value();
    QMap<QString, TilemapFormat> formatsMap = { {"plain", TilemapFormat::Plain}, {"4bpp", TilemapFormat::BPP_4}, {"8bpp", TilemapFormat::BPP_8} };
    this->tilemap_format = formatsMap[tilemapFormat];

    this->tileset_path = tilemapObject["tileset_path"].string_value();
    this->tilemap_path = tilemapObject["tilemap_path"].string_value();

    if (tilemapObject.contains("palette")) {
        this->palette_path = tilemapObject["palette"].string_value();
    }

    QImage tilesetFile(fullPath(this->tileset_path));
    if (tilesetFile.isNull()) {
        logError(QString("Failed to open region map tileset file '%1'.").arg(tileset_path));
        return false;
    }

    if (tilesetFile.width() < 8 || tilesetFile.height() < 8) {
        logError(QString("Region map tileset file '%1' must be at least 8x8.").arg(tileset_path));
        return false;
    }

    QFile tilemapFile(fullPath(this->tilemap_path));
    if (!tilemapFile.open(QIODevice::ReadOnly)) {
        logError(QString("Failed to open region map tilemap file '%1'.").arg(tilemap_path));
        return false;
    }

    if (tilemapFile.size() < tilemapBytes()) {
        logError(QString("The region map tilemap at '%1' is too small.").arg(tilemap_path));
        return false;
    }

    QByteArray newTilemap = tilemapFile.readAll();
    this->setTilemap(newTilemap);
    
    tilemapFile.close();

    return !errored;
}

bool RegionMap::loadLayout(poryjson::Json layoutJson) {
    if (layoutJson.is_null()) {
        this->layout_format = LayoutFormat::None;
        return true;
    }

    this->layout_constants.clear();

    poryjson::Json::object layoutObject = layoutJson.object_items();

    QString layoutFormat = layoutObject["format"].string_value();
    QMap<QString, LayoutFormat> layoutFormatMap = { {"binary", LayoutFormat::Binary}, {"C array", LayoutFormat::CArray} };
    this->layout_format = layoutFormatMap[layoutFormat];

    this->layout_path = layoutObject["path"].string_value();
    this->layout_width = layoutObject["width"].int_value();
    this->layout_height = layoutObject["height"].int_value();

    this->offset_left = layoutObject["offset_left"].int_value();
    this->offset_top = layoutObject["offset_top"].int_value();

    bool errored = false;

    switch (this->layout_format) {
        case LayoutFormat::Binary:
        {
            // only one layer supported for binary layouts
            QFile binFile(fullPath(this->layout_path));
            if (!binFile.open(QIODevice::ReadOnly)) {
                logError(QString("Failed to read region map layout binary file %1").arg(this->layout_path));
                return false;
            }
            QByteArray mapBinData = binFile.readAll();
            binFile.close();

            if (mapBinData.size() != this->layout_width * this->layout_height) {
                logError("Region map layout file size does not match given dimensions for " + this->alias);
                return false;
            }

            // for layouts with only a single layer, it is called main
            this->layout_layers.append("main");
            QList<LayoutSquare> layout;

            for (int y = 0; y < this->layout_height; y++) {
                for (int x = 0; x < this->layout_width; x++) {
                    int bin_index = x + y * this->layout_width;
                    uint8_t square_section_id = mapBinData.at(bin_index);
                    QString square_section_name = project->getLocationName(square_section_id);

                    LayoutSquare square;
                    square.map_section = square_section_name;
                    square.has_map = (square_section_name != this->default_map_section && !square_section_name.isEmpty());
                    square.x = x;
                    square.y = y;

                    layout.append(square);
                }
            }
            setLayout("main", layout);
            break;
        }
        case LayoutFormat::CArray:
        {
            ParseUtil parser;
            QString text = parser.readTextFile(fullPath(this->layout_path));

            static const QRegularExpression re("(?<qual_1>static)?\\s?(?<qual_2>const)?\\s?(?<type>[A-Za-z0-9_]+)?\\s+(?<label>[A-Za-z0-9_]+)"
                "(\\[(?<const_1>[A-Za-z0-9_]+)\\])(\\[(?<const_2>[A-Za-z0-9_]+)\\])(\\[(?<const_3>[A-Za-z0-9_]+)\\])\\s+=");

            // check for layers, extract info
            QRegularExpressionMatch match = re.match(text);
            if (match.hasMatch()) {
                QString qualifiers = match.captured("qual_1") + " " + match.captured("qual_2");
                QString type = match.captured("type");
                QString label = match.captured("label");
                QStringList constants;
                if (!match.captured("const_1").isNull()) constants.append(match.captured("const_1"));
                if (!match.captured("const_2").isNull()) constants.append(match.captured("const_2"));
                if (!match.captured("const_3").isNull()) constants.append(match.captured("const_3"));
                this->layout_constants = constants;
                this->layout_qualifiers = qualifiers + " " + type;
                this->layout_array_label = label;

                // find layers
                static const QRegularExpression reLayers("(?<layer>\\[(?<label>LAYER_[A-Za-z0-9_]+)\\][^\\[\\]]+)");
                QRegularExpressionMatchIterator i = reLayers.globalMatch(text);
                while (i.hasNext()) {
                    QRegularExpressionMatch m = i.next();

                    QString layerName = m.captured("label");
                    QString layerLayout = m.captured("layer");

                    static const QRegularExpression rowRe("{(?<row>[A-Z0-9_, ]+)}");
                    QRegularExpressionMatchIterator j = rowRe.globalMatch(layerLayout);

                    this->layout_layers.append(layerName);
                    QList<LayoutSquare> layout;

                    int y = 0;
                    while (j.hasNext()) {
                        QRegularExpressionMatch n = j.next();
                        QString row = n.captured("row");
                        QStringList rowSections = row.split(',');
                        int x = 0;
                        for (QString section : rowSections) {
                            QString square_section_name = section.trimmed();

                            LayoutSquare square;
                            square.map_section = square_section_name;
                            square.has_map = (square_section_name != this->default_map_section && !square_section_name.isEmpty());
                            square.x = x;
                            square.y = y;
                            layout.append(square);
                            x++;
                        }
                        y++;
                    }
                    setLayout(layerName, layout);
                }

            } else {
                // try single-layered
                static const QRegularExpression reAlt("(?<qual_1>static)?\\s?(?<qual_2>const)?\\s?(?<type>[A-Za-z0-9_]+)?\\s+(?<label>[A-Za-z0-9_]+)\\[\\]");
                QRegularExpressionMatch matchAlt = reAlt.match(text);
                if (matchAlt.hasMatch()) {
                    // single dimensional
                    QString qualifiers = matchAlt.captured("qual_1") + " " + matchAlt.captured("qual_2");
                    QString type = matchAlt.captured("type");
                    QString label = matchAlt.captured("label");
                    this->layout_constants.append("");
                    this->layout_qualifiers = qualifiers + " " + type;
                    this->layout_array_label = label;

                    static const QRegularExpression reSec(QString("(?<sec>%1[A-Za-z0-9_]+)").arg(this->section_prefix));
                    QRegularExpressionMatchIterator k = reSec.globalMatch(text);

                    QList<LayoutSquare> layout;
                    int l = 0;
                    while (k.hasNext()) {
                        QRegularExpressionMatch p = k.next();
                        QString sec = p.captured("sec");

                        int x = l % this->layout_width;
                        int y = l / this->layout_width;

                        LayoutSquare square;
                        square.map_section = sec;
                        square.has_map = (sec != this->default_map_section && !sec.isEmpty());
                        square.x = x;
                        square.y = y;
                        layout.append(square);

                        l++;
                    }
                    this->layout_layers.append("main");
                    setLayout("main", layout);
                } else {
                    static const QRegularExpression reAlt2("(?<qual_1>static)?\\s?(?<qual_2>const)?\\s?(?<type>[A-Za-z0-9_]+)?\\s+(?<label>[A-Za-z0-9_]+)"
                                  "(\\[(?<const_1>[A-Za-z0-9_]+)\\])(\\[(?<const_2>[A-Za-z0-9_]+)\\])\\s+=");
                    QRegularExpressionMatch matchAlt2 = reAlt2.match(text);
                    if (matchAlt2.hasMatch()) {
                        // double dimensional
                        QString qualifiers = matchAlt2.captured("qual_1") + " " + matchAlt2.captured("qual_2");
                        QString type = matchAlt2.captured("type");
                        QString label = matchAlt2.captured("label");
                        QStringList constants;
                        constants.append(matchAlt2.captured("const_1"));
                        constants.append(matchAlt2.captured("const_2"));
                        this->layout_constants = constants;
                        this->layout_qualifiers = qualifiers + " " + type;
                        this->layout_array_label = label;

                        static const QRegularExpression rowRe("{(?<row>[A-Z0-9_, ]+)}");
                        QRegularExpressionMatchIterator k = rowRe.globalMatch(text);

                        this->layout_layers.append("main");
                        QList<LayoutSquare> layout;

                        int y = 0;
                        while (k.hasNext()) {
                            QRegularExpressionMatch n = k.next();
                            QString row = n.captured("row");
                            QStringList rowSections = row.split(',');
                            int x = 0;
                            for (QString section : rowSections) {
                                QString square_section_name = section.trimmed();
                                LayoutSquare square;
                                square.map_section = square_section_name;
                                square.has_map = (square_section_name != this->default_map_section && !square_section_name.isEmpty());
                                square.x = x;
                                square.y = y;
                                layout.append(square);
                                x++;
                            }
                            y++;
                        }
                        setLayout("main", layout);
                    } else {
                        logError(QString("Failed to read region map layout from '%1'.").arg(this->layout_path));
                        return false;
                    }
                }
            }
            break;
        }
        case LayoutFormat::None:
        default:
            break;
    }
    if (this->layout_layers.isEmpty()) {
        logError("Encountered an error parsing the region map layout.");
        errored = true;
    } else {
        this->current_layer = this->layout_layers.first();
    }

    return !errored;
}

void RegionMap::commit(QUndoCommand *command) {
    editHistory.push(command);
}

void RegionMap::undo() {
    editHistory.undo();
}

void RegionMap::redo() {
    editHistory.redo();
}

void RegionMap::save() {
    saveTilemap();
    saveLayout();

    this->editHistory.setClean();
}

poryjson::Json::object RegionMap::config() {
    poryjson::Json::object config;

    config["alias"] = this->alias;

    poryjson::Json::object tilemapObject;
    tilemapObject["width"] = this->tilemap_width;
    tilemapObject["height"] = this->tilemap_height;

    QMap<TilemapFormat, QString> tilemapFormatMap = { {TilemapFormat::Plain, "plain"}, {TilemapFormat::BPP_4, "4bpp"}, {TilemapFormat::BPP_8, "8bpp"} };
    tilemapObject["format"] = tilemapFormatMap[this->tilemap_format];
    tilemapObject["tileset_path"] = this->tileset_path;
    tilemapObject["tilemap_path"] = this->tilemap_path;
    if (!this->palette_path.isEmpty()) {
        tilemapObject["palette"] = this->palette_path;
    }
    config["tilemap"] = tilemapObject;

    if (this->layout_format != LayoutFormat::None) {
        poryjson::Json::object layoutObject;
        layoutObject["width"] = this->layout_width;
        layoutObject["height"] = this->layout_height;
        layoutObject["offset_left"] = this->offset_left;
        layoutObject["offset_top"] = this->offset_top;
        static QMap<LayoutFormat, QString> layoutFormatMap = { {LayoutFormat::Binary, "binary"}, {LayoutFormat::CArray, "C array"} };
        layoutObject["format"] = layoutFormatMap[this->layout_format];
        layoutObject["path"] = this->layout_path;
        config["layout"] = layoutObject;
    } else {
        config["layout"] = nullptr;
    }

    return config;
}

void RegionMap::saveTilemap() {
    QFile tilemapFile(fullPath(this->tilemap_path));
    if (!tilemapFile.open(QIODevice::WriteOnly)) {
        logError(QString("Failed to open region map tilemap file %1 for writing.").arg(this->tilemap_path));
        return;
    }
    tilemapFile.write(this->getTilemap());
    tilemapFile.close();
}

void RegionMap::saveLayout() {
    switch (this->layout_format) {
        case LayoutFormat::Binary:
        {
            QByteArray data;
            int defaultValue = this->project->getLocationValue(this->default_map_section);
            for (int m = 0; m < this->layout_height; m++) {
                for (int n = 0; n < this->layout_width; n++) {
                    int i = n + this->layout_width * m;
                    int mapSectionValue = this->project->getLocationValue(this->layouts["main"][i].map_section);
                    if (mapSectionValue < 0){
                        mapSectionValue = defaultValue;
                    }
                    data.append(mapSectionValue);
                }
            }
            QFile bfile(fullPath(this->layout_path));
            if (!bfile.open(QIODevice::WriteOnly)) {
                logError("Failed to open region map layout binary for writing.");
            }
            bfile.write(data);
            bfile.close();
            break;
        }
        case LayoutFormat::CArray:
        {
            QString text = QString("%1 %2").arg(this->layout_qualifiers).arg(this->layout_array_label);
            for (QString label : this->layout_constants) {
                text += QString("[%1]").arg(label);
            }
            text += " = {\n";
            if (this->layout_layers.size() == 1) {
                if (this->layout_constants.count() == 1) {
                    for (LayoutSquare s : this->getLayout("main")) {
                        text += "    " + s.map_section + ",\n";
                    }
                    text.chop(2);
                    text += "\n";
                } else if (this->layout_constants.count() == 2) {
                    for (int row = 0; row < this->layout_height; row++) {
                        text += "    {";
                        for (int col = 0; col < this->layout_width; col++) {
                            int i = col + row * this->layout_width;
                            text += this->layouts["main"][i].map_section + ", ";
                        }
                        text.chop(2);
                        text += "},\n";
                    }
                } else {
                    logError(QString("Failed to save region map layout for %1").arg(this->layout_array_label));
                    return;
                }
            } else {
                // multi layered
                for (auto layoutName : this->layout_layers) {
                    text += QString("    [%1] =\n    {\n").arg(layoutName);
                    for (int row = 0; row < this->layout_height; row++) {
                        text += "        {";
                        for (int col = 0; col < this->layout_width; col++) {
                            int i = col + row * this->layout_width;
                            text += this->layouts[layoutName][i].map_section + ", ";
                        }
                        text.chop(2);
                        text += "},\n";
                    }
                    text += "    },\n";
                }
                text.chop(2);
                text += "\n";
            }
            text += "};\n";
            this->project->saveTextFile(fullPath(this->layout_path), text);
            break;
        }
        case LayoutFormat::None:
        default:
            break;
    }
}

void RegionMap::resetSquare(int index) {
    this->layouts[this->current_layer][index].map_section = this->default_map_section;
    this->layouts[this->current_layer][index].has_map = false;
}

void RegionMap::clearLayout() {
    for (int i = 0; i < this->layout_width * this->layout_height; i++) {
        resetSquare(i);
    }
}

void RegionMap::clearImage() {
    QByteArray zeros(this->tilemapSize(), 0);
    this->setTilemap(zeros);
}

void RegionMap::replaceSection(QString oldSection, QString newSection) {
    for (auto &square : this->layouts[this->current_layer]) {
        if (square.map_section == oldSection) {
            square.map_section = newSection;
            square.has_map = (newSection != this->default_map_section);
        }
    }
}

void RegionMap::swapSections(QString secA, QString secB) {
    for (auto &square : this->layouts[this->current_layer]) {
        if (square.map_section == secA) {
            square.map_section = secB;
            square.has_map = (square.map_section != this->default_map_section);
        }
        else if (square.map_section == secB) {
            square.map_section = secA;
            square.has_map = (square.map_section != this->default_map_section);
        }
    }
}

void RegionMap::resizeTilemap(int newWidth, int newHeight, bool update) {
    auto tilemapCopy = this->tilemap;
    int oldWidth = this->tilemap_width;
    int oldHeight = this->tilemap_height;
    this->tilemap_width = newWidth;
    this->tilemap_height = newHeight;

    if (update) {
        QByteArray tilemapArray;
        QDataStream dataStream(&tilemapArray, QIODevice::WriteOnly);
        dataStream.setByteOrder(QDataStream::LittleEndian);
        switch (this->tilemap_format) {
            case TilemapFormat::Plain:
                for (int y = 0; y < newHeight; y++)
                for (int x = 0; x < newWidth; x++) {
                    if (y < oldHeight && x < oldWidth) {
                        int i = x + y * oldWidth;
                        uint8_t tile = tilemapCopy[i]->raw();
                        dataStream << tile;
                    } else {
                        uint8_t tile = 0;
                        dataStream << tile;
                    }
                }
                break;
            case TilemapFormat::BPP_4:
            case TilemapFormat::BPP_8:
                for (int y = 0; y < newHeight; y++)
                for (int x = 0; x < newWidth; x++) {
                    if (y < oldHeight && x < oldWidth) {
                        int i = x + y * oldWidth;
                        uint16_t tile = tilemapCopy[i]->raw();
                        dataStream << tile;
                    } else {
                        uint16_t tile = 0;
                        dataStream << tile;
                    }
                }
                break;
        }
        setTilemap(tilemapArray);
    }
}

void RegionMap::emitDisplay() {
    emit mapNeedsDisplaying();
}

QByteArray RegionMap::getTilemap() {
    QByteArray tilemapArray;
    QDataStream dataStream(&tilemapArray, QIODevice::WriteOnly);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    switch (this->tilemap_format) {
        case TilemapFormat::Plain:
            for (int i = 0; i < tilemapSize(); i++) {
                uint8_t tile = this->tilemap[i]->raw();
                dataStream << tile;
            }
            break;
        case TilemapFormat::BPP_4:
            for (int i = 0; i < tilemapSize(); i++) {
                uint16_t tile = this->tilemap[i]->raw();
                dataStream << tile;
            }
            break;
        case TilemapFormat::BPP_8:
            for (int i = 0; i < tilemapSize(); i++) {
                uint16_t tile = this->tilemap[i]->raw();
                dataStream << tile;
            }
            break;
    }

    return tilemapArray;
}

void RegionMap::setTilemap(QByteArray newTilemap) {
    QDataStream dataStream(newTilemap);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    this->tilemap.clear();
    this->tilemap.resize(tilemapSize());
    switch (this->tilemap_format) {
        case TilemapFormat::Plain:
            for (int i = 0; i < tilemapBytes(); i++) {
                uint8_t tile;
                dataStream >> tile;
                this->tilemap[i] = make_shared<PlainTile>(tile);
            }
            break;
        case TilemapFormat::BPP_4:
            for (int i = 0; i < tilemapSize(); i++) {
                uint16_t tile;
                dataStream >> tile;
                this->tilemap[i] = make_shared<BPP4Tile>(tile & 0xffff);
            }
            break;
        case TilemapFormat::BPP_8:
            for (int i = 0; i < tilemapSize(); i++) {
                uint16_t tile;
                dataStream >> tile;
                this->tilemap[i] = make_shared<BPP8Tile>(tile & 0xffff);
            }
            break;
    }
}

QList<LayoutSquare> RegionMap::getLayout(QString layer) {
    return this->layouts[layer];
}

void RegionMap::setLayout(QString layer, QList<LayoutSquare> layout) {
    this->layouts[layer] = layout;
}

QMap<QString, QList<LayoutSquare>> RegionMap::getAllLayouts() {
    return this->layouts;
}

void RegionMap::setAllLayouts(QMap<QString, QList<LayoutSquare>> newLayouts) {
    this->layouts = newLayouts;
}

void RegionMap::setLayoutDimensions(int width, int height, bool update) {
    // for each layer
    if (update) {
        for (QString layer : this->layout_layers) {
            QList<LayoutSquare> oldLayout = this->getLayout(layer);
            QList<LayoutSquare> newLayout;

            for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++) {
                LayoutSquare newSquare;
                newSquare.x = x;
                newSquare.y = y;
                if (x < this->layout_width && y < this->layout_height) {
                    // within old layout
                    int oldIndex = this->get_layout_index(x, y);
                    newSquare = oldLayout[oldIndex];
                }
                newLayout.append(newSquare);
            }
            this->setLayout(layer, newLayout);
        }
    }

    this->layout_width = width;
    this->layout_height = height;
}

// Layout coords to image index.
int RegionMap::get_tilemap_index(int x, int y) {
    return ((x + this->offset_left) + (y + this->offset_top) * this->tilemap_width);
}

// Layout coords to layout index.
int RegionMap::get_layout_index(int x, int y) {
    return x + y * this->layout_width;
}

unsigned RegionMap::getTileId(int index) {
    if (index < tilemap.size()) {
        return tilemap[index]->id();
    }

    return 0;
}

shared_ptr<TilemapTile> RegionMap::getTile(int index) { 
    if (index < tilemap.size()) {
        return tilemap[index];
    }

    return nullptr;
}

unsigned RegionMap::getTileId(int x, int y) {
    int index = x + y * tilemap_width;
    return getTileId(index);
}

shared_ptr<TilemapTile> RegionMap::getTile(int x, int y) {
    int index = x + y * tilemap_width;
    return getTile(index);
}

void RegionMap::setTileId(int index, unsigned id) {
    if (index < tilemap.size()) {
        tilemap[index]->setId(id);
    }
}

void RegionMap::setTile(int index, TilemapTile &tile) {
    if (index < tilemap.size()) {
        tilemap[index]->copy(tile);
    }
}

void RegionMap::setTileData(int index, unsigned id, bool hFlip, bool vFlip, int palette) {
    if (index < tilemap.size()) {
        tilemap[index]->setId(id);
        tilemap[index]->setHFlip(hFlip);
        tilemap[index]->setVFlip(vFlip);
        tilemap[index]->setPalette(palette);
    }
}

int RegionMap::tilemapToLayoutIndex(int index) {
    int x = index % this->tilemap_width;
    if (x < this->offset_left) return -1;
    int y = index / this->tilemap_width;
    if (y < this->offset_top) return -1;
    int layoutX = x - this->offset_left;
    if (layoutX >= this->layout_width) return -1;
    int layoutY = y - this->offset_top;
    if (layoutY >= this->layout_height) return -1;
    return layoutX + layoutY * this->layout_width;
}

bool RegionMap::squareHasMap(int index) {
    int layoutIndex = tilemapToLayoutIndex(index);
    return (layoutIndex < 0 || !this->layouts.contains(this->current_layer)) ? false : this->layouts[this->current_layer][layoutIndex].has_map;
}

QString RegionMap::squareMapSection(int index) {
    int layoutIndex = tilemapToLayoutIndex(index);
    return (layoutIndex < 0 || !this->layouts.contains(this->current_layer)) ? QString() : this->layouts[this->current_layer][layoutIndex].map_section;
}

void RegionMap::setSquareMapSection(int index, QString section) {
    int layoutIndex = tilemapToLayoutIndex(index);
    if (!(layoutIndex < 0 || !this->layouts.contains(this->current_layer))) {
        this->layouts[this->current_layer][layoutIndex].map_section = section;
        this->layouts[this->current_layer][layoutIndex].has_map = !(section == this->default_map_section || section.isEmpty());
    }
}

int RegionMap::squareX(int index) {
    int layoutIndex = tilemapToLayoutIndex(index);
    return (layoutIndex < 0 || !this->layouts.contains(this->current_layer)) ? -1 : this->layouts[this->current_layer][layoutIndex].x;
}

int RegionMap::squareY(int index) {
    int layoutIndex = tilemapToLayoutIndex(index);
    return (layoutIndex < 0 || !this->layouts.contains(this->current_layer)) ? -1 : this->layouts[this->current_layer][layoutIndex].y;
}

bool RegionMap::squareInLayout(int x, int y) {
    return !((x < this->offset_left) || (x >= (this->offset_left + this->layout_width)) 
          || (y < this->offset_top) || (y >= (this->offset_top + this->layout_height)));
}

MapSectionEntry RegionMap::getEntry(QString section) {
    return this->region_map_entries->value(section, MapSectionEntry());
}

void RegionMap::setEntry(QString section, MapSectionEntry entry) {
    this->region_map_entries->insert(section, entry);
}

void RegionMap::removeEntry(QString section) {
    this->region_map_entries->remove(section);
}

QString RegionMap::palPath() {
    return this->palette_path.isEmpty() ? QString() : this->project->root + "/" + this->palette_path;
}

QString RegionMap::pngPath() {
    return this->project->root + "/" + this->tileset_path;
}

// From x, y of image.
int RegionMap::getMapSquareIndex(int x, int y) {
    int index = (x + y * this->tilemap_width);
    return ((index < tilemap.length()) && (index >= 0)) ? index : 0;
}

QString RegionMap::fullPath(QString local) {
    return this->project->root + "/" + local;
}
