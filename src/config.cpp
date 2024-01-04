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

const QSet<uint32_t> defaultWarpBehaviors_RSE = {
    0x0E, // MB_MOSSDEEP_GYM_WARP
    0x0F, // MB_MT_PYRE_HOLE
    0x1B, // MB_STAIRS_OUTSIDE_ABANDONED_SHIP
    0x1C, // MB_SHOAL_CAVE_ENTRANCE
    0x29, // MB_LAVARIDGE_GYM_B1F_WARP
    0x60, // MB_NON_ANIMATED_DOOR
    0x61, // MB_LADDER
    0x62, // MB_EAST_ARROW_WARP
    0x63, // MB_WEST_ARROW_WARP
    0x64, // MB_NORTH_ARROW_WARP
    0x65, // MB_SOUTH_ARROW_WARP
    0x67, // MB_AQUA_HIDEOUT_WARP
    0x68, // MB_LAVARIDGE_GYM_1F_WARP
    0x69, // MB_ANIMATED_DOOR
    0x6A, // MB_UP_ESCALATOR
    0x6B, // MB_DOWN_ESCALATOR
    0x6C, // MB_WATER_DOOR
    0x6D, // MB_WATER_SOUTH_ARROW_WARP
    0x6E, // MB_DEEP_SOUTH_WARP
    0x70, // MB_UNION_ROOM_WARP
    0x8D, // MB_PETALBURG_GYM_DOOR
    0x91, // MB_SECRET_BASE_SPOT_RED_CAVE_OPEN
    0x93, // MB_SECRET_BASE_SPOT_BROWN_CAVE_OPEN
    0x95, // MB_SECRET_BASE_SPOT_YELLOW_CAVE_OPEN
    0x97, // MB_SECRET_BASE_SPOT_TREE_LEFT_OPEN
    0x99, // MB_SECRET_BASE_SPOT_SHRUB_OPEN
    0x9B, // MB_SECRET_BASE_SPOT_BLUE_CAVE_OPEN
    0x9D, // MB_SECRET_BASE_SPOT_TREE_RIGHT_OPEN
};

const QSet<uint32_t> defaultWarpBehaviors_FRLG = {
    0x60, // MB_CAVE_DOOR
    0x61, // MB_LADDER
    0x62, // MB_EAST_ARROW_WARP
    0x63, // MB_WEST_ARROW_WARP
    0x64, // MB_NORTH_ARROW_WARP
    0x65, // MB_SOUTH_ARROW_WARP
    0x66, // MB_FALL_WARP
    0x67, // MB_REGULAR_WARP
    0x68, // MB_LAVARIDGE_1F_WARP
    0x69, // MB_WARP_DOOR
    0x6A, // MB_UP_ESCALATOR
    0x6B, // MB_DOWN_ESCALATOR
    0x6C, // MB_UP_RIGHT_STAIR_WARP
    0x6D, // MB_UP_LEFT_STAIR_WARP
    0x6E, // MB_DOWN_RIGHT_STAIR_WARP
    0x6F, // MB_DOWN_LEFT_STAIR_WARP
    0x71, // MB_UNION_ROOM_WARP
};

// TODO: symbol_wild_encounters should ultimately be removed from the table below. We can determine this name when we read the project.
const QMap<ProjectIdentifier, QPair<QString, QString>> ProjectConfig::defaultIdentifiers = {
    // Symbols
    {ProjectIdentifier::symbol_facing_directions,      {"symbol_facing_directions",      "gInitialMovementTypeFacingDirections"}},
    {ProjectIdentifier::symbol_obj_event_gfx_pointers, {"symbol_obj_event_gfx_pointers", "gObjectEventGraphicsInfoPointers"}},
    {ProjectIdentifier::symbol_pokemon_icon_table,     {"symbol_pokemon_icon_table",     "gMonIconTable"}},
    {ProjectIdentifier::symbol_wild_encounters,        {"symbol_wild_encounters",        "gWildMonHeaders"}},
    {ProjectIdentifier::symbol_heal_locations_type,    {"symbol_heal_locations_type",    "struct HealLocation"}},
    {ProjectIdentifier::symbol_heal_locations,         {"symbol_heal_locations",         "sHealLocations"}},
    {ProjectIdentifier::symbol_spawn_points,           {"symbol_spawn_points",           "sSpawnPoints"}},
    {ProjectIdentifier::symbol_spawn_maps,             {"symbol_spawn_maps",             "u16 sWhiteoutRespawnHealCenterMapIdxs"}},
    {ProjectIdentifier::symbol_spawn_npcs,             {"symbol_spawn_npcs",             "u8 sWhiteoutRespawnHealerNpcIds"}},
    {ProjectIdentifier::symbol_attribute_table,        {"symbol_attribute_table",        "sMetatileAttrMasks"}},
    {ProjectIdentifier::symbol_tilesets_prefix,        {"symbol_tilesets_prefix",        "gTileset_"}},
    // Defines
    {ProjectIdentifier::define_obj_event_count,        {"define_obj_event_count",        "OBJECT_EVENT_TEMPLATES_COUNT"}},
    {ProjectIdentifier::define_min_level,              {"define_min_level",              "MIN_LEVEL"}},
    {ProjectIdentifier::define_max_level,              {"define_max_level",              "MAX_LEVEL"}},
    {ProjectIdentifier::define_tiles_primary,          {"define_tiles_primary",          "NUM_TILES_IN_PRIMARY"}},
    {ProjectIdentifier::define_tiles_total,            {"define_tiles_total",            "NUM_TILES_TOTAL"}},
    {ProjectIdentifier::define_metatiles_primary,      {"define_metatiles_primary",      "NUM_METATILES_IN_PRIMARY"}},
    {ProjectIdentifier::define_pals_primary,           {"define_pals_primary",           "NUM_PALS_IN_PRIMARY"}},
    {ProjectIdentifier::define_pals_total,             {"define_pals_total",             "NUM_PALS_TOTAL"}},
    {ProjectIdentifier::define_map_size,               {"define_map_size",               "MAX_MAP_DATA_SIZE"}},
    {ProjectIdentifier::define_mask_metatile,          {"define_mask_metatile",          "MAPGRID_METATILE_ID_MASK"}},
    {ProjectIdentifier::define_mask_collision,         {"define_mask_collision",         "MAPGRID_COLLISION_MASK"}},
    {ProjectIdentifier::define_mask_elevation,         {"define_mask_elevation",         "MAPGRID_ELEVATION_MASK"}},
    {ProjectIdentifier::define_mask_behavior,          {"define_mask_behavior",          "METATILE_ATTR_BEHAVIOR_MASK"}},
    {ProjectIdentifier::define_mask_layer,             {"define_mask_layer",             "METATILE_ATTR_LAYER_MASK"}},
    {ProjectIdentifier::define_attribute_behavior,     {"define_attribute_behavior",     "METATILE_ATTRIBUTE_BEHAVIOR"}},
    {ProjectIdentifier::define_attribute_layer,        {"define_attribute_layer",        "METATILE_ATTRIBUTE_LAYER_TYPE"}},
    {ProjectIdentifier::define_attribute_terrain,      {"define_attribute_terrain",      "METATILE_ATTRIBUTE_TERRAIN"}},
    {ProjectIdentifier::define_attribute_encounter,    {"define_attribute_encounter",    "METATILE_ATTRIBUTE_ENCOUNTER_TYPE"}},
    {ProjectIdentifier::define_metatile_label_prefix,  {"define_metatile_label_prefix",  "METATILE_"}},
    {ProjectIdentifier::define_heal_locations_prefix,  {"define_heal_locations_prefix",  "HEAL_LOCATION_"}},
    {ProjectIdentifier::define_spawn_prefix,           {"define_spawn_prefix",           "SPAWN_"}},
    {ProjectIdentifier::define_map_prefix,             {"define_map_prefix",             "MAP_"}},
    {ProjectIdentifier::define_map_dynamic,            {"define_map_dynamic",            "DYNAMIC"}},
    {ProjectIdentifier::define_map_empty,              {"define_map_empty",              "UNDEFINED"}},
    {ProjectIdentifier::define_map_section_prefix,     {"define_map_section_prefix",     "MAPSEC_"}},
    {ProjectIdentifier::define_map_section_empty,      {"define_map_section_empty",      "NONE"}},
    {ProjectIdentifier::define_map_section_count,      {"define_map_section_count",      "COUNT"}},
    // Regex
    {ProjectIdentifier::regex_behaviors,               {"regex_behaviors",               "\\bMB_"}},
    {ProjectIdentifier::regex_obj_event_gfx,           {"regex_obj_event_gfx",           "\\bOBJ_EVENT_GFX_"}},
    {ProjectIdentifier::regex_items,                   {"regex_items",                   "\\bITEM_(?!(B_)?USE_)"}}, // Exclude ITEM_USE_ and ITEM_B_USE_ constants
    {ProjectIdentifier::regex_flags,                   {"regex_flags",                   "\\bFLAG_"}},
    {ProjectIdentifier::regex_vars,                    {"regex_vars",                    "\\bVAR_"}},
    {ProjectIdentifier::regex_movement_types,          {"regex_movement_types",          "\\bMOVEMENT_TYPE_"}},
    {ProjectIdentifier::regex_map_types,               {"regex_map_types",               "\\bMAP_TYPE_"}},
    {ProjectIdentifier::regex_battle_scenes,           {"regex_battle_scenes",           "\\bMAP_BATTLE_SCENE_"}},
    {ProjectIdentifier::regex_weather,                 {"regex_weather",                 "\\bWEATHER_"}},
    {ProjectIdentifier::regex_coord_event_weather,     {"regex_coord_event_weather",     "\\bCOORD_EVENT_WEATHER_"}},
    {ProjectIdentifier::regex_secret_bases,            {"regex_secret_bases",            "\\bSECRET_BASE_[A-Za-z0-9_]*_[0-9]+"}},
    {ProjectIdentifier::regex_sign_facing_directions,  {"regex_sign_facing_directions",  "\\bBG_EVENT_PLAYER_FACING_"}},
    {ProjectIdentifier::regex_trainer_types,           {"regex_trainer_types",           "\\bTRAINER_TYPE_"}},
    {ProjectIdentifier::regex_music,                   {"regex_music",                   "\\b(SE|MUS)_"}},
    {ProjectIdentifier::regex_species,                 {"regex_species",                 "\\bSPECIES_"}},
};

const QMap<ProjectFilePath, QPair<QString, QString>> ProjectConfig::defaultPaths = {
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
    {ProjectFilePath::constants_species,                { "constants_species",               "include/constants/species.h"}},
    {ProjectFilePath::constants_fieldmap,               { "constants_fieldmap",              "include/fieldmap.h"}},
    {ProjectFilePath::global_fieldmap,                  { "global_fieldmap",                 "include/global.fieldmap.h"}},
    {ProjectFilePath::fieldmap,                         { "fieldmap",                        "src/fieldmap.c"}},
    {ProjectFilePath::pokemon_icon_table,               { "pokemon_icon_table",              "src/pokemon_icon.c"}},
    {ProjectFilePath::initial_facing_table,             { "initial_facing_table",            "src/event_object_movement.c"}},
    {ProjectFilePath::pokemon_gfx,                      { "pokemon_gfx",                     "graphics/pokemon/"}},
};

ProjectIdentifier reverseDefaultIdentifier(QString str) {
    for (auto i = ProjectConfig::defaultIdentifiers.cbegin(), end = ProjectConfig::defaultIdentifiers.cend(); i != end; i++) {
        if (i.value().first == str) return i.key();
    }
    return static_cast<ProjectIdentifier>(-1);
}

ProjectFilePath reverseDefaultPaths(QString str) {
    for (auto it = ProjectConfig::defaultPaths.constKeyValueBegin(); it != ProjectConfig::defaultPaths.constKeyValueEnd(); ++it) {
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
    if (this->saveDisabled)
        return;

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

// For temporarily disabling saving during frequent config changes.
void KeyValueConfigBase::setSaveDisabled(bool disabled) {
    this->saveDisabled = disabled;
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
        this->recentProjects = value.split(",", Qt::SkipEmptyParts);
        this->recentProjects.removeDuplicates();
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
    } else if (key == "project_settings_editor_geometry") {
        this->projectSettingsEditorGeometry = bytesFromString(value);
    } else if (key == "project_settings_editor_state") {
        this->projectSettingsEditorState = bytesFromString(value);
    } else if (key == "custom_scripts_editor_geometry") {
        this->customScriptsEditorGeometry = bytesFromString(value);
    } else if (key == "custom_scripts_editor_state") {
        this->customScriptsEditorState = bytesFromString(value);
    } else if (key == "metatiles_zoom") {
        this->metatilesZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "collision_zoom") {
        this->collisionZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "show_player_view") {
        this->showPlayerView = getConfigBool(key, value);
    } else if (key == "show_cursor_tile") {
        this->showCursorTile = getConfigBool(key, value);
    } else if (key == "show_border") {
        this->showBorder = getConfigBool(key, value);
    } else if (key == "show_grid") {
        this->showGrid = getConfigBool(key, value);
    } else if (key == "show_tileset_editor_metatile_grid") {
        this->showTilesetEditorMetatileGrid = getConfigBool(key, value);
    } else if (key == "show_tileset_editor_layer_grid") {
        this->showTilesetEditorLayerGrid = getConfigBool(key, value);
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
    } else if (key == "project_settings_tab") {
        this->projectSettingsTab = getConfigInteger(key, value, 0);
    } else if (key == "warp_behavior_warning_disabled") {
        this->warpBehaviorWarningDisabled = getConfigBool(key, value);
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProjects.join(","));
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
    map.insert("project_settings_editor_geometry", stringFromByteArray(this->projectSettingsEditorGeometry));
    map.insert("project_settings_editor_state", stringFromByteArray(this->projectSettingsEditorState));
    map.insert("custom_scripts_editor_geometry", stringFromByteArray(this->customScriptsEditorGeometry));
    map.insert("custom_scripts_editor_state", stringFromByteArray(this->customScriptsEditorState));
    map.insert("collision_opacity", QString::number(this->collisionOpacity));
    map.insert("collision_zoom", QString::number(this->collisionZoom));
    map.insert("metatiles_zoom", QString::number(this->metatilesZoom));
    map.insert("show_player_view", this->showPlayerView ? "1" : "0");
    map.insert("show_cursor_tile", this->showCursorTile ? "1" : "0");
    map.insert("show_border", this->showBorder ? "1" : "0");
    map.insert("show_grid", this->showGrid ? "1" : "0");
    map.insert("show_tileset_editor_metatile_grid", this->showTilesetEditorMetatileGrid ? "1" : "0");
    map.insert("show_tileset_editor_layer_grid", this->showTilesetEditorLayerGrid ? "1" : "0");
    map.insert("monitor_files", this->monitorFiles ? "1" : "0");
    map.insert("tileset_checkerboard_fill", this->tilesetCheckerboardFill ? "1" : "0");
    map.insert("theme", this->theme);
    map.insert("text_editor_open_directory", this->textEditorOpenFolder);
    map.insert("text_editor_goto_line", this->textEditorGotoLine);
    map.insert("palette_editor_bit_depth", QString::number(this->paletteEditorBitDepth));
    map.insert("project_settings_tab", QString::number(this->projectSettingsTab));
    map.insert("warp_behavior_warning_disabled", QString::number(this->warpBehaviorWarningDisabled));
    
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

void PorymapConfig::addRecentProject(QString project) {
    this->recentProjects.removeOne(project);
    this->recentProjects.prepend(project);
    this->save();
}

void PorymapConfig::setRecentProjects(QStringList projects) {
    this->recentProjects = projects;
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

void PorymapConfig::setProjectSettingsEditorGeometry(QByteArray projectSettingsEditorGeometry_, QByteArray projectSettingsEditorState_) {
    this->projectSettingsEditorGeometry = projectSettingsEditorGeometry_;
    this->projectSettingsEditorState = projectSettingsEditorState_;
    this->save();
}

void PorymapConfig::setCustomScriptsEditorGeometry(QByteArray customScriptsEditorGeometry_, QByteArray customScriptsEditorState_) {
    this->customScriptsEditorGeometry = customScriptsEditorGeometry_;
    this->customScriptsEditorState = customScriptsEditorState_;
    this->save();
}

void PorymapConfig::setCollisionOpacity(int opacity) {
    this->collisionOpacity = opacity;
    // don't auto-save here because this can be called very frequently.
}

void PorymapConfig::setCollisionZoom(int zoom) {
    this->collisionZoom = zoom;
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

void PorymapConfig::setShowTilesetEditorMetatileGrid(bool enabled) {
    this->showTilesetEditorMetatileGrid = enabled;
    this->save();
}

void PorymapConfig::setShowTilesetEditorLayerGrid(bool enabled) {
    this->showTilesetEditorLayerGrid = enabled;
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

void PorymapConfig::setProjectSettingsTab(int tab) {
    this->projectSettingsTab = tab;
    this->save();
}

QString PorymapConfig::getRecentProject() {
    return this->recentProjects.value(0);
}

QStringList PorymapConfig::getRecentProjects() {
    return this->recentProjects;
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

QMap<QString, QByteArray> PorymapConfig::getProjectSettingsEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("project_settings_editor_geometry", this->projectSettingsEditorGeometry);
    geometry.insert("project_settings_editor_state", this->projectSettingsEditorState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getCustomScriptsEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("custom_scripts_editor_geometry", this->customScriptsEditorGeometry);
    geometry.insert("custom_scripts_editor_state", this->customScriptsEditorState);

    return geometry;
}

int PorymapConfig::getCollisionOpacity() {
    return this->collisionOpacity;
}

int PorymapConfig::getCollisionZoom() {
    return this->collisionZoom;
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

bool PorymapConfig::getShowTilesetEditorMetatileGrid() {
    return this->showTilesetEditorMetatileGrid;
}

bool PorymapConfig::getShowTilesetEditorLayerGrid() {
    return this->showTilesetEditorLayerGrid;
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

int PorymapConfig::getProjectSettingsTab() {
    return this->projectSettingsTab;
}

void PorymapConfig::setWarpBehaviorWarningDisabled(bool disabled) {
    this->warpBehaviorWarningDisabled = disabled;
    this->save();
}

bool PorymapConfig::getWarpBehaviorWarningDisabled() {
    return this->warpBehaviorWarningDisabled;
}

const QStringList ProjectConfig::versionStrings = {
    "pokeruby",
    "pokefirered",
    "pokeemerald",
};

const QMap<BaseGameVersion, QString> baseGameVersionMap = {
    {BaseGameVersion::pokeruby, ProjectConfig::versionStrings[0]},
    {BaseGameVersion::pokefirered, ProjectConfig::versionStrings[1]},
    {BaseGameVersion::pokeemerald, ProjectConfig::versionStrings[2]},
};

const QMap<QString, BaseGameVersion> baseGameVersionReverseMap = {
    {ProjectConfig::versionStrings[0], BaseGameVersion::pokeruby},
    {ProjectConfig::versionStrings[1], BaseGameVersion::pokefirered},
    {ProjectConfig::versionStrings[2], BaseGameVersion::pokeemerald},
};

BaseGameVersion ProjectConfig::stringToBaseGameVersion(QString string, bool * ok) {
    if (baseGameVersionReverseMap.contains(string)) {
        if (ok) *ok = true;
        return baseGameVersionReverseMap.value(string);
    } else {
        if (ok) *ok = false;
        return BaseGameVersion::pokeemerald;
    }
}

ProjectConfig projectConfig;

QString ProjectConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    return QDir(this->projectDir).filePath("porymap.project.cfg");
}

void ProjectConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "base_game_version") {
        bool ok;
        this->baseGameVersion = this->stringToBaseGameVersion(value.toLower(), &ok);
        if (!ok)
            logWarn(QString("Invalid config value for base_game_version: '%1'. Must be 'pokeruby', 'pokefirered' or 'pokeemerald'.").arg(value));
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
    } else if (key == "default_metatile") {
        this->defaultMetatileId = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "default_elevation") {
        this->defaultElevation = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "default_collision") {
        this->defaultCollision = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "new_map_border_metatiles") {
        this->newMapBorderMetatileIds.clear();
        QList<QString> metatileIds = value.split(",");
        for (int i = 0; i < metatileIds.size(); i++) {
            int metatileId = getConfigUint32(key, metatileIds.at(i), 0, Block::maxValue);
            this->newMapBorderMetatileIds.append(metatileId);
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
        this->metatileBehaviorMask = getConfigUint32(key, value);
    } else if (key == "metatile_terrain_type_mask") {
        this->metatileTerrainTypeMask = getConfigUint32(key, value);
    } else if (key == "metatile_encounter_type_mask") {
        this->metatileEncounterTypeMask = getConfigUint32(key, value);
    } else if (key == "metatile_layer_type_mask") {
        this->metatileLayerTypeMask = getConfigUint32(key, value);
    } else if (key == "block_metatile_id_mask") {
        this->blockMetatileIdMask = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "block_collision_mask") {
        this->blockCollisionMask = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "block_elevation_mask") {
        this->blockElevationMask = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "enable_map_allow_flags") {
        this->enableMapAllowFlags = getConfigBool(key, value);
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    } else if (key == "recent_map") {
        userConfig.setRecentMap(value);
    } else if (key == "use_encounter_json") {
        userConfig.useEncounterJson = getConfigBool(key, value);
    } else if (key == "custom_scripts") {
        userConfig.parseCustomScripts(value);
#endif
    } else if (key.startsWith("path/")) {
        auto k = reverseDefaultPaths(key.mid(5));
        if (k != static_cast<ProjectFilePath>(-1)) {
            this->setFilePath(k, value);
        } else {
            logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
        }
    } else if (key.startsWith("ident/")) {
        auto identifierId = reverseDefaultIdentifier(key.mid(6));
        if (identifierId != static_cast<ProjectIdentifier>(-1)) {
            this->setIdentifier(identifierId, value);
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
    } else if (key == "event_icon_path_object") {
        this->eventIconPaths[Event::Group::Object] = value;
    } else if (key == "event_icon_path_warp") {
        this->eventIconPaths[Event::Group::Warp] = value;
    } else if (key == "event_icon_path_coord") {
        this->eventIconPaths[Event::Group::Coord] = value;
    } else if (key == "event_icon_path_bg") {
        this->eventIconPaths[Event::Group::Bg] = value;
    } else if (key == "event_icon_path_heal") {
        this->eventIconPaths[Event::Group::Heal] = value;
    } else if (key.startsWith("pokemon_icon_path/")) {
        this->pokemonIconPaths.insert(key.mid(18).toUpper(), value);
    } else if (key == "collision_sheet_path") {
        this->collisionSheetPath = value;
    } else if (key == "collision_sheet_width") {
        this->collisionSheetWidth = getConfigUint32(key, value, 1, Block::maxValue);
    } else if (key == "collision_sheet_height") {
        this->collisionSheetHeight = getConfigUint32(key, value, 1, Block::maxValue);
    } else if (key == "warp_behaviors") {
        this->warpBehaviors.clear();
        value.remove(" ");
        QStringList behaviorList = value.split(",", Qt::SkipEmptyParts);
        for (auto s : behaviorList)
            this->warpBehaviors.insert(getConfigUint32(key, s));
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
    readKeys.append(key);
}

// Restore config to version-specific defaults
void::ProjectConfig::reset(BaseGameVersion baseGameVersion) {
    this->reset();
    this->baseGameVersion = baseGameVersion;
    this->setUnreadKeys();
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
    if (!readKeys.contains("metatile_behavior_mask")) this->metatileBehaviorMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::Behavior);
    if (!readKeys.contains("metatile_terrain_type_mask")) this->metatileTerrainTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::TerrainType);
    if (!readKeys.contains("metatile_encounter_type_mask")) this->metatileEncounterTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::EncounterType);
    if (!readKeys.contains("metatile_layer_type_mask")) this->metatileLayerTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::LayerType);
    if (!readKeys.contains("enable_map_allow_flags")) this->enableMapAllowFlags = (this->baseGameVersion != BaseGameVersion::pokeruby);
    if (!readKeys.contains("warp_behaviors")) this->warpBehaviors = isPokefirered ? defaultWarpBehaviors_FRLG : defaultWarpBehaviors_RSE;
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
    map.insert("default_metatile", Metatile::getMetatileIdString(this->defaultMetatileId));
    map.insert("default_elevation", QString::number(this->defaultElevation));
    map.insert("default_collision", QString::number(this->defaultCollision));
    map.insert("new_map_border_metatiles", Metatile::getMetatileIdStrings(this->newMapBorderMetatileIds));
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
    map.insert("block_metatile_id_mask", "0x" + QString::number(this->blockMetatileIdMask, 16).toUpper());
    map.insert("block_collision_mask", "0x" + QString::number(this->blockCollisionMask, 16).toUpper());
    map.insert("block_elevation_mask", "0x" + QString::number(this->blockElevationMask, 16).toUpper());
    map.insert("enable_map_allow_flags", QString::number(this->enableMapAllowFlags));
    map.insert("event_icon_path_object", this->eventIconPaths[Event::Group::Object]);
    map.insert("event_icon_path_warp", this->eventIconPaths[Event::Group::Warp]);
    map.insert("event_icon_path_coord", this->eventIconPaths[Event::Group::Coord]);
    map.insert("event_icon_path_bg", this->eventIconPaths[Event::Group::Bg]);
    map.insert("event_icon_path_heal", this->eventIconPaths[Event::Group::Heal]);
    for (auto i = this->pokemonIconPaths.cbegin(), end = this->pokemonIconPaths.cend(); i != end; i++){
        const QString path = i.value();
        if (!path.isEmpty()) map.insert("pokemon_icon_path/" + i.key(), path);
    }
    for (auto i = this->identifiers.cbegin(), end = this->identifiers.cend(); i != end; i++) {
        map.insert("ident/"+defaultIdentifiers.value(i.key()).first, i.value());
    }
    map.insert("collision_sheet_path", this->collisionSheetPath);
    map.insert("collision_sheet_width", QString::number(this->collisionSheetWidth));
    map.insert("collision_sheet_height", QString::number(this->collisionSheetHeight));
    QStringList warpBehaviorStrs;
    for (auto value : this->warpBehaviors)
        warpBehaviorStrs.append("0x" + QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
    map.insert("warp_behaviors", warpBehaviorStrs.join(","));

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

void ProjectConfig::setFilePath(ProjectFilePath pathId, const QString &path) {
    if (!defaultPaths.contains(pathId)) return;
    if (path.isEmpty()) {
        this->filePaths.remove(pathId);
    } else {
        this->filePaths[pathId] = path;
    }
}

void ProjectConfig::setFilePath(const QString &pathId, const QString &path) {
    this->setFilePath(reverseDefaultPaths(pathId), path);
}

QString ProjectConfig::getCustomFilePath(ProjectFilePath pathId) {
    return this->filePaths.value(pathId);
}

QString ProjectConfig::getCustomFilePath(const QString &pathId) {
    return this->getCustomFilePath(reverseDefaultPaths(pathId));
}

QString ProjectConfig::getFilePath(ProjectFilePath pathId) {
    const QString customPath = this->getCustomFilePath(pathId);
    if (!customPath.isEmpty()) {
        // A custom filepath has been specified. If the file/folder exists, use that.
        const QString absCustomPath = this->projectDir + QDir::separator() + customPath;
        if (QFileInfo::exists(absCustomPath)) {
            return customPath;
        } else {
            logError(QString("Custom project filepath '%1' not found. Using default.").arg(absCustomPath));
        }
    }
    return defaultPaths.contains(pathId) ? defaultPaths[pathId].second : QString();

}

void ProjectConfig::setIdentifier(ProjectIdentifier id, const QString &text) {
    if (!defaultIdentifiers.contains(id)) return;
    QString copy(text);
    if (copy.isEmpty()) {
        this->identifiers.remove(id);
    } else {
        this->identifiers[id] = copy;
    }
}

void ProjectConfig::setIdentifier(const QString &id, const QString &text) {
    this->setIdentifier(reverseDefaultIdentifier(id), text);
}

QString ProjectConfig::getCustomIdentifier(ProjectIdentifier id) {
    return this->identifiers.value(id);
}

QString ProjectConfig::getCustomIdentifier(const QString &id) {
    return this->getCustomIdentifier(reverseDefaultIdentifier(id));
}

QString ProjectConfig::getIdentifier(ProjectIdentifier id) {
    const QString customText = this->getCustomIdentifier(id);
    if (!customText.isEmpty())
        return customText;
    return defaultIdentifiers.contains(id) ? defaultIdentifiers[id].second : QString();
}

void ProjectConfig::setBaseGameVersion(BaseGameVersion baseGameVersion) {
    this->baseGameVersion = baseGameVersion;
    this->save();
}

BaseGameVersion ProjectConfig::getBaseGameVersion() {
    return this->baseGameVersion;
}

QString ProjectConfig::getBaseGameVersionString(BaseGameVersion version) {
    if (!baseGameVersionMap.contains(version)) {
        version = BaseGameVersion::pokeemerald;
    }
    return baseGameVersionMap.value(version);
}

QString ProjectConfig::getBaseGameVersionString() {
    return this->getBaseGameVersionString(this->baseGameVersion);
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

void ProjectConfig::setDefaultMetatileId(uint16_t metatileId) {
    this->defaultMetatileId = metatileId;
    this->save();
}

uint16_t ProjectConfig::getDefaultMetatileId() {
    return this->defaultMetatileId;
}

void ProjectConfig::setDefaultElevation(uint16_t elevation) {
    this->defaultElevation = elevation;
    this->save();
}

uint16_t ProjectConfig::getDefaultElevation() {
    return this->defaultElevation;
}

void ProjectConfig::setDefaultCollision(uint16_t collision) {
    this->defaultCollision = collision;
    this->save();
}

uint16_t ProjectConfig::getDefaultCollision() {
    return this->defaultCollision;
}

void ProjectConfig::setNewMapBorderMetatileIds(QList<uint16_t> metatileIds) {
    this->newMapBorderMetatileIds = metatileIds;
    this->save();
}

QList<uint16_t> ProjectConfig::getNewMapBorderMetatileIds() {
    return this->newMapBorderMetatileIds;
}

QString ProjectConfig::getDefaultPrimaryTileset() {
    return this->defaultPrimaryTileset;
}

QString ProjectConfig::getDefaultSecondaryTileset() {
    return this->defaultSecondaryTileset;
}

void ProjectConfig::setDefaultPrimaryTileset(QString tilesetName) {
    this->defaultPrimaryTileset = tilesetName;
    this->save();
}

void ProjectConfig::setDefaultSecondaryTileset(QString tilesetName) {
    this->defaultSecondaryTileset = tilesetName;
    this->save();
}

void ProjectConfig::setPrefabFilepath(QString filepath) {
    this->prefabFilepath = filepath;
    this->save();
}

QString ProjectConfig::getPrefabFilepath() {
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

void ProjectConfig::setMetatileAttributesSize(int size) {
    this->metatileAttributesSize = size;
    this->save();
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

void ProjectConfig::setMetatileBehaviorMask(uint32_t mask) {
    this->metatileBehaviorMask = mask;
    this->save();
}

void ProjectConfig::setMetatileTerrainTypeMask(uint32_t mask) {
    this->metatileTerrainTypeMask = mask;
    this->save();
}

void ProjectConfig::setMetatileEncounterTypeMask(uint32_t mask) {
    this->metatileEncounterTypeMask = mask;
    this->save();
}

void ProjectConfig::setMetatileLayerTypeMask(uint32_t mask) {
    this->metatileLayerTypeMask = mask;
    this->save();
}

uint16_t ProjectConfig::getBlockMetatileIdMask() {
    return this->blockMetatileIdMask;
}

uint16_t ProjectConfig::getBlockCollisionMask() {
    return this->blockCollisionMask;
}

uint16_t ProjectConfig::getBlockElevationMask() {
    return this->blockElevationMask;
}

void ProjectConfig::setBlockMetatileIdMask(uint16_t mask) {
    this->blockMetatileIdMask = mask;
    this->save();
}

void ProjectConfig::setBlockCollisionMask(uint16_t mask) {
    this->blockCollisionMask = mask;
    this->save();
}

void ProjectConfig::setBlockElevationMask(uint16_t mask) {
    this->blockElevationMask = mask;
    this->save();
}

bool ProjectConfig::getMapAllowFlagsEnabled() {
    return this->enableMapAllowFlags;
}

void ProjectConfig::setMapAllowFlagsEnabled(bool enabled) {
    this->enableMapAllowFlags = enabled;
    this->save();
}

void ProjectConfig::setEventIconPath(Event::Group group, const QString &path) {
    this->eventIconPaths[group] = path;
    this->save();
}

QString ProjectConfig::getEventIconPath(Event::Group group) {
    return this->eventIconPaths.value(group);
}

void ProjectConfig::setPokemonIconPath(const QString &species, const QString &path) {
    this->pokemonIconPaths[species] = path;
    this->save();
}

QString ProjectConfig::getPokemonIconPath(const QString &species) {
    return this->pokemonIconPaths.value(species);
}

QHash<QString, QString> ProjectConfig::getPokemonIconPaths() {
    return this->pokemonIconPaths;
}

void ProjectConfig::setCollisionSheetPath(const QString &path) {
    this->collisionSheetPath = path;
    this->save();
}

QString ProjectConfig::getCollisionSheetPath() {
    return this->collisionSheetPath;
}

void ProjectConfig::setCollisionSheetWidth(int width) {
    this->collisionSheetWidth = width;
    this->save();
}

int ProjectConfig::getCollisionSheetWidth() {
    return this->collisionSheetWidth;
}

void ProjectConfig::setCollisionSheetHeight(int height) {
    this->collisionSheetHeight = height;
    this->save();
}

int ProjectConfig::getCollisionSheetHeight() {
    return this->collisionSheetHeight;
}

void ProjectConfig::setWarpBehaviors(const QSet<uint32_t> &behaviors) {
    this->warpBehaviors = behaviors;
    this->save();
}

QSet<uint32_t> ProjectConfig::getWarpBehaviors() {
    return this->warpBehaviors;
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
        this->parseCustomScripts(value);
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
    map.insert("custom_scripts", this->outputCustomScripts());
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
    return this->useEncounterJson;
}

// Read input from the config to get the script paths and whether each is enabled or disbled.
// The format is a comma-separated list of paths. Each path can be followed (before the comma)
// by a :0 or :1 to indicate whether it should be disabled or enabled, respectively. If neither
// follow, it's assumed the script should be enabled.
void UserConfig::parseCustomScripts(QString input) {
    this->customScripts.clear();
    QList<QString> paths = input.split(",", Qt::SkipEmptyParts);
    for (QString path : paths) {
        // Read and remove suffix
        bool enabled = !path.endsWith(":0");
        if (!enabled || path.endsWith(":1"))
            path.chop(2);

        if (!path.isEmpty()) {
            // If a path is repeated only its last instance will be considered.
            this->customScripts.insert(path, enabled);
        }
    }
}

// Inverse of UserConfig::parseCustomScripts
QString UserConfig::outputCustomScripts() {
    QStringList list;
    QMapIterator<QString, bool> i(this->customScripts);
    while (i.hasNext()) {
        i.next();
        list.append(QString("%1:%2").arg(i.key()).arg(i.value() ? "1" : "0"));
    }
    return list.join(",");
}

void UserConfig::setCustomScripts(QStringList scripts, QList<bool> enabled) {
    this->customScripts.clear();
    size_t size = qMin(scripts.length(), enabled.length());
    for (size_t i = 0; i < size; i++)
        this->customScripts.insert(scripts.at(i), enabled.at(i));
    this->save();
}

QStringList UserConfig::getCustomScriptPaths() {
    return this->customScripts.keys();
}

QList<bool> UserConfig::getCustomScriptsEnabled() {
    return this->customScripts.values();
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
