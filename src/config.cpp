#include "config.h"
#include "log.h"
#include "shortcut.h"
#include "map.h"
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QComboBox>
#include <QLabel>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QAction>
#include <QAbstractButton>

const QMap<ProjectFilePath, std::pair<QString, QString>> defaultPaths = {
    {ProjectFilePath::data_map_folders,                 { "data_map_folders",                "data/maps/"}},
    {ProjectFilePath::data_scripts_folders,             { "data_scripts_folders",            "data/scripts/"}},
    {ProjectFilePath::data_layouts_folders,             { "data_layouts_folders",            "data/layouts/"}},
    {ProjectFilePath::data_tilesets_folders,            { "data_tilesets_folders",           "data/tilesets/"}},
    {ProjectFilePath::data_event_scripts,               { "data_event_scripts",              "data/event_scripts.s"}},
    {ProjectFilePath::json_map_groups,                  { "json_map_groups",                 "data/maps/map_groups.json"}},
    {ProjectFilePath::json_layouts,                     { "json_layouts",                    "data/layouts/layouts.json"}},
    {ProjectFilePath::json_wild_encounters,             { "json_wild_encounters",            "src/data/wild_encounters.json"}},
    {ProjectFilePath::json_region_map_entries,          { "json_region_map_entries",         "src/data/region_map/region_map_sections.json"}},
    {ProjectFilePath::json_region_porymap_cfg,          { "json_region_porymap_cfg",         "src/data/region_map/porymap_config.json"}},
    {ProjectFilePath::tilesets_headers,                 { "tilesets_headers",                "src/data/tilesets/headers.h"}},
    {ProjectFilePath::tilesets_graphics,                { "tilesets_graphics",               "src/data/tilesets/graphics.h"}},
    {ProjectFilePath::tilesets_metatiles,               { "tilesets_metatiles",              "src/data/tilesets/metatiles.h"}},
    {ProjectFilePath::tilesets_headers_asm,             { "tilesets_headers_asm",            "data/tilesets/headers.inc"}},
    {ProjectFilePath::tilesets_graphics_asm,            { "tilesets_graphics_asm",           "data/tilesets/graphics.inc"}},
    {ProjectFilePath::tilesets_metatiles_asm,           { "tilesets_metatiles_asm",          "data/tilesets/metatiles.inc"}},
    {ProjectFilePath::data_obj_event_gfx_pointers,      { "data_obj_event_gfx_pointers",     "src/data/object_events/object_event_graphics_info_pointers.h"}},
    {ProjectFilePath::data_obj_event_gfx_info,          { "data_obj_event_gfx_info",         "src/data/object_events/object_event_graphics_info.h"}},
    {ProjectFilePath::data_obj_event_pic_tables,        { "data_obj_event_pic_tables",       "src/data/object_events/object_event_pic_tables.h"}},
    {ProjectFilePath::data_obj_event_gfx,               { "data_obj_event_gfx",              "src/data/object_events/object_event_graphics.h"}},
    {ProjectFilePath::data_pokemon_gfx,                 { "data_pokemon_gfx",                "src/data/graphics/pokemon.h"}},
    {ProjectFilePath::data_heal_locations,              { "data_heal_locations",             "src/data/heal_locations.h"}},
    {ProjectFilePath::constants_global,                 { "constants_global",                "include/constants/global.h"}},
    {ProjectFilePath::constants_map_groups,             { "constants_map_groups",            "include/constants/map_groups.h"}},
    {ProjectFilePath::constants_items,                  { "constants_items",                 "include/constants/items.h"}},
    {ProjectFilePath::constants_opponents,              { "constants_opponents",             "include/constants/opponents.h"}},
    {ProjectFilePath::constants_flags,                  { "constants_flags",                 "include/constants/flags.h"}},
    {ProjectFilePath::constants_vars,                   { "constants_vars",                  "include/constants/vars.h"}},
    {ProjectFilePath::constants_weather,                { "constants_weather",               "include/constants/weather.h"}},
    {ProjectFilePath::constants_songs,                  { "constants_songs",                 "include/constants/songs.h"}},
    {ProjectFilePath::constants_heal_locations,         { "constants_heal_locations",        "include/constants/heal_locations.h"}},
    {ProjectFilePath::constants_pokemon,                { "constants_pokemon",               "include/constants/pokemon.h"}},
    {ProjectFilePath::constants_map_types,              { "constants_map_types",             "include/constants/map_types.h"}},
    {ProjectFilePath::constants_trainer_types,          { "constants_trainer_types",         "include/constants/trainer_types.h"}},
    {ProjectFilePath::constants_secret_bases,           { "constants_secret_bases",          "include/constants/secret_bases.h"}},
    {ProjectFilePath::constants_obj_event_movement,     { "constants_obj_event_movement",    "include/constants/event_object_movement.h"}},
    {ProjectFilePath::constants_obj_events,             { "constants_obj_events",            "include/constants/event_objects.h"}},
    {ProjectFilePath::constants_event_bg,               { "constants_event_bg",              "include/constants/event_bg.h"}},
    {ProjectFilePath::constants_region_map_sections,    { "constants_region_map_sections",   "include/constants/region_map_sections.h"}},
    {ProjectFilePath::constants_metatile_labels,        { "constants_metatile_labels",       "include/constants/metatile_labels.h"}},
    {ProjectFilePath::constants_metatile_behaviors,     { "constants_metatile_behaviors",    "include/constants/metatile_behaviors.h"}},
    {ProjectFilePath::constants_fieldmap,               { "constants_fieldmap",              "include/fieldmap.h"}},
    {ProjectFilePath::pokemon_icon_table,               { "pokemon_icon_table",              "src/pokemon_icon.c"}},
    {ProjectFilePath::initial_facing_table,             { "initial_facing_table",            "src/event_object_movement.c"}},
};

ProjectFilePath reverseDefaultPaths(QString str) {
    for (auto it = defaultPaths.constKeyValueBegin(); it != defaultPaths.constKeyValueEnd(); ++it) {
        if ((*it).second.first == str) return (*it).first;
    }
    return static_cast<ProjectFilePath>(-1);
}

KeyValueConfigBase::~KeyValueConfigBase() {

}

void KeyValueConfigBase::load() {
    reset();
    QFile file(this->getConfigFilepath());
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly)) {
            logError(QString("Could not create config file '%1'").arg(this->getConfigFilepath()));
        } else {
            file.close();
            this->onNewConfigFileCreated();
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open config file '%1': ").arg(this->getConfigFilepath()) + file.errorString());
    }

    QTextStream in(&file);
    QList<QString> configLines;
    static const QRegularExpression re("^(?<key>[^=]+)=(?<value>.*)$");
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        int commentIndex = line.indexOf("#");
        if (commentIndex >= 0) {
            line = line.left(commentIndex).trimmed();
        }

        if (line.length() == 0) {
            continue;
        }

        QRegularExpressionMatch match = re.match(line);
        if (!match.hasMatch()) {
            logWarn(QString("Invalid config line in %1: '%2'").arg(this->getConfigFilepath()).arg(line));
            continue;
        }

        this->parseConfigKeyValue(match.captured("key").trimmed().toLower(), match.captured("value").trimmed());
    }
    this->setUnreadKeys();
    this->save();

    file.close();
}

void KeyValueConfigBase::save() {
    QString text = "";
    QMap<QString, QString> map = this->getKeyValueMap();
    for (QMap<QString, QString>::iterator it = map.begin(); it != map.end(); it++) {
        text += QString("%1=%2\n").arg(it.key()).arg(it.value());
    }

    QFile file(this->getConfigFilepath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
        file.close();
    } else {
        logError(QString("Could not open config file '%1' for writing: ").arg(this->getConfigFilepath()) + file.errorString());
    }
}

bool KeyValueConfigBase::getConfigBool(QString key, QString value) {
    bool ok;
    int result = value.toInt(&ok, 0);
    if (!ok || (result != 0 && result != 1)) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be 0 or 1.").arg(key).arg(value));
    }
    return (result != 0);
}

int KeyValueConfigBase::getConfigInteger(QString key, QString value, int min, int max, int defaultValue) {
    bool ok;
    int result = value.toInt(&ok, 0);
    if (!ok) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be an integer.").arg(key).arg(value));
        return defaultValue;
    }
    return qMin(max, qMax(min, result));
}

uint32_t KeyValueConfigBase::getConfigUint32(QString key, QString value, uint32_t min, uint32_t max, uint32_t defaultValue) {
    bool ok;
    uint32_t result = value.toUInt(&ok, 0);
    if (!ok) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be an integer.").arg(key).arg(value));
        return defaultValue;
    }
    return qMin(max, qMax(min, result));
}

const QMap<MapSortOrder, QString> mapSortOrderMap = {
    {MapSortOrder::Group, "group"},
    {MapSortOrder::Layout, "layout"},
    {MapSortOrder::Area, "area"},
};

const QMap<QString, MapSortOrder> mapSortOrderReverseMap = {
    {"group", MapSortOrder::Group},
    {"layout", MapSortOrder::Layout},
    {"area", MapSortOrder::Area},
};

PorymapConfig porymapConfig;

QString PorymapConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);

    QString configPath = dir.absoluteFilePath("porymap.cfg");

    return configPath;
}

void PorymapConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_project") {
        this->recentProject = value;
    } else if (key == "reopen_on_launch") {
        this->reopenOnLaunch = getConfigBool(key, value);
    } else if (key == "pretty_cursors") {
        this->prettyCursors = getConfigBool(key, value);
    } else if (key == "map_sort_order") {
        QString sortOrder = value.toLower();
        if (mapSortOrderReverseMap.contains(sortOrder)) {
            this->mapSortOrder = mapSortOrderReverseMap.value(sortOrder);
        } else {
            this->mapSortOrder = MapSortOrder::Group;
            logWarn(QString("Invalid config value for map_sort_order: '%1'. Must be 'group', 'area', or 'layout'.").arg(value));
        }
    } else if (key == "main_window_geometry") {
        this->mainWindowGeometry = bytesFromString(value);
    } else if (key == "main_window_state") {
        this->mainWindowState = bytesFromString(value);
    } else if (key == "map_splitter_state") {
        this->mapSplitterState = bytesFromString(value);
    } else if (key == "main_splitter_state") {
        this->mainSplitterState = bytesFromString(value);
    } else if (key == "collision_opacity") {
        this->collisionOpacity = getConfigInteger(key, value, 0, 100, 50);
    } else if (key == "tileset_editor_geometry") {
        this->tilesetEditorGeometry = bytesFromString(value);
    } else if (key == "tileset_editor_state") {
        this->tilesetEditorState = bytesFromString(value);
    } else if (key == "palette_editor_geometry") {
        this->paletteEditorGeometry = bytesFromString(value);
    } else if (key == "palette_editor_state") {
        this->paletteEditorState = bytesFromString(value);
    } else if (key == "region_map_editor_geometry") {
        this->regionMapEditorGeometry = bytesFromString(value);
    } else if (key == "region_map_editor_state") {
        this->regionMapEditorState = bytesFromString(value);
    } else if (key == "metatiles_zoom") {
        this->metatilesZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "show_player_view") {
        this->showPlayerView = getConfigBool(key, value);
    } else if (key == "show_cursor_tile") {
        this->showCursorTile = getConfigBool(key, value);
    } else if (key == "show_border") {
        this->showBorder = getConfigBool(key, value);
    } else if (key == "show_grid") {
        this->showGrid = getConfigBool(key, value);
    } else if (key == "monitor_files") {
        this->monitorFiles = getConfigBool(key, value);
    } else if (key == "tileset_checkerboard_fill") {
        this->tilesetCheckerboardFill = getConfigBool(key, value);
    } else if (key == "theme") {
        this->theme = value;
    } else if (key == "text_editor_open_directory") {
        this->textEditorOpenFolder = value;
    } else if (key == "text_editor_goto_line") {
        this->textEditorGotoLine = value;
    } else if (key == "palette_editor_bit_depth") {
        this->paletteEditorBitDepth = getConfigInteger(key, value, 15, 24, 24);
        if (this->paletteEditorBitDepth != 15 && this->paletteEditorBitDepth != 24){
            this->paletteEditorBitDepth = 24;
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProject);
    map.insert("reopen_on_launch", this->reopenOnLaunch ? "1" : "0");
    map.insert("pretty_cursors", this->prettyCursors ? "1" : "0");
    map.insert("map_sort_order", mapSortOrderMap.value(this->mapSortOrder));
    map.insert("main_window_geometry", stringFromByteArray(this->mainWindowGeometry));
    map.insert("main_window_state", stringFromByteArray(this->mainWindowState));
    map.insert("map_splitter_state", stringFromByteArray(this->mapSplitterState));
    map.insert("main_splitter_state", stringFromByteArray(this->mainSplitterState));
    map.insert("tileset_editor_geometry", stringFromByteArray(this->tilesetEditorGeometry));
    map.insert("tileset_editor_state", stringFromByteArray(this->tilesetEditorState));
    map.insert("palette_editor_geometry", stringFromByteArray(this->paletteEditorGeometry));
    map.insert("palette_editor_state", stringFromByteArray(this->paletteEditorState));
    map.insert("region_map_editor_geometry", stringFromByteArray(this->regionMapEditorGeometry));
    map.insert("region_map_editor_state", stringFromByteArray(this->regionMapEditorState));
    map.insert("collision_opacity", QString("%1").arg(this->collisionOpacity));
    map.insert("metatiles_zoom", QString("%1").arg(this->metatilesZoom));
    map.insert("show_player_view", this->showPlayerView ? "1" : "0");
    map.insert("show_cursor_tile", this->showCursorTile ? "1" : "0");
    map.insert("show_border", this->showBorder ? "1" : "0");
    map.insert("show_grid", this->showGrid ? "1" : "0");
    map.insert("monitor_files", this->monitorFiles ? "1" : "0");
    map.insert("tileset_checkerboard_fill", this->tilesetCheckerboardFill ? "1" : "0");
    map.insert("theme", this->theme);
    map.insert("text_editor_open_directory", this->textEditorOpenFolder);
    map.insert("text_editor_goto_line", this->textEditorGotoLine);
    map.insert("palette_editor_bit_depth", QString("%1").arg(this->paletteEditorBitDepth));
    
    return map;
}

QString PorymapConfig::stringFromByteArray(QByteArray bytearray) {
    QString ret;
    for (auto ch : bytearray) {
        ret += QString::number(static_cast<int>(ch)) + ":";
    }
    return ret;
}

QByteArray PorymapConfig::bytesFromString(QString in) {
    QByteArray ba;
    QStringList split = in.split(":");
    for (auto ch : split) {
        ba.append(static_cast<char>(ch.toInt()));
    }
    return ba;
}

void PorymapConfig::setRecentProject(QString project) {
    this->recentProject = project;
    this->save();
}

void PorymapConfig::setReopenOnLaunch(bool enabled) {
    this->reopenOnLaunch = enabled;
    this->save();
}

void PorymapConfig::setMapSortOrder(MapSortOrder order) {
    this->mapSortOrder = order;
    this->save();
}

void PorymapConfig::setPrettyCursors(bool enabled) {
    this->prettyCursors = enabled;
    this->save();
}

void PorymapConfig::setMonitorFiles(bool monitor) {
    this->monitorFiles = monitor;
    this->save();
}

void PorymapConfig::setTilesetCheckerboardFill(bool checkerboard) {
    this->tilesetCheckerboardFill = checkerboard;
    this->save();
}

void PorymapConfig::setMainGeometry(QByteArray mainWindowGeometry_, QByteArray mainWindowState_,
                                QByteArray mapSplitterState_, QByteArray mainSplitterState_) {
    this->mainWindowGeometry = mainWindowGeometry_;
    this->mainWindowState = mainWindowState_;
    this->mapSplitterState = mapSplitterState_;
    this->mainSplitterState = mainSplitterState_;
    this->save();
}

void PorymapConfig::setTilesetEditorGeometry(QByteArray tilesetEditorGeometry_, QByteArray tilesetEditorState_) {
    this->tilesetEditorGeometry = tilesetEditorGeometry_;
    this->tilesetEditorState = tilesetEditorState_;
    this->save();
}

void PorymapConfig::setPaletteEditorGeometry(QByteArray paletteEditorGeometry_, QByteArray paletteEditorState_) {
    this->paletteEditorGeometry = paletteEditorGeometry_;
    this->paletteEditorState = paletteEditorState_;
    this->save();
}

void PorymapConfig::setRegionMapEditorGeometry(QByteArray regionMapEditorGeometry_, QByteArray regionMapEditorState_) {
    this->regionMapEditorGeometry = regionMapEditorGeometry_;
    this->regionMapEditorState = regionMapEditorState_;
    this->save();
}

void PorymapConfig::setCollisionOpacity(int opacity) {
    this->collisionOpacity = opacity;
    // don't auto-save here because this can be called very frequently.
}

void PorymapConfig::setMetatilesZoom(int zoom) {
    this->metatilesZoom = zoom;
    // don't auto-save here because this can be called very frequently.
}

void PorymapConfig::setShowPlayerView(bool enabled) {
    this->showPlayerView = enabled;
    this->save();
}

void PorymapConfig::setShowCursorTile(bool enabled) {
    this->showCursorTile = enabled;
    this->save();
}

void PorymapConfig::setShowBorder(bool enabled) {
    this->showBorder = enabled;
    this->save();
}

void PorymapConfig::setShowGrid(bool enabled) {
    this->showGrid = enabled;
    this->save();
}

void PorymapConfig::setTheme(QString theme) {
    this->theme = theme;
}

void PorymapConfig::setTextEditorOpenFolder(const QString &command) {
    this->textEditorOpenFolder = command;
    this->save();
}

void PorymapConfig::setTextEditorGotoLine(const QString &command) {
    this->textEditorGotoLine = command;
    this->save();
}

void PorymapConfig::setPaletteEditorBitDepth(int bitDepth) {
    this->paletteEditorBitDepth = bitDepth;
    this->save();
}

QString PorymapConfig::getRecentProject() {
    return this->recentProject;
}

bool PorymapConfig::getReopenOnLaunch() {
    return this->reopenOnLaunch;
}

MapSortOrder PorymapConfig::getMapSortOrder() {
    return this->mapSortOrder;
}

bool PorymapConfig::getPrettyCursors() {
    return this->prettyCursors;
}

QMap<QString, QByteArray> PorymapConfig::getMainGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("main_window_geometry", this->mainWindowGeometry);
    geometry.insert("main_window_state", this->mainWindowState);
    geometry.insert("map_splitter_state", this->mapSplitterState);
    geometry.insert("main_splitter_state", this->mainSplitterState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getTilesetEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("tileset_editor_geometry", this->tilesetEditorGeometry);
    geometry.insert("tileset_editor_state", this->tilesetEditorState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getPaletteEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("palette_editor_geometry", this->paletteEditorGeometry);
    geometry.insert("palette_editor_state", this->paletteEditorState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getRegionMapEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("region_map_editor_geometry", this->regionMapEditorGeometry);
    geometry.insert("region_map_editor_state", this->regionMapEditorState);

    return geometry;
}

int PorymapConfig::getCollisionOpacity() {
    return this->collisionOpacity;
}

int PorymapConfig::getMetatilesZoom() {
    return this->metatilesZoom;
}

bool PorymapConfig::getShowPlayerView() {
    return this->showPlayerView;
}

bool PorymapConfig::getShowCursorTile() {
    return this->showCursorTile;
}

bool PorymapConfig::getShowBorder() {
    return this->showBorder;
}

bool PorymapConfig::getShowGrid() {
    return this->showGrid;
}

bool PorymapConfig::getMonitorFiles() {
    return this->monitorFiles;
}

bool PorymapConfig::getTilesetCheckerboardFill() {
    return this->tilesetCheckerboardFill;
}

QString PorymapConfig::getTheme() {
    return this->theme;
}

QString PorymapConfig::getTextEditorOpenFolder() {
    return this->textEditorOpenFolder;
}

QString PorymapConfig::getTextEditorGotoLine() {
    return this->textEditorGotoLine;
}

int PorymapConfig::getPaletteEditorBitDepth() {
    return this->paletteEditorBitDepth;
}

const QMap<BaseGameVersion, QString> baseGameVersionMap = {
    {BaseGameVersion::pokeruby, "pokeruby"},
    {BaseGameVersion::pokefirered, "pokefirered"},
    {BaseGameVersion::pokeemerald, "pokeemerald"},
};

const QMap<QString, BaseGameVersion> baseGameVersionReverseMap = {
    {"pokeruby", BaseGameVersion::pokeruby},
    {"pokefirered", BaseGameVersion::pokefirered},
    {"pokeemerald", BaseGameVersion::pokeemerald},
};

ProjectConfig projectConfig;

QString ProjectConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    return QDir(this->projectDir).filePath("porymap.project.cfg");
}

void ProjectConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "base_game_version") {
        QString baseGameVersion = value.toLower();
        if (baseGameVersionReverseMap.contains(baseGameVersion)) {
            this->baseGameVersion = baseGameVersionReverseMap.value(baseGameVersion);
        } else {
            this->baseGameVersion = BaseGameVersion::pokeemerald;
            logWarn(QString("Invalid config value for base_game_version: '%1'. Must be 'pokeruby', 'pokefirered' or 'pokeemerald'.").arg(value));
        }
    } else if (key == "use_poryscript") {
        this->usePoryScript = getConfigBool(key, value);
    } else if (key == "use_custom_border_size") {
        this->useCustomBorderSize = getConfigBool(key, value);
    } else if (key == "enable_event_weather_trigger") {
        this->enableEventWeatherTrigger = getConfigBool(key, value);
    } else if (key == "enable_event_secret_base") {
        this->enableEventSecretBase = getConfigBool(key, value);
    } else if (key == "enable_hidden_item_quantity") {
        this->enableHiddenItemQuantity = getConfigBool(key, value);
    } else if (key == "enable_hidden_item_requires_itemfinder") {
        this->enableHiddenItemRequiresItemfinder = getConfigBool(key, value);
    } else if (key == "enable_heal_location_respawn_data") {
        this->enableHealLocationRespawnData = getConfigBool(key, value);
    } else if (key == "enable_event_clone_object" || key == "enable_object_event_in_connection") {
        this->enableEventCloneObject = getConfigBool(key, value);
        key = "enable_event_clone_object"; // Backwards compatibiliy, replace old name above
    } else if (key == "enable_floor_number") {
        this->enableFloorNumber = getConfigBool(key, value);
    } else if (key == "create_map_text_file") {
        this->createMapTextFile = getConfigBool(key, value);
    } else if (key == "enable_triple_layer_metatiles") {
        this->enableTripleLayerMetatiles = getConfigBool(key, value);
    } else if (key == "new_map_metatile") {
        this->newMapMetatileId = getConfigInteger(key, value, 0, 1023, 0);
    } else if (key == "new_map_elevation") {
        this->newMapElevation = getConfigInteger(key, value, 0, 15, 3);
    } else if (key == "new_map_border_metatiles") {
        this->newMapBorderMetatileIds.clear();
        QList<QString> metatileIds = value.split(",");
        const int maxSize = DEFAULT_BORDER_WIDTH * DEFAULT_BORDER_HEIGHT;
        const int size = qMin(metatileIds.size(), maxSize);
        int i;
        for (i = 0; i < size; i++) {
            int metatileId = getConfigInteger(key, metatileIds.at(i), 0, 1023, 0);
            this->newMapBorderMetatileIds.append(metatileId);
        }
        for (; i < maxSize; i++) {
            // Set any metatiles not provided to 0
            this->newMapBorderMetatileIds.append(0);
        }
    } else if (key == "default_primary_tileset") {
        this->defaultPrimaryTileset = value;
    } else if (key == "default_secondary_tileset") {
        this->defaultSecondaryTileset = value;
    } else if (key == "metatile_attributes_size") {
        int size = getConfigInteger(key, value, 1, 4, 2);
        if (size & (size - 1)) {
            logWarn(QString("Invalid config value for %1: must be 1, 2, or 4").arg(key));
            return; // Don't set a default now, it will be set later based on the base game version
        }
        this->metatileAttributesSize = size;
    } else if (key == "metatile_behavior_mask") {
        this->metatileBehaviorMask = getConfigUint32(key, value, 0, 0xFFFFFFFF, 0);
    } else if (key == "metatile_terrain_type_mask") {
        this->metatileTerrainTypeMask = getConfigUint32(key, value, 0, 0xFFFFFFFF, 0);
    } else if (key == "metatile_encounter_type_mask") {
        this->metatileEncounterTypeMask = getConfigUint32(key, value, 0, 0xFFFFFFFF, 0);
    } else if (key == "metatile_layer_type_mask") {
        this->metatileLayerTypeMask = getConfigUint32(key, value, 0, 0xFFFFFFFF, 0);
    } else if (key == "enable_map_allow_flags") {
        this->enableMapAllowFlags = getConfigBool(key, value);
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    } else if (key == "recent_map") {
        userConfig.setRecentMap(value);
    } else if (key == "use_encounter_json") {
        userConfig.useEncounterJson = getConfigBool(key, value);
    } else if (key == "custom_scripts") {
        userConfig.customScripts.clear();
        QList<QString> paths = value.split(",");
        paths.removeDuplicates();
        for (QString script : paths) {
            if (!script.isEmpty()) {
                userConfig.customScripts.append(script);
            }
        }
#endif
    } else if (key.startsWith("path/")) {
        auto k = reverseDefaultPaths(key.mid(5));
        if (k != static_cast<ProjectFilePath>(-1)) {
            this->setFilePath(k, value);
        } else {
            logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
        }
    } else if (key == "prefabs_filepath") {
        this->prefabFilepath = value;
    } else if (key == "prefabs_import_prompted") {
        this->prefabImportPrompted = getConfigBool(key, value);
    } else if (key == "tilesets_have_callback") {
        this->tilesetsHaveCallback = getConfigBool(key, value);
    } else if (key == "tilesets_have_is_compressed") {
        this->tilesetsHaveIsCompressed = getConfigBool(key, value);
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
    readKeys.append(key);
}

void ProjectConfig::setUnreadKeys() {
    // Set game-version specific defaults for any config field that wasn't read
    bool isPokefirered = this->baseGameVersion == BaseGameVersion::pokefirered;
    if (!readKeys.contains("use_custom_border_size")) this->useCustomBorderSize = isPokefirered;
    if (!readKeys.contains("enable_event_weather_trigger")) this->enableEventWeatherTrigger = !isPokefirered;
    if (!readKeys.contains("enable_event_secret_base")) this->enableEventSecretBase = !isPokefirered;
    if (!readKeys.contains("enable_hidden_item_quantity")) this->enableHiddenItemQuantity = isPokefirered;
    if (!readKeys.contains("enable_hidden_item_requires_itemfinder")) this->enableHiddenItemRequiresItemfinder = isPokefirered;
    if (!readKeys.contains("enable_heal_location_respawn_data")) this->enableHealLocationRespawnData = isPokefirered;
    if (!readKeys.contains("enable_event_clone_object")) this->enableEventCloneObject = isPokefirered;
    if (!readKeys.contains("enable_floor_number")) this->enableFloorNumber = isPokefirered;
    if (!readKeys.contains("create_map_text_file")) this->createMapTextFile = (this->baseGameVersion != BaseGameVersion::pokeemerald);
    if (!readKeys.contains("new_map_border_metatiles")) this->newMapBorderMetatileIds = isPokefirered ? DEFAULT_BORDER_FRLG : DEFAULT_BORDER_RSE;
    if (!readKeys.contains("default_secondary_tileset")) this->defaultSecondaryTileset = isPokefirered ? "gTileset_PalletTown" : "gTileset_Petalburg";
    if (!readKeys.contains("metatile_attributes_size")) this->metatileAttributesSize = Metatile::getDefaultAttributesSize(this->baseGameVersion);
    if (!readKeys.contains("metatile_behavior_mask")) this->metatileBehaviorMask = Metatile::getBehaviorMask(this->baseGameVersion);
    if (!readKeys.contains("metatile_terrain_type_mask")) this->metatileTerrainTypeMask = Metatile::getTerrainTypeMask(this->baseGameVersion);
    if (!readKeys.contains("metatile_encounter_type_mask")) this->metatileEncounterTypeMask = Metatile::getEncounterTypeMask(this->baseGameVersion);
    if (!readKeys.contains("metatile_layer_type_mask")) this-> metatileLayerTypeMask = Metatile::getLayerTypeMask(this->baseGameVersion);
    if (!readKeys.contains("enable_map_allow_flags")) this->enableMapAllowFlags = (this->baseGameVersion != BaseGameVersion::pokeruby);
}

QMap<QString, QString> ProjectConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("base_game_version", baseGameVersionMap.value(this->baseGameVersion));
    map.insert("use_poryscript", QString::number(this->usePoryScript));
    map.insert("use_custom_border_size", QString::number(this->useCustomBorderSize));
    map.insert("enable_event_weather_trigger", QString::number(this->enableEventWeatherTrigger));
    map.insert("enable_event_secret_base", QString::number(this->enableEventSecretBase));
    map.insert("enable_hidden_item_quantity", QString::number(this->enableHiddenItemQuantity));
    map.insert("enable_hidden_item_requires_itemfinder", QString::number(this->enableHiddenItemRequiresItemfinder));
    map.insert("enable_heal_location_respawn_data", QString::number(this->enableHealLocationRespawnData));
    map.insert("enable_event_clone_object", QString::number(this->enableEventCloneObject));
    map.insert("enable_floor_number", QString::number(this->enableFloorNumber));
    map.insert("create_map_text_file", QString::number(this->createMapTextFile));
    map.insert("enable_triple_layer_metatiles", QString::number(this->enableTripleLayerMetatiles));
    map.insert("new_map_metatile", QString::number(this->newMapMetatileId));
    map.insert("new_map_elevation", QString::number(this->newMapElevation));
    QStringList metatiles;
    for (auto metatile : this->newMapBorderMetatileIds)
        metatiles << QString::number(metatile);
    map.insert("new_map_border_metatiles", metatiles.join(","));
    map.insert("default_primary_tileset", this->defaultPrimaryTileset);
    map.insert("default_secondary_tileset", this->defaultSecondaryTileset);
    map.insert("prefabs_filepath", this->prefabFilepath);
    map.insert("prefabs_import_prompted", QString::number(this->prefabImportPrompted));
    for (auto it = this->filePaths.constKeyValueBegin(); it != this->filePaths.constKeyValueEnd(); ++it) {
        map.insert("path/"+defaultPaths[(*it).first].first, (*it).second);
    }
    map.insert("tilesets_have_callback", QString::number(this->tilesetsHaveCallback));
    map.insert("tilesets_have_is_compressed", QString::number(this->tilesetsHaveIsCompressed));
    map.insert("metatile_attributes_size", QString::number(this->metatileAttributesSize));
    map.insert("metatile_behavior_mask", "0x" + QString::number(this->metatileBehaviorMask, 16).toUpper());
    map.insert("metatile_terrain_type_mask", "0x" + QString::number(this->metatileTerrainTypeMask, 16).toUpper());
    map.insert("metatile_encounter_type_mask", "0x" + QString::number(this->metatileEncounterTypeMask, 16).toUpper());
    map.insert("metatile_layer_type_mask", "0x" + QString::number(this->metatileLayerTypeMask, 16).toUpper());
    map.insert("enable_map_allow_flags", QString::number(this->enableMapAllowFlags));
    return map;
}

void ProjectConfig::onNewConfigFileCreated() {
    QString dirName = QDir(this->projectDir).dirName().toLower();
    if (baseGameVersionReverseMap.contains(dirName)) {
        this->baseGameVersion = baseGameVersionReverseMap.value(dirName);
        logInfo(QString("Auto-detected base_game_version as '%1'").arg(dirName));
    } else {
        QDialog dialog(nullptr, Qt::WindowTitleHint);
        dialog.setWindowTitle("Project Configuration");
        dialog.setWindowModality(Qt::NonModal);

        QFormLayout form(&dialog);

        QComboBox *baseGameVersionComboBox = new QComboBox();
        baseGameVersionComboBox->addItem("pokeruby", BaseGameVersion::pokeruby);
        baseGameVersionComboBox->addItem("pokefirered", BaseGameVersion::pokefirered);
        baseGameVersionComboBox->addItem("pokeemerald", BaseGameVersion::pokeemerald);
        form.addRow(new QLabel("Game Version"), baseGameVersionComboBox);

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        form.addRow(&buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            this->baseGameVersion = static_cast<BaseGameVersion>(baseGameVersionComboBox->currentData().toInt());
        }
    }
    this->setUnreadKeys(); // Initialize version-specific options
}

void ProjectConfig::setProjectDir(QString projectDir) {
    this->projectDir = projectDir;
}

QString ProjectConfig::getProjectDir() {
    return this->projectDir;
}

void ProjectConfig::setFilePath(ProjectFilePath pathId, QString path) {
    if (!defaultPaths.contains(pathId)) return;
    this->filePaths[pathId] = path;
}

QString ProjectConfig::getFilePath(ProjectFilePath pathId) {
    if (this->filePaths.contains(pathId)) {
        return this->filePaths[pathId];
    } else if (defaultPaths.contains(pathId)) {
        return defaultPaths[pathId].second;
    } else {
        return QString();
    }
}

void ProjectConfig::setBaseGameVersion(BaseGameVersion baseGameVersion) {
    this->baseGameVersion = baseGameVersion;
    this->save();
}

BaseGameVersion ProjectConfig::getBaseGameVersion() {
    return this->baseGameVersion;
}

QString ProjectConfig::getBaseGameVersionString() {
    return baseGameVersionMap.value(this->baseGameVersion);
}

void ProjectConfig::setUsePoryScript(bool usePoryScript) {
    this->usePoryScript = usePoryScript;
    this->save();
}

bool ProjectConfig::getUsePoryScript() {
    return this->usePoryScript;
}

void ProjectConfig::setUseCustomBorderSize(bool enable) {
    this->useCustomBorderSize = enable;
    this->save();
}

bool ProjectConfig::getUseCustomBorderSize() {
    return this->useCustomBorderSize;
}

void ProjectConfig::setEventWeatherTriggerEnabled(bool enable) {
    this->enableEventWeatherTrigger = enable;
    this->save();
}

bool ProjectConfig::getEventWeatherTriggerEnabled() {
    return this->enableEventWeatherTrigger;
}

void ProjectConfig::setEventSecretBaseEnabled(bool enable) {
    this->enableEventSecretBase = enable;
    this->save();
}

bool ProjectConfig::getEventSecretBaseEnabled() {
    return this->enableEventSecretBase;
}

void ProjectConfig::setHiddenItemQuantityEnabled(bool enable) {
    this->enableHiddenItemQuantity = enable;
    this->save();
}

bool ProjectConfig::getHiddenItemQuantityEnabled() {
    return this->enableHiddenItemQuantity;
}

void ProjectConfig::setHiddenItemRequiresItemfinderEnabled(bool enable) {
    this->enableHiddenItemRequiresItemfinder = enable;
    this->save();
}

bool ProjectConfig::getHiddenItemRequiresItemfinderEnabled() {
    return this->enableHiddenItemRequiresItemfinder;
}

void ProjectConfig::setHealLocationRespawnDataEnabled(bool enable) {
    this->enableHealLocationRespawnData = enable;
    this->save();
}

bool ProjectConfig::getHealLocationRespawnDataEnabled() {
    return this->enableHealLocationRespawnData;
}

void ProjectConfig::setEventCloneObjectEnabled(bool enable) {
    this->enableEventCloneObject = enable;
    this->save();
}

bool ProjectConfig::getEventCloneObjectEnabled() {
    return this->enableEventCloneObject;
}

void ProjectConfig::setFloorNumberEnabled(bool enable) {
    this->enableFloorNumber = enable;
    this->save();
}

bool ProjectConfig::getFloorNumberEnabled() {
    return this->enableFloorNumber;
}

void ProjectConfig::setCreateMapTextFileEnabled(bool enable) {
    this->createMapTextFile = enable;
    this->save();
}

bool ProjectConfig::getCreateMapTextFileEnabled() {
    return this->createMapTextFile;
}

void ProjectConfig::setTripleLayerMetatilesEnabled(bool enable) {
    this->enableTripleLayerMetatiles = enable;
    this->save();
}

bool ProjectConfig::getTripleLayerMetatilesEnabled() {
    return this->enableTripleLayerMetatiles;
}

int ProjectConfig::getNumLayersInMetatile() {
    return this->enableTripleLayerMetatiles ? 3 : 2;
}

int ProjectConfig::getNumTilesInMetatile() {
    return this->enableTripleLayerMetatiles ? 12 : 8;
}

void ProjectConfig::setNewMapMetatileId(int metatileId) {
    this->newMapMetatileId = metatileId;
    this->save();
}

int ProjectConfig::getNewMapMetatileId() {
    return this->newMapMetatileId;
}

void ProjectConfig::setNewMapElevation(int elevation) {
    this->newMapElevation = elevation;
    this->save();
}

int ProjectConfig::getNewMapElevation() {
    return this->newMapElevation;
}

void ProjectConfig::setNewMapBorderMetatileIds(QList<int> metatileIds) {
    this->newMapBorderMetatileIds = metatileIds;
    this->save();
}

QList<int> ProjectConfig::getNewMapBorderMetatileIds() {
    return this->newMapBorderMetatileIds;
}

QString ProjectConfig::getDefaultPrimaryTileset() {
    return this->defaultPrimaryTileset;
}

QString ProjectConfig::getDefaultSecondaryTileset() {
    return this->defaultSecondaryTileset;
}

void ProjectConfig::setPrefabFilepath(QString filepath) {
    this->prefabFilepath = filepath;
    this->save();
}

QString ProjectConfig::getPrefabFilepath(bool setIfEmpty) {
    if (setIfEmpty && this->prefabFilepath.isEmpty()) {
        this->setPrefabFilepath("prefabs.json");
    }
    return this->prefabFilepath;
}

void ProjectConfig::setPrefabImportPrompted(bool prompted) {
    this->prefabImportPrompted = prompted;
    this->save();
}

bool ProjectConfig::getPrefabImportPrompted() {
    return this->prefabImportPrompted;
}

void ProjectConfig::setTilesetsHaveCallback(bool has) {
    this->tilesetsHaveCallback = has;
    this->save();
}

bool ProjectConfig::getTilesetsHaveCallback() {
    return this->tilesetsHaveCallback;
}

void ProjectConfig::setTilesetsHaveIsCompressed(bool has) {
    this->tilesetsHaveIsCompressed = has;
    this->save();
}

bool ProjectConfig::getTilesetsHaveIsCompressed() {
    return this->tilesetsHaveIsCompressed;
}

int ProjectConfig::getMetatileAttributesSize() {
    return this->metatileAttributesSize;
}

uint32_t ProjectConfig::getMetatileBehaviorMask() {
    return this->metatileBehaviorMask;
}

uint32_t ProjectConfig::getMetatileTerrainTypeMask() {
    return this->metatileTerrainTypeMask;
}

uint32_t ProjectConfig::getMetatileEncounterTypeMask() {
    return this->metatileEncounterTypeMask;
}

uint32_t ProjectConfig::getMetatileLayerTypeMask() {
    return this->metatileLayerTypeMask;
}

bool ProjectConfig::getMapAllowFlagsEnabled() {
    return this->enableMapAllowFlags;
}


UserConfig userConfig;

QString UserConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    return QDir(this->projectDir).filePath("porymap.user.cfg");
}

void UserConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_map") {
        this->recentMap = value;
    } else if (key == "use_encounter_json") {
        this->useEncounterJson = getConfigBool(key, value);
    } else if (key == "custom_scripts") {
        this->customScripts.clear();
        QList<QString> paths = value.split(",");
        paths.removeDuplicates();
        for (QString script : paths) {
            if (!script.isEmpty()) {
                this->customScripts.append(script);
            }
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
    readKeys.append(key);
}

void UserConfig::setUnreadKeys() {
}

QMap<QString, QString> UserConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_map", this->recentMap);
    map.insert("use_encounter_json", QString::number(this->useEncounterJson));
    map.insert("custom_scripts", this->customScripts.join(","));
    return map;
}

void UserConfig::onNewConfigFileCreated() {
    QString dirName = QDir(this->projectDir).dirName().toLower();
    this->useEncounterJson = true;
    this->customScripts.clear();
}

void UserConfig::setProjectDir(QString projectDir) {
    this->projectDir = projectDir;
}

QString UserConfig::getProjectDir() {
    return this->projectDir;
}

void UserConfig::setRecentMap(const QString &map) {
    this->recentMap = map;
    this->save();
}

QString UserConfig::getRecentMap() {
    return this->recentMap;
}

void UserConfig::setEncounterJsonActive(bool active) {
    this->useEncounterJson = active;
    this->save();
}

bool UserConfig::getEncounterJsonActive() {
    return true;
}

void UserConfig::setCustomScripts(QList<QString> scripts) {
    this->customScripts = scripts;
    this->save();
}

QList<QString> UserConfig::getCustomScripts() {
    return this->customScripts;
}

ShortcutsConfig shortcutsConfig;

QString ShortcutsConfig::getConfigFilepath() {
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);

    QString configPath = dir.absoluteFilePath("porymap.shortcuts.cfg");

    return configPath;
}

void ShortcutsConfig::parseConfigKeyValue(QString key, QString value) {
    QStringList keySequences = value.split(' ');
    for (auto keySequence : keySequences)
        user_shortcuts.insert(key, keySequence);
}

QMap<QString, QString> ShortcutsConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    for (auto cfg_key : user_shortcuts.uniqueKeys()) {
        auto keySequences = user_shortcuts.values(cfg_key);
        QStringList keySequenceStrings;
        for (auto keySequence : keySequences)
            keySequenceStrings.append(keySequence.toString());
        map.insert(cfg_key, keySequenceStrings.join(' '));
    }
    return map;
}

void ShortcutsConfig::setDefaultShortcuts(const QObjectList &objects) {
    storeShortcutsFromList(StoreType::Default, objects);
    save();
}

QList<QKeySequence> ShortcutsConfig::defaultShortcuts(const QObject *object) const {
    return default_shortcuts.values(cfgKey(object));
}

void ShortcutsConfig::setUserShortcuts(const QObjectList &objects) {
    storeShortcutsFromList(StoreType::User, objects);
    save();
}

void ShortcutsConfig::setUserShortcuts(const QMultiMap<const QObject *, QKeySequence> &objects_keySequences) {
    for (auto *object : objects_keySequences.uniqueKeys())
        if (!object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            storeShortcuts(StoreType::User, cfgKey(object), objects_keySequences.values(object));
    save();
}

QList<QKeySequence> ShortcutsConfig::userShortcuts(const QObject *object) const {
    return user_shortcuts.values(cfgKey(object));
}

void ShortcutsConfig::storeShortcutsFromList(StoreType storeType, const QObjectList &objects) {
    for (const auto *object : objects)
        if (!object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            storeShortcuts(storeType, cfgKey(object), currentShortcuts(object));
}

void ShortcutsConfig::storeShortcuts(
        StoreType storeType,
        const QString &cfgKey,
        const QList<QKeySequence> &keySequences)
{
    bool storeUser = (storeType == User) || !user_shortcuts.contains(cfgKey);

    if (storeType == Default)
        default_shortcuts.remove(cfgKey);
    if (storeUser)
        user_shortcuts.remove(cfgKey);

    if (keySequences.isEmpty()) {
        if (storeType == Default)
            default_shortcuts.insert(cfgKey, QKeySequence());
        if (storeUser)
            user_shortcuts.insert(cfgKey, QKeySequence());
    } else {
        for (auto keySequence : keySequences) {
            if (storeType == Default)
                default_shortcuts.insert(cfgKey, keySequence);
            if (storeUser)
                user_shortcuts.insert(cfgKey, keySequence);
        }
    }
}

/* Creates a config key from the object's name prepended with the parent 
 * window's object name, and converts camelCase to snake_case. */
QString ShortcutsConfig::cfgKey(const QObject *object) const {
    auto cfg_key = QString();
    auto *parentWidget = static_cast<QWidget *>(object->parent());
    if (parentWidget)
        cfg_key = parentWidget->window()->objectName() + '_';
    cfg_key += object->objectName();

    static const QRegularExpression re("[A-Z]");
    int i = cfg_key.indexOf(re, 1);
    while (i != -1) {
        if (cfg_key.at(i - 1) != '_')
            cfg_key.insert(i++, '_');
        i = cfg_key.indexOf(re, i + 1);
    }
    return cfg_key.toLower();
}

QList<QKeySequence> ShortcutsConfig::currentShortcuts(const QObject *object) const {
    if (object->inherits("QAction")) {
        const auto *action = qobject_cast<const QAction *>(object);
        return action->shortcuts();
    } else if (object->inherits("Shortcut")) {
        const auto *shortcut = qobject_cast<const Shortcut *>(object);
        return shortcut->keys();
    } else if (object->inherits("QShortcut")) {
        const auto *qshortcut = qobject_cast<const QShortcut *>(object);
        return { qshortcut->key() };
    } else if (object->property("shortcut").isValid()) {
        return { object->property("shortcut").value<QKeySequence>() };
    } else {
        return { };
    }
}
