#include "config.h"
#include "log.h"
#include "shortcut.h"
#include "map.h"
#include "validator.h"
#include "utility.h"
#include "metatile.h"
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
#include <QAction>
#include <QAbstractButton>

const QVersionNumber porymapVersion = QVersionNumber::fromString(PORYMAP_VERSION);

// In both versions the default new map border is a generic tree
const QList<uint16_t> defaultBorder_RSE = {0x1D4, 0x1D5, 0x1DC, 0x1DD};
const QList<uint16_t> defaultBorder_FRLG = {0x14, 0x15, 0x1C, 0x1D};

const QList<uint32_t> defaultWarpBehaviors_RSE = {
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

const QList<uint32_t> defaultWarpBehaviors_FRLG = {
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

const QMap<ProjectIdentifier, QPair<QString, QString>> ProjectConfig::defaultIdentifiers = {
    // Symbols
    {ProjectIdentifier::symbol_facing_directions,      {"symbol_facing_directions",      "gInitialMovementTypeFacingDirections"}},
    {ProjectIdentifier::symbol_obj_event_gfx_pointers, {"symbol_obj_event_gfx_pointers", "gObjectEventGraphicsInfoPointers"}},
    {ProjectIdentifier::symbol_pokemon_icon_table,     {"symbol_pokemon_icon_table",     "gMonIconTable"}},
    {ProjectIdentifier::symbol_attribute_table,        {"symbol_attribute_table",        "sMetatileAttrMasks"}},
    {ProjectIdentifier::symbol_tilesets_prefix,        {"symbol_tilesets_prefix",        "gTileset_"}},
    {ProjectIdentifier::symbol_dynamic_map_name,       {"symbol_dynamic_map_name",       "Dynamic"}},
    // Defines
    {ProjectIdentifier::define_obj_event_count,        {"define_obj_event_count",        "OBJECT_EVENT_TEMPLATES_COUNT"}},
    {ProjectIdentifier::define_min_level,              {"define_min_level",              "MIN_LEVEL"}},
    {ProjectIdentifier::define_max_level,              {"define_max_level",              "MAX_LEVEL"}},
    {ProjectIdentifier::define_max_encounter_rate,     {"define_max_encounter_rate",     "MAX_ENCOUNTER_RATE"}},
    {ProjectIdentifier::define_tiles_primary,          {"define_tiles_primary",          "NUM_TILES_IN_PRIMARY"}},
    {ProjectIdentifier::define_tiles_total,            {"define_tiles_total",            "NUM_TILES_TOTAL"}},
    {ProjectIdentifier::define_metatiles_primary,      {"define_metatiles_primary",      "NUM_METATILES_IN_PRIMARY"}},
    {ProjectIdentifier::define_pals_primary,           {"define_pals_primary",           "NUM_PALS_IN_PRIMARY"}},
    {ProjectIdentifier::define_pals_total,             {"define_pals_total",             "NUM_PALS_TOTAL"}},
    {ProjectIdentifier::define_tiles_per_metatile,     {"define_tiles_per_metatile",     "NUM_TILES_PER_METATILE"}},
    {ProjectIdentifier::define_map_size,               {"define_map_size",               "MAX_MAP_DATA_SIZE"}},
    {ProjectIdentifier::define_map_offset_width,       {"define_map_offset_width",       "MAP_OFFSET_W"}},
    {ProjectIdentifier::define_map_offset_height,      {"define_map_offset_height",      "MAP_OFFSET_H"}},
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
    {ProjectIdentifier::define_layout_prefix,          {"define_layout_prefix",          "LAYOUT_"}},
    {ProjectIdentifier::define_map_prefix,             {"define_map_prefix",             "MAP_"}},
    {ProjectIdentifier::define_map_dynamic,            {"define_map_dynamic",            "MAP_DYNAMIC"}},
    {ProjectIdentifier::define_map_empty,              {"define_map_empty",              "MAP_UNDEFINED"}},
    {ProjectIdentifier::define_map_section_prefix,     {"define_map_section_prefix",     "MAPSEC_"}},
    {ProjectIdentifier::define_map_section_empty,      {"define_map_section_empty",      "NONE"}},
    {ProjectIdentifier::define_species_prefix,         {"define_species_prefix",         "SPECIES_"}},
    {ProjectIdentifier::define_species_empty,          {"define_species_empty",          "NONE"}},
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
    {ProjectIdentifier::regex_secret_bases,            {"regex_secret_bases",            "\\bSECRET_BASE_[\\w]+_[\\d]+"}},
    {ProjectIdentifier::regex_sign_facing_directions,  {"regex_sign_facing_directions",  "\\bBG_EVENT_PLAYER_FACING_"}},
    {ProjectIdentifier::regex_trainer_types,           {"regex_trainer_types",           "\\bTRAINER_TYPE_"}},
    {ProjectIdentifier::regex_music,                   {"regex_music",                   "\\b(SE|MUS)_"}},
    {ProjectIdentifier::regex_encounter_types,         {"regex_encounter_types",         "\\bTILE_ENCOUNTER_"}},
    {ProjectIdentifier::regex_terrain_types,           {"regex_terrain_types",           "\\bTILE_TERRAIN_"}},
    // Other
    {ProjectIdentifier::pals_output_extension,         {"pals_output_extension",         ".gbapal"}},
    {ProjectIdentifier::tiles_output_extension,        {"tiles_output_extension",        ".4bpp.lz"}},
};

const QMap<ProjectFilePath, QPair<QString, QString>> ProjectConfig::defaultPaths = {
    {ProjectFilePath::data_map_folders,                 { "data_map_folders",                "data/maps/"}},
    {ProjectFilePath::data_scripts_folders,             { "data_scripts_folders",            "data/scripts/"}},
    {ProjectFilePath::data_layouts_folders,             { "data_layouts_folders",            "data/layouts/"}},
    {ProjectFilePath::data_primary_tilesets_folders,    { "data_primary_tilesets_folders",   "data/tilesets/primary/"}},
    {ProjectFilePath::data_secondary_tilesets_folders,  { "data_secondary_tilesets_folders", "data/tilesets/secondary/"}},
    {ProjectFilePath::data_event_scripts,               { "data_event_scripts",              "data/event_scripts.s"}},
    {ProjectFilePath::json_map_groups,                  { "json_map_groups",                 "data/maps/map_groups.json"}},
    {ProjectFilePath::json_layouts,                     { "json_layouts",                    "data/layouts/layouts.json"}},
    {ProjectFilePath::json_wild_encounters,             { "json_wild_encounters",            "src/data/wild_encounters.json"}},
    {ProjectFilePath::json_heal_locations,              { "json_heal_locations",             "src/data/heal_locations.json"}},
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
    {ProjectFilePath::constants_global,                 { "constants_global",                "include/constants/global.h"}},
    {ProjectFilePath::constants_items,                  { "constants_items",                 "include/constants/items.h"}},
    {ProjectFilePath::constants_flags,                  { "constants_flags",                 "include/constants/flags.h"}},
    {ProjectFilePath::constants_vars,                   { "constants_vars",                  "include/constants/vars.h"}},
    {ProjectFilePath::constants_weather,                { "constants_weather",               "include/constants/weather.h"}},
    {ProjectFilePath::constants_songs,                  { "constants_songs",                 "include/constants/songs.h"}},
    {ProjectFilePath::constants_pokemon,                { "constants_pokemon",               "include/constants/pokemon.h"}},
    {ProjectFilePath::constants_map_types,              { "constants_map_types",             "include/constants/map_types.h"}},
    {ProjectFilePath::constants_trainer_types,          { "constants_trainer_types",         "include/constants/trainer_types.h"}},
    {ProjectFilePath::constants_secret_bases,           { "constants_secret_bases",          "include/constants/secret_bases.h"}},
    {ProjectFilePath::constants_obj_event_movement,     { "constants_obj_event_movement",    "include/constants/event_object_movement.h"}},
    {ProjectFilePath::constants_obj_events,             { "constants_obj_events",            "include/constants/event_objects.h"}},
    {ProjectFilePath::constants_event_bg,               { "constants_event_bg",              "include/constants/event_bg.h"}},
    {ProjectFilePath::constants_metatile_labels,        { "constants_metatile_labels",       "include/constants/metatile_labels.h"}},
    {ProjectFilePath::constants_metatile_behaviors,     { "constants_metatile_behaviors",    "include/constants/metatile_behaviors.h"}},
    {ProjectFilePath::constants_species,                { "constants_species",               "include/constants/species.h"}},
    {ProjectFilePath::constants_fieldmap,               { "constants_fieldmap",              "include/fieldmap.h"}},
    {ProjectFilePath::global_fieldmap,                  { "global_fieldmap",                 "include/global.fieldmap.h"}},
    {ProjectFilePath::fieldmap,                         { "fieldmap",                        "src/fieldmap.c"}},
    {ProjectFilePath::pokemon_icon_table,               { "pokemon_icon_table",              "src/pokemon_icon.c"}},
    {ProjectFilePath::initial_facing_table,             { "initial_facing_table",            "src/event_object_movement.c"}},
    {ProjectFilePath::wild_encounter,                   { "wild_encounter",                  "src/wild_encounter.c"}},
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

void KeyValueConfigBase::setRoot(const QString &root) {
    m_root = root;
    QDir dir(m_root);
    if (!m_root.isEmpty() && !dir.exists()) {
        dir.mkpath(m_root);
    }
    m_filepath = dir.absoluteFilePath(m_filename);
}

bool KeyValueConfigBase::load(const QString &root) {
    if (!root.isEmpty()) {
        setRoot(root);
    }
    reset();
    QFile file(this->filepath());
    if (file.exists() && !file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open config file '%1': ").arg(this->filepath()) + file.errorString());
        return false;
    } else if (file.size() == 0) {
        this->init();
    }

    QTextStream in(&file);
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
            logWarn(QString("Invalid config line in %1: '%2'").arg(this->filepath()).arg(line));
            continue;
        }

        this->parseConfigKeyValue(match.captured("key").trimmed(), match.captured("value").trimmed());
    }
    this->setUnreadKeys();

    file.close();
    return true;
}

bool KeyValueConfigBase::save() {
    QString text = "";
    QMap<QString, QString> map = this->getKeyValueMap();
    for (QMap<QString, QString>::iterator it = map.begin(); it != map.end(); it++) {
        text += QString("%1=%2\n").arg(it.key()).arg(it.value());
    }

    QFile file(this->filepath());
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open config file '%1' for writing: ").arg(this->filepath()) + file.errorString());
        return false;
    }

    file.write(text.toUtf8());
    file.close();
    return true;
}

bool KeyValueConfigBase::getConfigBool(const QString &key, const QString &value) {
    bool ok;
    int result = value.toInt(&ok, 0);
    if (!ok || (result != 0 && result != 1)) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be 0 or 1.").arg(key).arg(value));
    }
    return (result != 0);
}

int KeyValueConfigBase::getConfigInteger(const QString &key, const QString &value, int min, int max, int defaultValue) {
    bool ok;
    int result = value.toInt(&ok, 0);
    if (!ok) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be an integer. Using default value '%3'.").arg(key).arg(value).arg(defaultValue));
        result = defaultValue;
    }
    return qBound(min, result, max);
}

uint32_t KeyValueConfigBase::getConfigUint32(const QString &key, const QString &value, uint32_t min, uint32_t max, uint32_t defaultValue) {
    bool ok;
    uint32_t result = value.toUInt(&ok, 0);
    if (!ok) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be an integer. Using default value '%3'.").arg(key).arg(value).arg(defaultValue));
        result = defaultValue;
    }
    return qBound(min, result, max);
}

QColor KeyValueConfigBase::getConfigColor(const QString &key, const QString &value, const QColor &defaultValue) {
    if (value.isEmpty())
        return QColor();

    QColor color = QColor("#" + value);
    if (!color.isValid()) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be a color in the format 'RRGGBB'. Using default value '%3'.").arg(key).arg(value).arg(defaultValue.name()));
        color = defaultValue;
    }
    return color;
}

QString KeyValueConfigBase::toConfigColor(const QColor &color) {
    return color.isValid() ? color.name().remove("#") : QString(); // Our text config treats '#' as the start of a comment.
}

PorymapConfig porymapConfig;

PorymapConfig::PorymapConfig() : KeyValueConfigBase(QStringLiteral("porymap.cfg")) {
    reset();
}

void PorymapConfig::reset() {
    setRoot(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    this->recentProjects.clear();
    this->projectManuallyClosed = false;
    this->reopenOnLaunch = true;
    this->mapListTab = 0;
    this->mapListEditGroupsEnabled = false;
    this->mapListHideEmptyEnabled.clear();
    this->prettyCursors = true;
    this->mirrorConnectingMaps = true;
    this->showDiveEmergeMaps = false;
    this->diveEmergeMapOpacity = 30;
    this->diveMapOpacity = 15;
    this->emergeMapOpacity = 15;
    this->collisionOpacity = 50;
    this->collisionZoom = 30;
    this->metatilesZoom = 30;
    this->tilesetEditorMetatilesZoom = 30;
    this->tilesetEditorTilesZoom = 30;
    this->tilesetEditorLayerOrientation = Qt::Vertical;
    this->showPlayerView = false;
    this->showCursorTile = true;
    this->showBorder = true;
    this->showGrid = false;
    this->showTilesetEditorMetatileGrid = false;
    this->showTilesetEditorLayerGrid = true;
    this->showTilesetEditorDivider = false;
    this->showTilesetEditorRawAttributes = false;
    this->showPaletteEditorUnusedColors = false;
    this->monitorFiles = true;
    this->tilesetCheckerboardFill = true;
    this->newMapHeaderSectionExpanded = false;
    this->theme = "default";
    this->wildMonChartTheme = "";
    this->textEditorOpenFolder = "";
    this->textEditorGotoLine = "";
    this->paletteEditorBitDepth = 24;
    this->projectSettingsTab = 0;
    this->scriptAutocompleteMode = ScriptAutocompleteMode::MapOnly;
    this->warpBehaviorWarningDisabled = false;
    this->eventDeleteWarningDisabled = false;
    this->eventOverlayEnabled = false;
    this->checkForUpdates = true;
    this->lastUpdateCheckTime = QDateTime();
    this->lastUpdateCheckVersion = porymapVersion;
    this->rateLimitTimes.clear();
    this->eventSelectionShapeMode = QGraphicsPixmapItem::MaskShape;
    this->shownInGameReloadMessage = false;
    this->gridSettings = GridSettings();
    this->gridSettings.width = Metatile::pixelWidth();
    this->gridSettings.height = Metatile::pixelHeight();
    this->statusBarLogTypes = { LogType::LOG_ERROR, LogType::LOG_WARN };
    this->applicationFont = QFont();
    this->mapListFont = PorymapConfig::defaultMapListFont();
#ifdef Q_OS_MACOS
    // Since the release of the Retina display, Apple products use the Display P3 color space by default.
    // If we don't use this for exported images (which by default will either have no color space or the sRGB
    // color space) then they may appear to have different colors than the same image displayed in Porymap.
    this->imageExportColorSpaceId = static_cast<int>(QColorSpace::DisplayP3);
#else
    // As of writing Qt has no way to get a reasonable color space from the user's environment,
    // so we export images without one and let them handle it.
    this->imageExportColorSpaceId = 0;
#endif
}

void PorymapConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_project") {
        this->recentProjects = value.split(",", Qt::SkipEmptyParts);
        this->recentProjects.removeDuplicates();
    } else if (key == "project_manually_closed") {
        this->projectManuallyClosed = getConfigBool(key, value);
    } else if (key == "reopen_on_launch") {
        this->reopenOnLaunch = getConfigBool(key, value);
    } else if (key == "pretty_cursors") {
        this->prettyCursors = getConfigBool(key, value);
    } else if (key == "map_list_tab") {
        this->mapListTab = getConfigInteger(key, value, 0, 2, 0);
    } else if (key == "map_list_edit_groups_enabled") {
        this->mapListEditGroupsEnabled = getConfigBool(key, value);
    } else if (key.startsWith("map_list_hide_empty_enabled/")) {
        bool ok;
        int tab = key.mid(QStringLiteral("map_list_hide_empty_enabled/").length()).toInt(&ok, 0);
        if (!ok) {
            logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
            return;
        }
        this->mapListHideEmptyEnabled.insert(tab, getConfigBool(key, value));
    } else if (key == "main_window_geometry") {
        this->mainWindowGeometry = bytesFromString(value);
    } else if (key == "main_window_state") {
        this->mainWindowState = bytesFromString(value);
    } else if (key == "map_splitter_state") {
        this->mapSplitterState = bytesFromString(value);
    } else if (key == "main_splitter_state") {
        this->mainSplitterState = bytesFromString(value);
    } else if (key == "metatiles_splitter_state") {
        this->metatilesSplitterState = bytesFromString(value);
    } else if (key == "mirror_connecting_maps") {
        this->mirrorConnectingMaps = getConfigBool(key, value);
    } else if (key == "show_dive_emerge_maps") {
        this->showDiveEmergeMaps = getConfigBool(key, value);
    } else if (key == "dive_emerge_map_opacity") {
        this->diveEmergeMapOpacity = getConfigInteger(key, value, 10, 90, 30);
    } else if (key == "dive_map_opacity") {
        this->diveMapOpacity = getConfigInteger(key, value, 10, 90, 15);
    } else if (key == "emerge_map_opacity") {
        this->emergeMapOpacity = getConfigInteger(key, value, 10, 90, 15);
    } else if (key == "collision_opacity") {
        this->collisionOpacity = getConfigInteger(key, value, 0, 100, 50);
    } else if (key == "tileset_editor_geometry") {
        this->tilesetEditorGeometry = bytesFromString(value);
    } else if (key == "tileset_editor_state") {
        this->tilesetEditorState = bytesFromString(value);
    } else if (key == "tileset_editor_splitter_state") {
        this->tilesetEditorSplitterState = bytesFromString(value);
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
    } else if (key == "wild_mon_chart_geometry") {
        this->wildMonChartGeometry = bytesFromString(value);
    } else if (key == "new_map_dialog_geometry") {
        this->newMapDialogGeometry = bytesFromString(value);
    } else if (key == "new_layout_dialog_geometry") {
        this->newLayoutDialogGeometry = bytesFromString(value);
    } else if (key == "metatiles_zoom") {
        this->metatilesZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "collision_zoom") {
        this->collisionZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "tileset_editor_metatiles_zoom") {
        this->tilesetEditorMetatilesZoom = getConfigInteger(key, value, 10, 100, 30);
    } else if (key == "tileset_editor_tiles_zoom") {
        this->tilesetEditorTilesZoom = getConfigInteger(key, value, 10, 100, 30);
     } else if (key == "tileset_editor_layer_orientation") {
        // Being explicit here to avoid casting out-of-range values.
        this->tilesetEditorLayerOrientation = (getConfigInteger(key, value) == static_cast<int>(Qt::Horizontal)) ? Qt::Horizontal : Qt::Vertical;
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
    } else if (key == "show_tileset_editor_divider") {
        this->showTilesetEditorDivider = getConfigBool(key, value);
    } else if (key == "show_tileset_editor_raw_attributes") {
        this->showTilesetEditorRawAttributes = getConfigBool(key, value);
    } else if (key == "show_palette_editor_unused_colors") {
        this->showPaletteEditorUnusedColors = getConfigBool(key, value);
    } else if (key == "monitor_files") {
        this->monitorFiles = getConfigBool(key, value);
    } else if (key == "tileset_checkerboard_fill") {
        this->tilesetCheckerboardFill = getConfigBool(key, value);
    } else if (key == "new_map_header_section_expanded") {
        this->newMapHeaderSectionExpanded = getConfigBool(key, value);
    } else if (key == "theme") {
        this->theme = value;
    } else if (key == "wild_mon_chart_theme") {
        this->wildMonChartTheme = value;
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
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    // Old setting replaced by script_autocomplete_mode
    } else if (key == "load_all_event_scripts") {
        this->scriptAutocompleteMode = getConfigBool(key, value) ? ScriptAutocompleteMode::All : ScriptAutocompleteMode::MapOnly;
#endif
    } else if (key == "script_autocomplete_mode") {
        this->scriptAutocompleteMode = static_cast<ScriptAutocompleteMode>(getConfigInteger(key, value, ScriptAutocompleteMode::MapOnly, ScriptAutocompleteMode::All));
    } else if (key == "warp_behavior_warning_disabled") {
        this->warpBehaviorWarningDisabled = getConfigBool(key, value);
    } else if (key == "event_delete_warning_disabled") {
        this->eventDeleteWarningDisabled = getConfigBool(key, value);
    } else if (key == "event_overlay_enabled") {
        this->eventOverlayEnabled = getConfigBool(key, value);
    } else if (key == "check_for_updates") {
        this->checkForUpdates = getConfigBool(key, value);
    } else if (key == "last_update_check_time") {
        this->lastUpdateCheckTime = QDateTime::fromString(value).toLocalTime();
    } else if (key == "last_update_check_version") {
        auto version = QVersionNumber::fromString(value);
        if (version.segmentCount() != 3) {
            logWarn(QString("Invalid config value for %1: '%2'. Must be 3 numbers separated by '.'").arg(key).arg(value));
            this->lastUpdateCheckVersion = porymapVersion;
        } else {
            this->lastUpdateCheckVersion = version;
        }
    } else if (key.startsWith("rate_limit_time/")) {
        static const QRegularExpression regex("\\brate_limit_time/(?<url>.+)");
        QRegularExpressionMatch match = regex.match(key);
        if (match.hasMatch()) {
            this->rateLimitTimes.insert(match.captured("url"), QDateTime::fromString(value).toLocalTime());
        }
    } else if (key == "event_selection_shape_mode") {
        if (value == "mask") {
            this->eventSelectionShapeMode = QGraphicsPixmapItem::MaskShape;
        } else if (value == "bounding_rect") {
            this->eventSelectionShapeMode = QGraphicsPixmapItem::BoundingRectShape;
        } else {
            logWarn(QString("Invalid config value for %1: '%2'. Must be 'mask' or 'bounding_rect'.").arg(key).arg(value));
        }
    } else if (key == "shown_in_game_reload_message") {
        this->shownInGameReloadMessage = getConfigBool(key, value);
    } else if (key == "grid_width") {
        this->gridSettings.width = getConfigUint32(key, value);
    } else if (key == "grid_height") {
        this->gridSettings.height = getConfigUint32(key, value);
    } else if (key == "grid_x") {
        this->gridSettings.offsetX = getConfigInteger(key, value, 0, 999);
    } else if (key == "grid_y") {
        this->gridSettings.offsetY = getConfigInteger(key, value, 0, 999);
    } else if (key == "grid_style") {
        this->gridSettings.style = GridSettings::getStyleFromName(value);
    } else if (key == "grid_color") {
        this->gridSettings.color = getConfigColor(key, value);
    } else if (key == "status_bar_log_types") {
        this->statusBarLogTypes.clear();
        auto typeStrings = value.split(",", Qt::SkipEmptyParts);
        for (const auto &typeString : typeStrings) {
            LogType type = static_cast<LogType>(getConfigInteger(key, typeString, 0, 2));
            this->statusBarLogTypes.insert(type);
        }
    } else if (key == "application_font") {
        this->applicationFont = QFont();
        this->applicationFont.fromString(value);
    } else if (key == "map_list_font") {
        this->mapListFont = QFont();
        this->mapListFont.fromString(value);
    } else if (key == "image_export_color_space_id") {
        this->imageExportColorSpaceId = getConfigInteger(key, value, 0, 8);
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProjects.join(","));
    map.insert("project_manually_closed", this->projectManuallyClosed ? "1" : "0");
    map.insert("reopen_on_launch", this->reopenOnLaunch ? "1" : "0");
    map.insert("pretty_cursors", this->prettyCursors ? "1" : "0");
    map.insert("map_list_tab", QString::number(this->mapListTab));
    map.insert("map_list_edit_groups_enabled", this->mapListEditGroupsEnabled ? "1" : "0");
    for (auto i = this->mapListHideEmptyEnabled.constBegin(); i != this->mapListHideEmptyEnabled.constEnd(); i++) {
        map.insert(QStringLiteral("map_list_hide_empty_enabled/") + QString::number(i.key()), i.value() ? "1" : "0");
    }
    map.insert("main_window_geometry", stringFromByteArray(this->mainWindowGeometry));
    map.insert("main_window_state", stringFromByteArray(this->mainWindowState));
    map.insert("map_splitter_state", stringFromByteArray(this->mapSplitterState));
    map.insert("main_splitter_state", stringFromByteArray(this->mainSplitterState));
    map.insert("metatiles_splitter_state", stringFromByteArray(this->metatilesSplitterState));
    map.insert("tileset_editor_geometry", stringFromByteArray(this->tilesetEditorGeometry));
    map.insert("tileset_editor_state", stringFromByteArray(this->tilesetEditorState));
    map.insert("tileset_editor_splitter_state", stringFromByteArray(this->tilesetEditorSplitterState));
    map.insert("palette_editor_geometry", stringFromByteArray(this->paletteEditorGeometry));
    map.insert("palette_editor_state", stringFromByteArray(this->paletteEditorState));
    map.insert("region_map_editor_geometry", stringFromByteArray(this->regionMapEditorGeometry));
    map.insert("region_map_editor_state", stringFromByteArray(this->regionMapEditorState));
    map.insert("project_settings_editor_geometry", stringFromByteArray(this->projectSettingsEditorGeometry));
    map.insert("project_settings_editor_state", stringFromByteArray(this->projectSettingsEditorState));
    map.insert("custom_scripts_editor_geometry", stringFromByteArray(this->customScriptsEditorGeometry));
    map.insert("custom_scripts_editor_state", stringFromByteArray(this->customScriptsEditorState));
    map.insert("wild_mon_chart_geometry", stringFromByteArray(this->wildMonChartGeometry));
    map.insert("new_map_dialog_geometry", stringFromByteArray(this->newMapDialogGeometry));
    map.insert("new_layout_dialog_geometry", stringFromByteArray(this->newLayoutDialogGeometry));
    map.insert("mirror_connecting_maps", this->mirrorConnectingMaps ? "1" : "0");
    map.insert("show_dive_emerge_maps", this->showDiveEmergeMaps ? "1" : "0");
    map.insert("dive_emerge_map_opacity", QString::number(this->diveEmergeMapOpacity));
    map.insert("dive_map_opacity", QString::number(this->diveMapOpacity));
    map.insert("emerge_map_opacity", QString::number(this->emergeMapOpacity));
    map.insert("collision_opacity", QString::number(this->collisionOpacity));
    map.insert("collision_zoom", QString::number(this->collisionZoom));
    map.insert("metatiles_zoom", QString::number(this->metatilesZoom));
    map.insert("tileset_editor_metatiles_zoom", QString::number(this->tilesetEditorMetatilesZoom));
    map.insert("tileset_editor_tiles_zoom", QString::number(this->tilesetEditorTilesZoom));
    map.insert("tileset_editor_layer_orientation", QString::number(this->tilesetEditorLayerOrientation));
    map.insert("show_player_view", this->showPlayerView ? "1" : "0");
    map.insert("show_cursor_tile", this->showCursorTile ? "1" : "0");
    map.insert("show_border", this->showBorder ? "1" : "0");
    map.insert("show_grid", this->showGrid ? "1" : "0");
    map.insert("show_tileset_editor_metatile_grid", this->showTilesetEditorMetatileGrid ? "1" : "0");
    map.insert("show_tileset_editor_layer_grid", this->showTilesetEditorLayerGrid ? "1" : "0");
    map.insert("show_tileset_editor_divider", this->showTilesetEditorDivider ? "1" : "0");
    map.insert("show_tileset_editor_raw_attributes", this->showTilesetEditorRawAttributes ? "1" : "0");
    map.insert("show_palette_editor_unused_colors", this->showPaletteEditorUnusedColors ? "1" : "0");
    map.insert("monitor_files", this->monitorFiles ? "1" : "0");
    map.insert("tileset_checkerboard_fill", this->tilesetCheckerboardFill ? "1" : "0");
    map.insert("new_map_header_section_expanded", this->newMapHeaderSectionExpanded ? "1" : "0");
    map.insert("theme", this->theme);
    map.insert("wild_mon_chart_theme", this->wildMonChartTheme);
    map.insert("text_editor_open_directory", this->textEditorOpenFolder);
    map.insert("text_editor_goto_line", this->textEditorGotoLine);
    map.insert("palette_editor_bit_depth", QString::number(this->paletteEditorBitDepth));
    map.insert("project_settings_tab", QString::number(this->projectSettingsTab));
    map.insert("script_autocomplete_mode", QString::number(this->scriptAutocompleteMode));
    map.insert("warp_behavior_warning_disabled", QString::number(this->warpBehaviorWarningDisabled));
    map.insert("event_delete_warning_disabled", QString::number(this->eventDeleteWarningDisabled));
    map.insert("event_overlay_enabled", QString::number(this->eventOverlayEnabled));
    map.insert("check_for_updates", QString::number(this->checkForUpdates));
    map.insert("last_update_check_time", this->lastUpdateCheckTime.toUTC().toString());
    map.insert("last_update_check_version", this->lastUpdateCheckVersion.toString());
    for (auto i = this->rateLimitTimes.cbegin(), end = this->rateLimitTimes.cend(); i != end; i++){
        // Only include rate limit times that are still active (i.e., in the future)
        const QDateTime time = i.value();
        if (!time.isNull() && time > QDateTime::currentDateTime())
            map.insert("rate_limit_time/" + i.key().toString(), time.toUTC().toString());
    }
    map.insert("event_selection_shape_mode", (this->eventSelectionShapeMode == QGraphicsPixmapItem::MaskShape) ? "mask" : "bounding_rect");
    map.insert("shown_in_game_reload_message", this->shownInGameReloadMessage ? "1" : "0");
    map.insert("grid_width", QString::number(this->gridSettings.width));
    map.insert("grid_height", QString::number(this->gridSettings.height));
    map.insert("grid_x", QString::number(this->gridSettings.offsetX));
    map.insert("grid_y", QString::number(this->gridSettings.offsetY));
    map.insert("grid_style", GridSettings::getStyleName(this->gridSettings.style));
    map.insert("grid_color", toConfigColor(this->gridSettings.color));

    QStringList logTypesStrings;
    for (const auto &type : this->statusBarLogTypes) {
        logTypesStrings.append(QString::number(type));
    }
    map.insert("status_bar_log_types", logTypesStrings.join(","));
    map.insert("application_font", this->applicationFont.toString());
    map.insert("map_list_font", this->mapListFont.toString());
    map.insert("image_export_color_space_id", QString::number(this->imageExportColorSpaceId));
    
    return map;
}

QString PorymapConfig::stringFromByteArray(const QByteArray &bytearray) {
    QString ret;
    for (const auto &ch : bytearray) {
        ret += QString::number(static_cast<int>(ch)) + ":";
    }
    return ret;
}

QByteArray PorymapConfig::bytesFromString(const QString &in) {
    QByteArray ba;
    QStringList split = in.split(":", Qt::SkipEmptyParts);
    for (const auto &ch : split) {
        ba.append(static_cast<char>(ch.toInt()));
    }
    return ba;
}

void PorymapConfig::addRecentProject(QString project) {
    this->recentProjects.removeOne(project);
    this->recentProjects.prepend(project);
}

void PorymapConfig::setRecentProjects(QStringList projects) {
    this->recentProjects = projects;
}

QString PorymapConfig::getRecentProject() {
    return this->recentProjects.value(0);
}

QStringList PorymapConfig::getRecentProjects() {
    return this->recentProjects;
}

void PorymapConfig::setMainGeometry(QByteArray mainWindowGeometry_, QByteArray mainWindowState_,
                                QByteArray mapSplitterState_, QByteArray mainSplitterState_, QByteArray metatilesSplitterState_) {
    this->mainWindowGeometry = mainWindowGeometry_;
    this->mainWindowState = mainWindowState_;
    this->mapSplitterState = mapSplitterState_;
    this->mainSplitterState = mainSplitterState_;
    this->metatilesSplitterState = metatilesSplitterState_;
}

void PorymapConfig::setTilesetEditorGeometry(QByteArray tilesetEditorGeometry_, QByteArray tilesetEditorState_,
                                            QByteArray tilesetEditorSplitterState_) {
    this->tilesetEditorGeometry = tilesetEditorGeometry_;
    this->tilesetEditorState = tilesetEditorState_;
    this->tilesetEditorSplitterState = tilesetEditorSplitterState_;
}

void PorymapConfig::setPaletteEditorGeometry(QByteArray paletteEditorGeometry_, QByteArray paletteEditorState_) {
    this->paletteEditorGeometry = paletteEditorGeometry_;
    this->paletteEditorState = paletteEditorState_;
}

void PorymapConfig::setRegionMapEditorGeometry(QByteArray regionMapEditorGeometry_, QByteArray regionMapEditorState_) {
    this->regionMapEditorGeometry = regionMapEditorGeometry_;
    this->regionMapEditorState = regionMapEditorState_;
}

void PorymapConfig::setProjectSettingsEditorGeometry(QByteArray projectSettingsEditorGeometry_, QByteArray projectSettingsEditorState_) {
    this->projectSettingsEditorGeometry = projectSettingsEditorGeometry_;
    this->projectSettingsEditorState = projectSettingsEditorState_;
}

void PorymapConfig::setCustomScriptsEditorGeometry(QByteArray customScriptsEditorGeometry_, QByteArray customScriptsEditorState_) {
    this->customScriptsEditorGeometry = customScriptsEditorGeometry_;
    this->customScriptsEditorState = customScriptsEditorState_;
}

QMap<QString, QByteArray> PorymapConfig::getMainGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("main_window_geometry", this->mainWindowGeometry);
    geometry.insert("main_window_state", this->mainWindowState);
    geometry.insert("map_splitter_state", this->mapSplitterState);
    geometry.insert("main_splitter_state", this->mainSplitterState);
    geometry.insert("metatiles_splitter_state", this->metatilesSplitterState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getTilesetEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("tileset_editor_geometry", this->tilesetEditorGeometry);
    geometry.insert("tileset_editor_state", this->tilesetEditorState);
    geometry.insert("tileset_editor_splitter_state", this->tilesetEditorSplitterState);

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

const QMap<BaseGameVersion, QStringList> versionDetectNames = {
    {BaseGameVersion::pokeruby, {"ruby", "sapphire"}},
    {BaseGameVersion::pokefirered, {"firered", "leafgreen"}},
    {BaseGameVersion::pokeemerald, {"emerald"}},
};

// If a string exclusively contains one version name we assume its identity,
// otherwise we leave it unknown and we'll need the user to tell us the version.
BaseGameVersion ProjectConfig::stringToBaseGameVersion(const QString &string) {
    BaseGameVersion version = BaseGameVersion::none;
    for (auto i = versionDetectNames.cbegin(), end = versionDetectNames.cend(); i != end; i++) {
        // Compare the given string to all the possible names for this game version
        const QStringList names = i.value();
        for (auto name : names) {
            if (string.contains(name)) {
                if (version != BaseGameVersion::none) {
                    // The given string matches multiple versions, so we can't be sure which it is.
                    return BaseGameVersion::none;
                }
                version = i.key();
                break;
            }
        }
    }
    // We finished checking the names for each version; the name either matched 1 version or none.
    return version;
}

QString ProjectConfig::getPlayerIconPath(BaseGameVersion baseGameVersion, int character) {
    switch (baseGameVersion) {
        case BaseGameVersion::pokeemerald: {
            static const QStringList paths = { QStringLiteral(":/icons/player/brendan_em.ico"),
                                               QStringLiteral(":/icons/player/may_em.ico"), };
            return paths.value(character);
        }
        case BaseGameVersion::pokefirered: {
            static const QStringList paths = { QStringLiteral(":/icons/player/red.ico"),
                                               QStringLiteral(":/icons/player/green.ico"), };
            return paths.value(character);
        }
        case BaseGameVersion::pokeruby:    {
            static const QStringList paths = { QStringLiteral(":/icons/player/brendan_rs.ico"),
                                               QStringLiteral(":/icons/player/may_rs.ico"), };
            return paths.value(character);
        }
        default: break;
    }
    return QString();
}

QIcon ProjectConfig::getPlayerIcon(BaseGameVersion baseGameVersion, int character) {
    return QIcon(getPlayerIconPath(baseGameVersion, character));
}

ProjectConfig projectConfig;

ProjectConfig::ProjectConfig() : KeyValueConfigBase(QStringLiteral("porymap.project.cfg")) {
    reset();
}

void ProjectConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "base_game_version") {
        this->baseGameVersion = this->stringToBaseGameVersion(value.toLower());
        if (this->baseGameVersion == BaseGameVersion::none) {
            logWarn(QString("Invalid config value for base_game_version: '%1'. Must be 'pokeruby', 'pokefirered' or 'pokeemerald'.").arg(value));
            this->baseGameVersion = BaseGameVersion::pokeemerald;
        }
    } else if (key == "use_poryscript") {
        this->usePoryScript = getConfigBool(key, value);
    } else if (key == "use_custom_border_size") {
        this->useCustomBorderSize = getConfigBool(key, value);
    } else if (key == "enable_event_weather_trigger") {
        this->eventWeatherTriggerEnabled = getConfigBool(key, value);
    } else if (key == "enable_event_secret_base") {
        this->eventSecretBaseEnabled = getConfigBool(key, value);
    } else if (key == "enable_hidden_item_quantity") {
        this->hiddenItemQuantityEnabled = getConfigBool(key, value);
    } else if (key == "enable_hidden_item_requires_itemfinder") {
        this->hiddenItemRequiresItemfinderEnabled = getConfigBool(key, value);
    } else if (key == "enable_heal_location_respawn_data") {
        this->healLocationRespawnDataEnabled = getConfigBool(key, value);
    } else if (key == "enable_event_clone_object" || key == "enable_object_event_in_connection") {
        this->eventCloneObjectEnabled = getConfigBool(key, value);
        key = "enable_event_clone_object"; // Backwards compatibiliy, replace old name above
    } else if (key == "enable_floor_number") {
        this->floorNumberEnabled = getConfigBool(key, value);
    } else if (key == "create_map_text_file") {
        this->createMapTextFileEnabled = getConfigBool(key, value);
    } else if (key == "enable_triple_layer_metatiles") {
        this->tripleLayerMetatilesEnabled = getConfigBool(key, value);
    } else if (key == "default_metatile") {
        this->defaultMetatileId = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "default_elevation") {
        this->defaultElevation = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "default_collision") {
        this->defaultCollision = getConfigUint32(key, value, 0, Block::maxValue);
    } else if (key == "default_map_width") {
        this->defaultMapSize.setWidth(getConfigInteger(key, value, 1));
    } else if (key == "default_map_height") {
        this->defaultMapSize.setHeight(getConfigInteger(key, value, 1));
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
    } else if (key == "unused_tile_normal") {
        this->unusedTileNormal = getConfigUint32(key, value, 0, Tile::maxValue);
    } else if (key == "unused_tile_covered") {
        this->unusedTileCovered = getConfigUint32(key, value, 0, Tile::maxValue);
    } else if (key == "unused_tile_split") {
        this->unusedTileSplit = getConfigUint32(key, value, 0, Tile::maxValue);
    } else if (key == "enable_map_allow_flags") {
        this->mapAllowFlagsEnabled = getConfigBool(key, value);
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    } else if (key == "recent_map_or_layout") {
        userConfig.recentMapOrLayout = value;
    } else if (key == "use_encounter_json") {
        userConfig.useEncounterJson = getConfigBool(key, value);
    } else if (key == "custom_scripts") {
        userConfig.parseCustomScripts(value);
#endif
    } else if (key.startsWith("path/")) {
        auto k = reverseDefaultPaths(key.mid(QStringLiteral("path/").length()));
        if (k != static_cast<ProjectFilePath>(-1)) {
            this->setFilePath(k, value);
        } else {
            logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
        }
    } else if (key.startsWith("ident/")) {
        auto identifierId = reverseDefaultIdentifier(key.mid(QStringLiteral("ident/").length()));
        if (identifierId != static_cast<ProjectIdentifier>(-1)) {
            this->setIdentifier(identifierId, value);
        } else {
            logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
        }
    } else if (key.startsWith("global_constant/")) {
        this->globalConstants.insert(key.mid(QStringLiteral("global_constant/").length()), value);
    } else if (key == "global_constants_filepaths") {
        this->globalConstantsFilepaths = value.split(",", Qt::SkipEmptyParts);
    } else if (key == "prefabs_filepath") {
        this->prefabFilepath = value;
    } else if (key == "prefabs_import_prompted") {
        this->prefabImportPrompted = getConfigBool(key, value);
    } else if (key == "tilesets_have_callback") {
        this->tilesetsHaveCallback = getConfigBool(key, value);
    } else if (key == "tilesets_have_is_compressed") {
        this->tilesetsHaveIsCompressed = getConfigBool(key, value);
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    // Old setting replaced by transparency_color
    } else if (key == "set_transparent_pixels_black") {
        this->transparencyColor = getConfigBool(key, value) ? QColor(Qt::black) : QColor();
#endif
    } else if (key == "transparency_color") {
        this->transparencyColor = getConfigColor(key, value);
    } else if (key == "preserve_matching_only_data") {
        this->preserveMatchingOnlyData = getConfigBool(key, value);
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
        this->pokemonIconPaths.insert(key.mid(QStringLiteral("pokemon_icon_path/").length()), value);
    } else if (key == "events_tab_icon_path") {
        this->eventsTabIconPath = value;
    } else if (key == "collision_sheet_path") {
        this->collisionSheetPath = value;
    } else if (key == "collision_sheet_width") {
        this->collisionSheetSize.setWidth(getConfigInteger(key, value, 1, Block::maxValue));
    } else if (key == "collision_sheet_height") {
        this->collisionSheetSize.setHeight(getConfigInteger(key, value, 1, Block::maxValue));
    } else if (key == "player_view_north") {
        this->playerViewDistance.setTop(getConfigInteger(key, value, 0, INT_MAX, GBA_V_DIST_TO_CENTER));
    } else if (key == "player_view_south") {
        this->playerViewDistance.setBottom(getConfigInteger(key, value, 0, INT_MAX, GBA_V_DIST_TO_CENTER));
    } else if (key == "player_view_west") {
        this->playerViewDistance.setLeft(getConfigInteger(key, value, 0, INT_MAX, GBA_H_DIST_TO_CENTER));
    } else if (key == "player_view_east") {
        this->playerViewDistance.setRight(getConfigInteger(key, value, 0, INT_MAX, GBA_H_DIST_TO_CENTER));
    } else if (key == "warp_behaviors") {
        this->warpBehaviors.clear();
        value.remove(" ");
        const QStringList behaviorList = value.split(",", Qt::SkipEmptyParts);
        for (auto s : behaviorList)
            this->warpBehaviors.append(getConfigUint32(key, s));
    } else if (key == "max_events_per_group") {
        this->maxEventsPerGroup = getConfigInteger(key, value, 1, INT_MAX, 255);
    } else if (key == "forced_major_version") {
        this->forcedMajorVersion = getConfigInteger(key, value);
    } else if (key == "metatile_selector_width") {
        this->metatileSelectorWidth = getConfigInteger(key, value, 1, INT_MAX, 8);
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
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
    if (!readKeys.contains("enable_event_weather_trigger")) this->eventWeatherTriggerEnabled = !isPokefirered;
    if (!readKeys.contains("enable_event_secret_base")) this->eventSecretBaseEnabled = !isPokefirered;
    if (!readKeys.contains("enable_hidden_item_quantity")) this->hiddenItemQuantityEnabled = isPokefirered;
    if (!readKeys.contains("enable_hidden_item_requires_itemfinder")) this->hiddenItemRequiresItemfinderEnabled = isPokefirered;
    if (!readKeys.contains("enable_heal_location_respawn_data")) this->healLocationRespawnDataEnabled = isPokefirered;
    if (!readKeys.contains("enable_event_clone_object")) this->eventCloneObjectEnabled = isPokefirered;
    if (!readKeys.contains("enable_floor_number")) this->floorNumberEnabled = isPokefirered;
    if (!readKeys.contains("create_map_text_file")) this->createMapTextFileEnabled = (this->baseGameVersion != BaseGameVersion::pokeemerald);
    if (!readKeys.contains("new_map_border_metatiles")) this->newMapBorderMetatileIds = isPokefirered ? defaultBorder_FRLG : defaultBorder_RSE;
    if (!readKeys.contains("default_secondary_tileset")) this->defaultSecondaryTileset = isPokefirered ? "gTileset_PalletTown" : "gTileset_Petalburg";
    if (!readKeys.contains("metatile_attributes_size")) this->metatileAttributesSize = Metatile::getDefaultAttributesSize(this->baseGameVersion);
    if (!readKeys.contains("metatile_behavior_mask")) this->metatileBehaviorMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::Behavior);
    if (!readKeys.contains("metatile_terrain_type_mask")) this->metatileTerrainTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::TerrainType);
    if (!readKeys.contains("metatile_encounter_type_mask")) this->metatileEncounterTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::EncounterType);
    if (!readKeys.contains("metatile_layer_type_mask")) this->metatileLayerTypeMask = Metatile::getDefaultAttributesMask(this->baseGameVersion, Metatile::Attr::LayerType);
    if (!readKeys.contains("enable_map_allow_flags")) this->mapAllowFlagsEnabled = (this->baseGameVersion != BaseGameVersion::pokeruby);
    if (!readKeys.contains("warp_behaviors")) this->warpBehaviors = isPokefirered ? defaultWarpBehaviors_FRLG : defaultWarpBehaviors_RSE;
}

QMap<QString, QString> ProjectConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("base_game_version", baseGameVersionMap.value(this->baseGameVersion));
    map.insert("use_poryscript", QString::number(this->usePoryScript));
    map.insert("use_custom_border_size", QString::number(this->useCustomBorderSize));
    map.insert("enable_event_weather_trigger", QString::number(this->eventWeatherTriggerEnabled));
    map.insert("enable_event_secret_base", QString::number(this->eventSecretBaseEnabled));
    map.insert("enable_hidden_item_quantity", QString::number(this->hiddenItemQuantityEnabled));
    map.insert("enable_hidden_item_requires_itemfinder", QString::number(this->hiddenItemRequiresItemfinderEnabled));
    map.insert("enable_heal_location_respawn_data", QString::number(this->healLocationRespawnDataEnabled));
    map.insert("enable_event_clone_object", QString::number(this->eventCloneObjectEnabled));
    map.insert("enable_floor_number", QString::number(this->floorNumberEnabled));
    map.insert("create_map_text_file", QString::number(this->createMapTextFileEnabled));
    map.insert("enable_triple_layer_metatiles", QString::number(this->tripleLayerMetatilesEnabled));
    map.insert("default_metatile", Metatile::getMetatileIdString(this->defaultMetatileId));
    map.insert("default_elevation", QString::number(this->defaultElevation));
    map.insert("default_collision", QString::number(this->defaultCollision));
    map.insert("default_map_width", QString::number(this->defaultMapSize.width()));
    map.insert("default_map_height", QString::number(this->defaultMapSize.height()));
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
    map.insert("transparency_color", toConfigColor(this->transparencyColor));
    map.insert("preserve_matching_only_data", QString::number(this->preserveMatchingOnlyData));
    map.insert("metatile_attributes_size", QString::number(this->metatileAttributesSize));
    map.insert("metatile_behavior_mask", Util::toHexString(this->metatileBehaviorMask));
    map.insert("metatile_terrain_type_mask", Util::toHexString(this->metatileTerrainTypeMask));
    map.insert("metatile_encounter_type_mask", Util::toHexString(this->metatileEncounterTypeMask));
    map.insert("metatile_layer_type_mask", Util::toHexString(this->metatileLayerTypeMask));
    map.insert("block_metatile_id_mask", Util::toHexString(this->blockMetatileIdMask));
    map.insert("block_collision_mask", Util::toHexString(this->blockCollisionMask));
    map.insert("block_elevation_mask", Util::toHexString(this->blockElevationMask));
    map.insert("unused_tile_normal", Util::toHexString(this->unusedTileNormal));
    map.insert("unused_tile_covered", Util::toHexString(this->unusedTileCovered));
    map.insert("unused_tile_split", Util::toHexString(this->unusedTileSplit));
    map.insert("enable_map_allow_flags", QString::number(this->mapAllowFlagsEnabled));
    map.insert("event_icon_path_object", this->eventIconPaths[Event::Group::Object]);
    map.insert("event_icon_path_warp", this->eventIconPaths[Event::Group::Warp]);
    map.insert("event_icon_path_coord", this->eventIconPaths[Event::Group::Coord]);
    map.insert("event_icon_path_bg", this->eventIconPaths[Event::Group::Bg]);
    map.insert("event_icon_path_heal", this->eventIconPaths[Event::Group::Heal]);
    for (auto it = this->pokemonIconPaths.constBegin(); it != this->pokemonIconPaths.constEnd(); it++) {
        const QString path = it.value();
        if (!path.isEmpty()) map.insert("pokemon_icon_path/" + it.key(), path);
    }
    for (auto it = this->globalConstants.constBegin(); it != this->globalConstants.constEnd(); it++) {
        map.insert("global_constant/" + it.key(), it.value());
    }
    map.insert("global_constants_filepaths", this->globalConstantsFilepaths.join(","));
    for (auto it = this->identifiers.constBegin(); it != this->identifiers.constEnd(); it++) {
        map.insert("ident/"+defaultIdentifiers.value(it.key()).first, it.value());
    }
    map.insert("events_tab_icon_path", this->eventsTabIconPath);
    map.insert("collision_sheet_path", this->collisionSheetPath);
    map.insert("collision_sheet_width", QString::number(this->collisionSheetSize.width()));
    map.insert("collision_sheet_height", QString::number(this->collisionSheetSize.height()));
    map.insert("player_view_north", QString::number(this->playerViewDistance.top()));
    map.insert("player_view_south", QString::number(this->playerViewDistance.bottom()));
    map.insert("player_view_west", QString::number(this->playerViewDistance.left()));
    map.insert("player_view_east", QString::number(this->playerViewDistance.right()));
    QStringList warpBehaviorStrs;
    for (const auto &value : this->warpBehaviors)
        warpBehaviorStrs.append("0x" + QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
    map.insert("warp_behaviors", warpBehaviorStrs.join(","));
    map.insert("max_events_per_group", QString::number(this->maxEventsPerGroup));
    map.insert("forced_major_version", QString::number(this->forcedMajorVersion));
    map.insert("metatile_selector_width", QString::number(this->metatileSelectorWidth));

    return map;
}

void ProjectConfig::init() {
    QString dirName = QDir(this->projectDir()).dirName().toLower();

    BaseGameVersion version = stringToBaseGameVersion(dirName);
    if (version != BaseGameVersion::none) {
        this->baseGameVersion = version;
        logInfo(QString("Auto-detected base_game_version as '%1'").arg(getBaseGameVersionString(version)));
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

        // TODO: Add an 'Advanced' button to open the project settings window (with some settings disabled)

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        form.addRow(&buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            this->baseGameVersion = static_cast<BaseGameVersion>(baseGameVersionComboBox->currentData().toInt());
        } else {
            logWarn(QString("No base_game_version selected, using default '%1'").arg(getBaseGameVersionString(this->baseGameVersion)));
        }
    }
    this->setUnreadKeys(); // Initialize version-specific options
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
    return QDir::cleanPath(this->filePaths.value(pathId));
}

QString ProjectConfig::getCustomFilePath(const QString &pathId) {
    return this->getCustomFilePath(reverseDefaultPaths(pathId));
}

QString ProjectConfig::getFilePath(ProjectFilePath pathId) {
    QString customPath = this->getCustomFilePath(pathId);
    if (!customPath.isEmpty()) {
        // A custom filepath has been specified. If the file/folder exists, use that.
        const QString baseDir = this->projectDir() + "/";
        if (customPath.startsWith(baseDir)) {
            customPath.remove(0, baseDir.length());
        }
        if (QFileInfo::exists(QDir::cleanPath(baseDir + customPath))) {
            return customPath;
        } else {
            logError(QString("Custom project filepath '%1' not found. Using default.").arg(customPath));
        }
    }
    return defaultPaths.contains(pathId) ? defaultPaths[pathId].second : QString();

}

void ProjectConfig::setIdentifier(ProjectIdentifier id, QString text) {
    if (!defaultIdentifiers.contains(id))
        return;

    if (text.isEmpty()) {
        this->identifiers.remove(id);
    } else {
        const QString idName = defaultIdentifiers.value(id).first;
        if (idName.startsWith("define_") || idName.startsWith("symbol_")) {
            // Validate the input for the identifier, depending on the type.
            IdentifierValidator validator;
            if (!validator.isValid(text)) {
                logError(QString("The name '%1' for project identifier '%2' is invalid. It must only contain word characters, and cannot start with a digit.").arg(text).arg(idName));
                return;
            }
        }
        this->identifiers[id] = text;
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

QString ProjectConfig::getBaseGameVersionString(BaseGameVersion version) {
    if (!baseGameVersionMap.contains(version)) {
        version = BaseGameVersion::pokeemerald;
    }
    return baseGameVersionMap.value(version);
}

QString ProjectConfig::getBaseGameVersionString() {
    return this->getBaseGameVersionString(this->baseGameVersion);
}

int ProjectConfig::getNumLayersInMetatile() {
    return this->tripleLayerMetatilesEnabled ? 3 : 2;
}

int ProjectConfig::getNumTilesInMetatile() {
    return getNumLayersInMetatile() * Metatile::tilesPerLayer();
}

void ProjectConfig::setEventIconPath(Event::Group group, const QString &path) {
    this->eventIconPaths[group] = path;
}

QString ProjectConfig::getEventIconPath(Event::Group group) {
    return this->eventIconPaths.value(group);
}

void ProjectConfig::setPokemonIconPath(const QString &species, const QString &path) {
    this->pokemonIconPaths[species] = path;
}

QString ProjectConfig::getPokemonIconPath(const QString &species) {
    return this->pokemonIconPaths.value(species);
}

QMap<QString, QString> ProjectConfig::getPokemonIconPaths() {
    return this->pokemonIconPaths;
}

UserConfig userConfig;

UserConfig::UserConfig() : KeyValueConfigBase(QStringLiteral("porymap.user.cfg")) {
    reset();
}

void UserConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_map_or_layout") {
        this->recentMapOrLayout = value;
    } else if (key == "use_encounter_json") {
        this->useEncounterJson = getConfigBool(key, value);
    } else if (key == "custom_scripts") {
        this->parseCustomScripts(value);
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->filepath()).arg(key));
    }
    readKeys.append(key);
}

void UserConfig::setUnreadKeys() {
}

QMap<QString, QString> UserConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_map_or_layout", this->recentMapOrLayout);
    map.insert("use_encounter_json", QString::number(this->useEncounterJson));
    map.insert("custom_scripts", this->outputCustomScripts());
    return map;
}

void UserConfig::init() {
    this->useEncounterJson = true;
    this->customScripts.clear();
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
}

QStringList UserConfig::getCustomScriptPaths() {
    return this->customScripts.keys();
}

QList<bool> UserConfig::getCustomScriptsEnabled() {
    return this->customScripts.values();
}

ShortcutsConfig shortcutsConfig;

ShortcutsConfig::ShortcutsConfig() : KeyValueConfigBase(QStringLiteral("porymap.shortcuts.cfg")),
    user_shortcuts({ }),
    default_shortcuts({ }) {
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
}

QList<QKeySequence> ShortcutsConfig::defaultShortcuts(const QObject *object) const {
    return default_shortcuts.values(cfgKey(object));
}

void ShortcutsConfig::setUserShortcuts(const QObjectList &objects) {
    storeShortcutsFromList(StoreType::User, objects);
}

void ShortcutsConfig::setUserShortcuts(const QMultiMap<const QObject *, QKeySequence> &objects_keySequences) {
    for (auto *object : objects_keySequences.uniqueKeys())
        if (!object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            storeShortcuts(StoreType::User, cfgKey(object), objects_keySequences.values(object));
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
