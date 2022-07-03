#include "project.h"
#include "config.h"
#include "history.h"
#include "log.h"
#include "parseutil.h"
#include "paletteutil.h"
#include "tile.h"
#include "tileset.h"
#include "imageexport.h"
#include "map.h"

#include "orderedjson.h"
#include "lib/fex/lexer.h"
#include "lib/fex/parser.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QTextStream>
#include <QStandardItem>
#include <QMessageBox>
#include <QRegularExpression>
#include <algorithm>

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;

int Project::num_tiles_primary = 512;
int Project::num_tiles_total = 1024;
int Project::num_metatiles_primary = 512;
int Project::num_metatiles_total = 1024;
int Project::num_pals_primary = 6;
int Project::num_pals_total = 13;
int Project::max_map_data_size = 10240; // 0x2800
int Project::default_map_size = 20;
int Project::max_object_events = 64;

Project::Project(QWidget *parent) :
    QObject(parent),
    eventScriptLabelModel(this),
    eventScriptLabelCompleter(this)
{
    initSignals();
}

Project::~Project()
{
    clearMapCache();
    clearTilesetCache();
}

void Project::initSignals() {
    // detect changes to specific filepaths being monitored
    QObject::connect(&fileWatcher, &QFileSystemWatcher::fileChanged, [this](QString changed){
        if (!porymapConfig.getMonitorFiles()) return;
        if (modifiedFileTimestamps.contains(changed)) {
            if (QDateTime::currentMSecsSinceEpoch() < modifiedFileTimestamps[changed]) {
                return;
            }
            modifiedFileTimestamps.remove(changed);
        }

        static bool showing = false;
        if (showing) return;

        QMessageBox notice(this->parentWidget());
        notice.setText("File Changed");
        notice.setInformativeText(QString("The file %1 has changed on disk. Would you like to reload the project?")
                                  .arg(changed.remove(this->root + "/")));
        notice.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        notice.setIcon(QMessageBox::Question);

        QCheckBox showAgainCheck("Do not ask again.");
        notice.setCheckBox(&showAgainCheck);

        showing = true;
        int choice = notice.exec();
        if (choice == QMessageBox::Yes) {
            emit reloadProject();
        } else if (choice == QMessageBox::No) {
            if (showAgainCheck.isChecked()) {
                porymapConfig.setMonitorFiles(false);
                emit uncheckMonitorFilesAction();
            }
        }
        showing = false;
    });
}

void Project::set_root(QString dir) {
    this->root = dir;
    this->parser.set_root(dir);
}

QString Project::getProjectTitle() {
    if (!root.isNull()) {
        return root.section('/', -1);
    } else {
        return QString();
    }
}

void Project::clearMapCache() {
    for (auto *map : mapCache.values()) {
        if (map)
            delete map;
    }
    mapCache.clear();
    emit mapCacheCleared();
}

void Project::clearTilesetCache() {
    for (auto *tileset : tilesetCache.values()) {
        if (tileset)
            delete tileset;
    }
    tilesetCache.clear();
}

Map* Project::loadMap(QString map_name) {
    Map *map;
    if (mapCache.contains(map_name)) {
        map = mapCache.value(map_name);
        // TODO: uncomment when undo/redo history is fully implemented for all actions.
        if (true/*map->hasUnsavedChanges()*/) {
            return map;
        }
    } else {
        map = new Map;
        map->setName(map_name);
    }

    if (!(loadMapData(map) && loadMapLayout(map)))
        return nullptr;

    mapCache.insert(map_name, map);
    return map;
}

void Project::setNewMapConnections(Map *map) {
    map->connections.clear();
}

static QMap<QString, bool> defaultTopLevelMapFields {
    {"id", true},
    {"name", true},
    {"layout", true},
    {"music", true},
    {"region_map_section", true},
    {"requires_flash", true},
    {"weather", true},
    {"map_type", true},
    {"show_map_name", true},
    {"battle_scene", true},
    {"connections", true},
    {"object_events", true},
    {"warp_events", true},
    {"coord_events", true},
    {"bg_events", true},
    {"shared_events_map", true},
    {"shared_scripts_map", true},
};

QMap<QString, bool> Project::getTopLevelMapFields() {
    QMap<QString, bool> topLevelMapFields = defaultTopLevelMapFields;
    if (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby) {
        topLevelMapFields.insert("allow_cycling", true);
        topLevelMapFields.insert("allow_escaping", true);
        topLevelMapFields.insert("allow_running", true);
    }

    if (projectConfig.getFloorNumberEnabled()) {
        topLevelMapFields.insert("floor_number", true);
    }
    return topLevelMapFields;
}

bool Project::loadMapData(Map* map) {
    if (!map->isPersistedToFile) {
        return true;
    }

    QString mapFilepath = QString("%1/data/maps/%2/map.json").arg(root).arg(map->name);
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map data from %1").arg(mapFilepath));
        return false;
    }

    QJsonObject mapObj = mapDoc.object();

    map->song = mapObj["music"].toString();
    map->layoutId = mapObj["layout"].toString();
    map->location = mapObj["region_map_section"].toString();
    map->requiresFlash = QString::number(mapObj["requires_flash"].toBool());
    map->weather = mapObj["weather"].toString();
    map->type = mapObj["map_type"].toString();
    map->requiresFlash = QString::number(mapObj["requires_flash"].toBool());
    map->show_location = QString::number(mapObj["show_map_name"].toBool());
    map->battle_scene = mapObj["battle_scene"].toString();
    if (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby) {
        map->allowBiking = QString::number(mapObj["allow_cycling"].toBool());
        map->allowEscapeRope = QString::number(mapObj["allow_escaping"].toBool());
        map->allowRunning = QString::number(mapObj["allow_running"].toBool());
    }
    if (projectConfig.getFloorNumberEnabled()) {
        map->floorNumber = mapObj["floor_number"].toInt();
    }
    map->sharedEventsMap = mapObj["shared_events_map"].toString();
    map->sharedScriptsMap = mapObj["shared_scripts_map"].toString();

    // Events
    map->events[EventGroup::Object].clear();
    QJsonArray objectEventsArr = mapObj["object_events"].toArray();
    bool hasCloneObjects = projectConfig.getEventCloneObjectEnabled();
    for (int i = 0; i < objectEventsArr.size(); i++) {
        QJsonObject event = objectEventsArr[i].toObject();
        // If clone objects are not enabled then no type field is present
        QString type = hasCloneObjects ? event["type"].toString() : "object";
        if (type.isEmpty() || type == "object") {
            Event *object = new Event(event, EventType::Object);
            object->put("map_name", map->name);
            object->put("sprite", event["graphics_id"].toString());
            object->put("x", QString::number(event["x"].toInt()));
            object->put("y", QString::number(event["y"].toInt()));
            object->put("elevation", QString::number(event["elevation"].toInt()));
            object->put("movement_type", event["movement_type"].toString());
            object->put("radius_x", QString::number(event["movement_range_x"].toInt()));
            object->put("radius_y", QString::number(event["movement_range_y"].toInt()));
            object->put("trainer_type", event["trainer_type"].toString());
            object->put("sight_radius_tree_id", event["trainer_sight_or_berry_tree_id"].toString());
            object->put("script_label", event["script"].toString());
            object->put("event_flag", event["flag"].toString());
            object->put("event_group_type", EventGroup::Object);
            map->events[EventGroup::Object].append(object);
        } else if (type == "clone") {
            Event *object = new Event(event, EventType::CloneObject);
            object->put("map_name", map->name);
            object->put("sprite", event["graphics_id"].toString());
            object->put("x", QString::number(event["x"].toInt()));
            object->put("y", QString::number(event["y"].toInt()));
            object->put("target_local_id", QString::number(event["target_local_id"].toInt()));

            // Ensure the target map constant is valid before adding it to the events.
            QString mapConstant = event["target_map"].toString();
            if (mapConstantsToMapNames.contains(mapConstant)) {
                object->put("target_map", mapConstantsToMapNames.value(mapConstant));
                object->put("event_group_type", EventGroup::Object);
                map->events[EventGroup::Object].append(object);
            } else if (mapConstant == NONE_MAP_CONSTANT) {
                object->put("target_map", NONE_MAP_NAME);
                object->put("event_group_type", EventGroup::Object);
                map->events[EventGroup::Object].append(object);
            } else {
                logError(QString("Destination map constant '%1' is invalid").arg(mapConstant));
            }
        } else {
            logError(QString("Map %1 object_event %2 has invalid type '%3'. Must be 'object' or 'clone'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events[EventGroup::Warp].clear();
    QJsonArray warpEventsArr = mapObj["warp_events"].toArray();
    for (int i = 0; i < warpEventsArr.size(); i++) {
        QJsonObject event = warpEventsArr[i].toObject();
        Event *warp = new Event(event, EventType::Warp);
        warp->put("map_name", map->name);
        warp->put("x", QString::number(event["x"].toInt()));
        warp->put("y", QString::number(event["y"].toInt()));
        warp->put("elevation", QString::number(event["elevation"].toInt()));
        warp->put("destination_warp", QString::number(event["dest_warp_id"].toInt()));

        // Ensure the warp destination map constant is valid before adding it to the warps.
        QString mapConstant = event["dest_map"].toString();
        if (mapConstantsToMapNames.contains(mapConstant)) {
            warp->put("destination_map_name", mapConstantsToMapNames.value(mapConstant));
            warp->put("event_group_type", EventGroup::Warp);
            map->events[EventGroup::Warp].append(warp);
        } else if (mapConstant == NONE_MAP_CONSTANT) {
            warp->put("destination_map_name", NONE_MAP_NAME);
            warp->put("event_group_type", EventGroup::Warp);
            map->events[EventGroup::Warp].append(warp);
        } else {
            logError(QString("Destination map constant '%1' is invalid for warp").arg(mapConstant));
        }
    }

    map->events[EventGroup::Heal].clear();
    for (auto it = healLocations.begin(); it != healLocations.end(); it++) {

        HealLocation loc = *it;

        //if TRUE map is flyable / has healing location
        if (loc.mapName == QString(mapNamesToMapConstants.value(map->name)).remove(0,4)) {
            Event *heal = new Event;
            heal->put("map_name", map->name);
            heal->put("x", loc.x);
            heal->put("y", loc.y);
            heal->put("loc_name", loc.mapName);
            heal->put("id_name", loc.idName);
            heal->put("index", loc.index);
            heal->put("elevation", 3); // TODO: change this?
            heal->put("destination_map_name", mapConstantsToMapNames.value(map->name));
            heal->put("event_group_type", EventGroup::Heal);
            heal->put("event_type", EventType::HealLocation);
            if (projectConfig.getHealLocationRespawnDataEnabled()) {
                heal->put("respawn_map", mapConstantsToMapNames.value(QString("MAP_" + loc.respawnMap)));
                heal->put("respawn_npc", loc.respawnNPC);
            }
            map->events[EventGroup::Heal].append(heal);
        }

    }

    map->events[EventGroup::Coord].clear();
    QJsonArray coordEventsArr = mapObj["coord_events"].toArray();
    for (int i = 0; i < coordEventsArr.size(); i++) {
        QJsonObject event = coordEventsArr[i].toObject();
        QString type = event["type"].toString();
        if (type == "trigger") {
            Event *coord = new Event(event, EventType::Trigger);
            coord->put("map_name", map->name);
            coord->put("x", QString::number(event["x"].toInt()));
            coord->put("y", QString::number(event["y"].toInt()));
            coord->put("elevation", QString::number(event["elevation"].toInt()));
            coord->put("script_var", event["var"].toString());
            coord->put("script_var_value", event["var_value"].toString());
            coord->put("script_label", event["script"].toString());
            coord->put("event_group_type", EventGroup::Coord);
            map->events[EventGroup::Coord].append(coord);
        } else if (type == "weather") {
            Event *coord = new Event(event, EventType::WeatherTrigger);
            coord->put("map_name", map->name);
            coord->put("x", QString::number(event["x"].toInt()));
            coord->put("y", QString::number(event["y"].toInt()));
            coord->put("elevation", QString::number(event["elevation"].toInt()));
            coord->put("weather", event["weather"].toString());
            coord->put("event_group_type", EventGroup::Coord);
            coord->put("event_type", EventType::WeatherTrigger);
            map->events[EventGroup::Coord].append(coord);
        } else {
            logError(QString("Map %1 coord_event %2 has invalid type '%3'. Must be 'trigger' or 'weather'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events[EventGroup::Bg].clear();
    QJsonArray bgEventsArr = mapObj["bg_events"].toArray();
    for (int i = 0; i < bgEventsArr.size(); i++) {
        QJsonObject event = bgEventsArr[i].toObject();
        QString type = event["type"].toString();
        if (type == "sign") {
            Event *bg = new Event(event, EventType::Sign);
            bg->put("map_name", map->name);
            bg->put("x", QString::number(event["x"].toInt()));
            bg->put("y", QString::number(event["y"].toInt()));
            bg->put("elevation", QString::number(event["elevation"].toInt()));
            bg->put("player_facing_direction", event["player_facing_dir"].toString());
            bg->put("script_label", event["script"].toString());
            bg->put("event_group_type", EventGroup::Bg);
            map->events[EventGroup::Bg].append(bg);
        } else if (type == "hidden_item") {
            Event *bg = new Event(event, EventType::HiddenItem);
            bg->put("map_name", map->name);
            bg->put("x", QString::number(event["x"].toInt()));
            bg->put("y", QString::number(event["y"].toInt()));
            bg->put("elevation", QString::number(event["elevation"].toInt()));
            bg->put("item", event["item"].toString());
            bg->put("flag", event["flag"].toString());
            if (projectConfig.getHiddenItemQuantityEnabled()) {
                bg->put("quantity", event["quantity"].toInt());
            }
            if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
                bg->put("underfoot", event["underfoot"].toBool());
            }
            bg->put("event_group_type", EventGroup::Bg);
            map->events[EventGroup::Bg].append(bg);
        } else if (type == "secret_base") {
            Event *bg = new Event(event, EventType::SecretBase);
            bg->put("map_name", map->name);
            bg->put("x", QString::number(event["x"].toInt()));
            bg->put("y", QString::number(event["y"].toInt()));
            bg->put("elevation", QString::number(event["elevation"].toInt()));
            bg->put("secret_base_id", event["secret_base_id"].toString());
            bg->put("event_group_type", EventGroup::Bg);
            map->events[EventGroup::Bg].append(bg);
        } else {
            logError(QString("Map %1 bg_event %2 has invalid type '%3'. Must be 'sign', 'hidden_item', or 'secret_base'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->connections.clear();
    QJsonArray connectionsArr = mapObj["connections"].toArray();
    if (!connectionsArr.isEmpty()) {
        for (int i = 0; i < connectionsArr.size(); i++) {
            QJsonObject connectionObj = connectionsArr[i].toObject();
            MapConnection *connection = new MapConnection;
            connection->direction = connectionObj["direction"].toString();
            connection->offset = QString::number(connectionObj["offset"].toInt());
            QString mapConstant = connectionObj["map"].toString();
            if (mapConstantsToMapNames.contains(mapConstant)) {
                connection->map_name = mapConstantsToMapNames.value(mapConstant);
                map->connections.append(connection);
            } else {
                logError(QString("Failed to find connected map for map constant '%1'").arg(mapConstant));
            }
        }
    }

    // Check for custom fields
    QMap<QString, bool> baseFields = this->getTopLevelMapFields();
    for (QString key : mapObj.keys()) {
        if (!baseFields.contains(key)) {
            map->customHeaders.insert(key, mapObj[key].toString());
        }
    }

    return true;
}

QString Project::readMapLayoutId(QString map_name) {
    if (mapCache.contains(map_name)) {
        return mapCache.value(map_name)->layoutId;
    }

    QString mapFilepath = QString("%1/data/maps/%2/map.json").arg(root).arg(map_name);
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map layout id from %1").arg(mapFilepath));
        return QString();
    }

    QJsonObject mapObj = mapDoc.object();
    return mapObj["layout"].toString();
}

QString Project::readMapLocation(QString map_name) {
    if (mapCache.contains(map_name)) {
        return mapCache.value(map_name)->location;
    }

    QString mapFilepath = QString("%1/data/maps/%2/map.json").arg(root).arg(map_name);
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map's region map section from %1").arg(mapFilepath));
        return QString();
    }

    QJsonObject mapObj = mapDoc.object();
    return mapObj["region_map_section"].toString();
}

void Project::setNewMapHeader(Map* map, int mapIndex) {
    map->layoutId = QString("%1").arg(mapIndex);
    map->location = mapSectionValueToName.value(0);
    map->requiresFlash = "FALSE";
    map->weather = weatherNames.value(0, "WEATHER_NONE");
    map->type = mapTypes.value(0, "MAP_TYPE_NONE");
    map->song = defaultSong;
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        map->show_location = "TRUE";
    } else {
        map->allowBiking = "1";
        map->allowEscapeRope = "0";
        map->allowRunning = "1";
        map->show_location = "1";
    }
    if (projectConfig.getFloorNumberEnabled()) {
        map->floorNumber = 0;
    }

    map->battle_scene = mapBattleScenes.value(0, "MAP_BATTLE_SCENE_NORMAL");
}

bool Project::loadLayout(MapLayout *layout) {
    // Force these to run even if one fails
    bool loadedTilesets = loadLayoutTilesets(layout);
    bool loadedBlockdata = loadBlockdata(layout);
    bool loadedBorder = loadLayoutBorder(layout);

    return loadedTilesets 
        && loadedBlockdata 
        && loadedBorder;
}

bool Project::loadMapLayout(Map* map) {
    if (!map->isPersistedToFile) {
        return true;
    }

    if (mapLayouts.contains(map->layoutId)) {
        map->layout = mapLayouts[map->layoutId];
    } else {
        logError(QString("Error: Map '%1' has an unknown layout '%2'").arg(map->name).arg(map->layoutId));
        return false;
    }

    if (map->hasUnsavedChanges()) {
        return true;
    } else {
        return loadLayout(map->layout);
    }
}

bool Project::readMapLayouts() {
    mapLayouts.clear();
    mapLayoutsTable.clear();

    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    fileWatcher.addPath(layoutsFilepath);
    QJsonDocument layoutsDoc;
    if (!parser.tryParseJsonFile(&layoutsDoc, layoutsFilepath)) {
        logError(QString("Failed to read map layouts from %1").arg(layoutsFilepath));
        return false;
    }

    QJsonObject layoutsObj = layoutsDoc.object();
    QJsonArray layouts = layoutsObj["layouts"].toArray();
    if (layouts.size() == 0) {
        logError(QString("'layouts' array is missing from %1.").arg(layoutsFilepath));
        return false;
    }

    layoutsLabel = layoutsObj["layouts_table_label"].toString();
    if (layoutsLabel.isNull()) {
        layoutsLabel = "gMapLayouts";
        logWarn(QString("'layouts_table_label' value is missing from %1. Defaulting to %2")
                 .arg(layoutsFilepath)
                 .arg(layoutsLabel));
    }

    QList<QString> requiredFields = QList<QString>{
        "id",
        "name",
        "width",
        "height",
        "primary_tileset",
        "secondary_tileset",
        "border_filepath",
        "blockdata_filepath",
    };
    bool useCustomBorderSize = projectConfig.getUseCustomBorderSize();
    if (useCustomBorderSize) {
        requiredFields.append("border_width");
        requiredFields.append("border_height");
    }
    for (int i = 0; i < layouts.size(); i++) {
        QJsonObject layoutObj = layouts[i].toObject();
        if (layoutObj.isEmpty())
            continue;
        if (!parser.ensureFieldsExist(layoutObj, requiredFields)) {
            logError(QString("Layout %1 is missing field(s) in %2.").arg(i).arg(layoutsFilepath));
            return false;
        }
        MapLayout *layout = new MapLayout();
        layout->id = layoutObj["id"].toString();
        if (layout->id.isEmpty()) {
            logError(QString("Missing 'id' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->name = layoutObj["name"].toString();
        if (layout->name.isEmpty()) {
            logError(QString("Missing 'name' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        int lwidth = layoutObj["width"].toInt();
        if (lwidth <= 0) {
            logError(QString("Invalid layout 'width' value '%1' on layout %2 in %3. Must be greater than 0.").arg(lwidth).arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->width = QString::number(lwidth);
        int lheight = layoutObj["height"].toInt();
        if (lheight <= 0) {
            logError(QString("Invalid layout 'height' value '%1' on layout %2 in %3. Must be greater than 0.").arg(lheight).arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->height = QString::number(lheight);
        if (useCustomBorderSize) {
            int bwidth = layoutObj["border_width"].toInt();
            if (bwidth <= 0) {  // 0 is an expected border width/height that should be handled, GF used it for the RS layouts in FRLG
                logWarn(QString("Invalid layout 'border_width' value '%1' on layout %2 in %3. Must be greater than 0. Using default (%4) instead.").arg(bwidth).arg(i).arg(layoutsFilepath).arg(DEFAULT_BORDER_WIDTH));
                bwidth = DEFAULT_BORDER_WIDTH;
            }
            layout->border_width = QString::number(bwidth);
            int bheight = layoutObj["border_height"].toInt();
            if (bheight <= 0) {
                logWarn(QString("Invalid layout 'border_height' value '%1' on layout %2 in %3. Must be greater than 0. Using default (%4) instead.").arg(bheight).arg(i).arg(layoutsFilepath).arg(DEFAULT_BORDER_HEIGHT));
                bheight = DEFAULT_BORDER_HEIGHT;
            }
            layout->border_height = QString::number(bheight);
        } else {
            layout->border_width = QString::number(DEFAULT_BORDER_WIDTH);
            layout->border_height = QString::number(DEFAULT_BORDER_HEIGHT);
        }
        layout->tileset_primary_label = layoutObj["primary_tileset"].toString();
        if (layout->tileset_primary_label.isEmpty()) {
            logError(QString("Missing 'primary_tileset' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->tileset_secondary_label = layoutObj["secondary_tileset"].toString();
        if (layout->tileset_secondary_label.isEmpty()) {
            logError(QString("Missing 'secondary_tileset' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->border_path = layoutObj["border_filepath"].toString();
        if (layout->border_path.isEmpty()) {
            logError(QString("Missing 'border_filepath' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        layout->blockdata_path = layoutObj["blockdata_filepath"].toString();
        if (layout->blockdata_path.isEmpty()) {
            logError(QString("Missing 'blockdata_filepath' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            return false;
        }
        mapLayouts.insert(layout->id, layout);
        mapLayoutsTable.append(layout->id);
    }

    // Deep copy
    mapLayoutsMaster = mapLayouts;
    mapLayoutsMaster.detach();
    mapLayoutsTableMaster = mapLayoutsTable;
    mapLayoutsTableMaster.detach();
    return true;
}

void Project::saveMapLayouts() {
    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(layoutsFilepath));
        return;
    }

    OrderedJson::object layoutsObj;
    layoutsObj["layouts_table_label"] = layoutsLabel;

    bool useCustomBorderSize = projectConfig.getUseCustomBorderSize();
    OrderedJson::array layoutsArr;
    for (QString layoutId : mapLayoutsTableMaster) {
        MapLayout *layout = mapLayouts.value(layoutId);
        OrderedJson::object layoutObj;
        layoutObj["id"] = layout->id;
        layoutObj["name"] = layout->name;
        layoutObj["width"] = layout->width.toInt(nullptr, 0);
        layoutObj["height"] = layout->height.toInt(nullptr, 0);
        if (useCustomBorderSize) {
            layoutObj["border_width"] = layout->border_width.toInt(nullptr, 0);
            layoutObj["border_height"] = layout->border_height.toInt(nullptr, 0);
        }
        layoutObj["primary_tileset"] = layout->tileset_primary_label;
        layoutObj["secondary_tileset"] = layout->tileset_secondary_label;
        layoutObj["border_filepath"] = layout->border_path;
        layoutObj["blockdata_filepath"] = layout->blockdata_path;
        layoutsArr.push_back(layoutObj);
    }

    ignoreWatchedFileTemporarily(layoutsFilepath);

    layoutsObj["layouts"] = layoutsArr;
    OrderedJson layoutJson(layoutsObj);
    OrderedJsonDoc jsonDoc(&layoutJson);
    jsonDoc.dump(&layoutsFile);
    layoutsFile.close();
}

void Project::ignoreWatchedFileTemporarily(QString filepath) {
    // Ignore any file-change events for this filepath for the next 5 seconds.
    modifiedFileTimestamps.insert(filepath, QDateTime::currentMSecsSinceEpoch() + 5000);
}

void Project::setNewMapLayout(Map* map) {
    MapLayout *layout = new MapLayout();
    layout->id = MapLayout::layoutConstantFromName(map->name);
    layout->name = QString("%1_Layout").arg(map->name);
    layout->width = QString::number(getDefaultMapSize());
    layout->height = QString::number(getDefaultMapSize());
    layout->border_width = QString::number(DEFAULT_BORDER_WIDTH);
    layout->border_height = QString::number(DEFAULT_BORDER_HEIGHT);
    layout->border_path = QString("data/layouts/%1/border.bin").arg(map->name);
    layout->blockdata_path = QString("data/layouts/%1/map.bin").arg(map->name);
    layout->tileset_primary_label = tilesetLabels["primary"].value(0, "gTileset_General");
    layout->tileset_secondary_label = tilesetLabels["secondary"].value(0, projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered ? "gTileset_PalletTown" : "gTileset_Petalburg");
    map->layout = layout;
    map->layoutId = layout->id;

    // Insert new entry into the global map layouts.
    mapLayouts.insert(layout->id, layout);
    mapLayoutsTable.append(layout->id);
}

void Project::saveMapGroups() {
    QString mapGroupsFilepath = QString("%1/data/maps/map_groups.json").arg(root);
    QFile mapGroupsFile(mapGroupsFilepath);
    if (!mapGroupsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(mapGroupsFilepath));
        return;
    }

    OrderedJson::object mapGroupsObj;
    mapGroupsObj["layouts_table_label"] = layoutsLabel;

    OrderedJson::array groupNamesArr;
    for (QString groupName : this->groupNames) {
        groupNamesArr.push_back(groupName);
    }
    mapGroupsObj["group_order"] = groupNamesArr;

    int groupNum = 0;
    for (QStringList mapNames : groupedMapNames) {
        OrderedJson::array groupArr;
        for (QString mapName : mapNames) {
            groupArr.push_back(mapName);
        }
        mapGroupsObj[this->groupNames.at(groupNum)] = groupArr;
        groupNum++;
    }

    ignoreWatchedFileTemporarily(mapGroupsFilepath);

    OrderedJson mapGroupJson(mapGroupsObj);
    OrderedJsonDoc jsonDoc(&mapGroupJson);
    jsonDoc.dump(&mapGroupsFile);
    mapGroupsFile.close();
}

void Project::saveWildMonData() {
    if (!projectConfig.getEncounterJsonActive()) return;

    QString wildEncountersJsonFilepath = QString("%1/src/data/wild_encounters.json").arg(root);
    QFile wildEncountersFile(wildEncountersJsonFilepath);
    if (!wildEncountersFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(wildEncountersJsonFilepath));
        return;
    }

    OrderedJson::object wildEncountersObject;
    OrderedJson::array wildEncounterGroups;

    // gWildMonHeaders label is not mutable
    OrderedJson::object monHeadersObject;
    monHeadersObject["label"] = "gWildMonHeaders";
    monHeadersObject["for_maps"] = true;

    OrderedJson::array fieldsInfoArray;
    for (EncounterField fieldInfo : wildMonFields) {
        OrderedJson::object fieldObject;
        OrderedJson::array rateArray;

        for (int rate : fieldInfo.encounterRates) {
            rateArray.push_back(rate);
        }

        fieldObject["type"] = fieldInfo.name;
        fieldObject["encounter_rates"] = rateArray;

        OrderedJson::object groupsObject;
        for (auto groupNamePair : fieldInfo.groups) {
            QString groupName = groupNamePair.first;
            OrderedJson::array subGroupIndices;
            std::sort(fieldInfo.groups[groupName].begin(), fieldInfo.groups[groupName].end());
            for (int slotIndex : fieldInfo.groups[groupName]) {
                subGroupIndices.push_back(slotIndex);
            }
            groupsObject[groupName] = subGroupIndices;
        }
        if (!groupsObject.empty()) fieldObject["groups"] = groupsObject;

        fieldsInfoArray.append(fieldObject);
    }
    monHeadersObject["fields"] = fieldsInfoArray;

    OrderedJson::array encountersArray;
    for (auto keyPair : wildMonData) {
        QString key = keyPair.first;
        for (auto grouplLabelPair : wildMonData[key]) {
            QString groupLabel = grouplLabelPair.first;
            OrderedJson::object encounterObject;
            encounterObject["map"] = key;
            encounterObject["base_label"] = groupLabel;

            WildPokemonHeader encounterHeader = wildMonData[key][groupLabel];
            for (auto fieldNamePair : encounterHeader.wildMons) {
                QString fieldName = fieldNamePair.first;
                OrderedJson::object fieldObject;
                WildMonInfo monInfo = encounterHeader.wildMons[fieldName];
                fieldObject["encounter_rate"] = monInfo.encounterRate;
                OrderedJson::array monArray;
                for (WildPokemon wildMon : monInfo.wildPokemon) {
                    OrderedJson::object monEntry;
                    monEntry["min_level"] = wildMon.minLevel;
                    monEntry["max_level"] = wildMon.maxLevel;
                    monEntry["species"] = wildMon.species;
                    monArray.push_back(monEntry);
                }
                fieldObject["mons"] = monArray;
                encounterObject[fieldName] = fieldObject;
            }
            encountersArray.push_back(encounterObject);
        }
    }
    monHeadersObject["encounters"] = encountersArray;
    wildEncounterGroups.push_back(monHeadersObject);

    // add extra Json objects that are not associated with maps to the file
    for (auto extraObject : extraEncounterGroups) {
        wildEncounterGroups.push_back(extraObject);
    }

    wildEncountersObject["wild_encounter_groups"] = wildEncounterGroups;

    ignoreWatchedFileTemporarily(wildEncountersJsonFilepath);
    OrderedJson encounterJson(wildEncountersObject);
    OrderedJsonDoc jsonDoc(&encounterJson);
    jsonDoc.dump(&wildEncountersFile);
    wildEncountersFile.close();
}

void Project::saveMapConstantsHeader() {
    QString text = QString("#ifndef GUARD_CONSTANTS_MAP_GROUPS_H\n");
    text += QString("#define GUARD_CONSTANTS_MAP_GROUPS_H\n");
    text += QString("\n//\n// DO NOT MODIFY THIS FILE! It is auto-generated from data/maps/map_groups.json\n//\n\n");

    int groupNum = 0;
    for (QStringList mapNames : groupedMapNames) {
        text += "// " + groupNames.at(groupNum) + "\n";
        int maxLength = 0;
        for (QString mapName : mapNames) {
            QString mapConstantName = mapNamesToMapConstants.value(mapName);
            if (mapConstantName.length() > maxLength)
                maxLength = mapConstantName.length();
        }
        int groupIndex = 0;
        for (QString mapName : mapNames) {
            QString mapConstantName = mapNamesToMapConstants.value(mapName);
            text += QString("#define %1%2(%3 | (%4 << 8))\n")
                    .arg(mapConstantName)
                    .arg(QString(" ").repeated(maxLength - mapConstantName.length() + 1))
                    .arg(groupIndex)
                    .arg(groupNum);
            groupIndex++;
        }
        text += QString("\n");
        groupNum++;
    }

    text += QString("#define MAP_GROUPS_COUNT %1\n\n").arg(groupNum);
    text += QString("#endif // GUARD_CONSTANTS_MAP_GROUPS_H\n");

    QString mapGroupFilepath = root + "/include/constants/map_groups.h";
    ignoreWatchedFileTemporarily(mapGroupFilepath);
    saveTextFile(mapGroupFilepath, text);
}

// saves heal location coords in root + /src/data/heal_locations.h
// and indexes as defines in root + /include/constants/heal_locations.h
void Project::saveHealLocationStruct(Map *map) {
    QString constantPrefix, arrayName;
    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        constantPrefix = "SPAWN_";
        arrayName = "sSpawnPoints";
    } else {
        constantPrefix = "HEAL_LOCATION_";
        arrayName = "sHealLocations";
    }

    QString data_text = QString("%1%2struct HealLocation %3[] =\n{\n")
        .arg(dataQualifiers.value("heal_locations").isStatic ? "static " : "")
        .arg(dataQualifiers.value("heal_locations").isConst ? "const " : "")
        .arg(arrayName);

    QString constants_text = QString("#ifndef GUARD_CONSTANTS_HEAL_LOCATIONS_H\n");
    constants_text += QString("#define GUARD_CONSTANTS_HEAL_LOCATIONS_H\n\n");

    QMap<QString, int> healLocationsDupes;
    QSet<QString> healLocationsUnique;

    // set healLocationsDupes and healLocationsUnique
    for (auto it = healLocations.begin(); it != healLocations.end(); it++) {
        HealLocation loc = *it;
        QString xname = loc.idName;
        if (healLocationsUnique.contains(xname)) {
            healLocationsDupes[xname] = 1;
        }
        healLocationsUnique.insert(xname);
    }

    // set new location in healLocations list
    if (map->events[EventGroup::Heal].length() > 0) {
        for (Event *healEvent : map->events[EventGroup::Heal]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            healLocations[hl.index - 1] = hl;
        }
    }

    int i = 1;
    for (auto map_in : healLocations) {
        // add numbered suffix for duplicate constants
        if (healLocationsDupes.keys().contains(map_in.idName)) {
            QString duplicateName = map_in.idName;
            map_in.idName += QString("_%1").arg(healLocationsDupes[duplicateName]);
            healLocationsDupes[duplicateName]++;
        }

        // Save first array (heal location coords), only data array in RSE
        data_text += QString("    [%1%2 - 1] = {MAP_GROUP(%3), MAP_NUM(%3), %4, %5},\n")
                     .arg(constantPrefix)
                     .arg(map_in.idName)
                     .arg(map_in.mapName)
                     .arg(map_in.x)
                     .arg(map_in.y);

        // Save constants
        if (map_in.index != 0) {
            constants_text += QString("#define %1%2 %3\n")
                              .arg(constantPrefix)
                              .arg(map_in.idName)
                              .arg(map_in.index);
        } else {
            constants_text += QString("#define %1%2 %3\n")
                              .arg(constantPrefix)
                              .arg(map_in.idName)
                              .arg(i);
        }
        i++;
    }
    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        // Save second array (map where player respawns for each heal location)
        data_text += QString("};\n\n%1%2u16 sWhiteoutRespawnHealCenterMapIdxs[][2] =\n{\n")
                        .arg(dataQualifiers.value("heal_locations").isStatic ? "static " : "")
                        .arg(dataQualifiers.value("heal_locations").isConst ? "const " : "");
        for (auto map_in : healLocations) {
            data_text += QString("    [%1%2 - 1] = {MAP_GROUP(%3), MAP_NUM(%3)},\n")
                         .arg(constantPrefix)
                         .arg(map_in.idName)
                         .arg(map_in.respawnMap);
        }

        // Save third array (object id of NPC player speaks to upon respawning for each heal location)
        data_text += QString("};\n\n%1%2u8 sWhiteoutRespawnHealerNpcIds[] =\n{\n")
                        .arg(dataQualifiers.value("heal_locations").isStatic ? "static " : "")
                        .arg(dataQualifiers.value("heal_locations").isConst ? "const " : "");
        for (auto map_in : healLocations) {
            data_text += QString("    [%1%2 - 1] = %3,\n")
                         .arg(constantPrefix)
                         .arg(map_in.idName)
                         .arg(map_in.respawnNPC);
        }
    }

    data_text += QString("};\n");
    constants_text += QString("\n#endif // GUARD_CONSTANTS_HEAL_LOCATIONS_H\n");

    QString healLocationFilepath = root + "/src/data/heal_locations.h";
    ignoreWatchedFileTemporarily(healLocationFilepath);
    saveTextFile(healLocationFilepath, data_text);

    QString healLocationConstantsFilepath = root + "/include/constants/heal_locations.h";
    ignoreWatchedFileTemporarily(healLocationConstantsFilepath);
    saveTextFile(healLocationConstantsFilepath, constants_text);
}

void Project::saveTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    saveTilesetMetatileLabels(primaryTileset, secondaryTileset);
    saveTilesetMetatileAttributes(primaryTileset);
    saveTilesetMetatileAttributes(secondaryTileset);
    saveTilesetMetatiles(primaryTileset);
    saveTilesetMetatiles(secondaryTileset);
    saveTilesetTilesImage(primaryTileset);
    saveTilesetTilesImage(secondaryTileset);
    saveTilesetPalettes(primaryTileset);
    saveTilesetPalettes(secondaryTileset);
}

void Project::saveTilesetMetatileLabels(Tileset *primaryTileset, Tileset *secondaryTileset) {
    QString primaryPrefix = QString("METATILE_%1_").arg(QString(primaryTileset->name).replace("gTileset_", ""));
    QString secondaryPrefix = QString("METATILE_%1_").arg(QString(secondaryTileset->name).replace("gTileset_", ""));

    QMap<QString, int> defines;
    bool definesFileModified = false;

    QString metatileLabelsFilename = "include/constants/metatile_labels.h";
    defines = parser.readCDefines(metatileLabelsFilename, (QStringList() << "METATILE_"));

    // Purge old entries from the file.
    QStringList definesToRemove;
    for (QString defineName : defines.keys()) {
        if (defineName.startsWith(primaryPrefix) || defineName.startsWith(secondaryPrefix)) {
            definesToRemove << defineName;
        }
    }
    for (QString defineName : definesToRemove) {
        defines.remove(defineName);
        definesFileModified = true;
    }

    // Add the new labels.
    for (int i = 0; i < primaryTileset->metatiles.size(); i++) {
        Metatile *metatile = primaryTileset->metatiles.at(i);
        if (metatile->label.size() != 0) {
            QString defineName = QString("%1%2").arg(primaryPrefix, metatile->label);
            defines.insert(defineName, i);
            definesFileModified = true;
        }
    }
    for (int i = 0; i < secondaryTileset->metatiles.size(); i++) {
        Metatile *metatile = secondaryTileset->metatiles.at(i);
        if (metatile->label.size() != 0) {
            QString defineName = QString("%1%2").arg(secondaryPrefix, metatile->label);
            defines.insert(defineName, i + Project::num_tiles_primary);
            definesFileModified = true;
        }
    }

    if (!definesFileModified) {
        return;
    }

    auto getTilesetFromLabel = [](QString labelName) {
        return QRegularExpression("METATILE_(?<tileset>[A-Za-z0-9]+)_").match(labelName).captured("tileset");
    };

    QString outputText = "#ifndef GUARD_METATILE_LABELS_H\n";
    outputText += "#define GUARD_METATILE_LABELS_H\n";

    for (int i = 0; i < defines.size();) {
        QString defineName = defines.keys()[i];
        QString currentTileset = getTilesetFromLabel(defineName);
        outputText += QString("\n// gTileset_%1\n").arg(currentTileset);

        int j = 0, longestLength = 0;
        QMap<QString, int> definesOut;

        // Setup for pretty formatting.
        while (i + j < defines.size() && getTilesetFromLabel(defines.keys()[i + j]) == currentTileset) {
            defineName = defines.keys()[i + j];
            if (defineName.size() > longestLength)
                longestLength = defineName.size();
            definesOut.insert(defineName, defines[defineName]);
            j++;
        }
        for (QString defineName : definesOut.keys()) {
            int value = defines[defineName];
            QString line = QString("#define %1  0x%2\n")
                .arg(defineName, -1 * longestLength)
                .arg(QString("%1").arg(value, 3, 16, QChar('0')).toUpper());
            outputText += line;
        }
        i += j;
    }

    outputText += "\n#endif // GUARD_METATILE_LABELS_H\n";

    ignoreWatchedFileTemporarily(root + "/" + metatileLabelsFilename);
    saveTextFile(root + "/" + metatileLabelsFilename, outputText);
}

void Project::saveTilesetMetatileAttributes(Tileset *tileset) {
    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray data;
        BaseGameVersion version = projectConfig.getBaseGameVersion();
        int attrSize = Metatile::getAttributesSize(version);
        for (Metatile *metatile : tileset->metatiles) {
            uint32_t attributes = metatile->getAttributes(version);
            for (int i = 0; i < attrSize; i++)
                data.append(static_cast<char>(attributes >> (8 * i)));
        }
        attrs_file.write(data);
    } else {
        logError(QString("Could not save tileset metatile attributes file '%1'").arg(tileset->metatile_attrs_path));
    }
}

void Project::saveTilesetMetatiles(Tileset *tileset) {
    QFile metatiles_file(tileset->metatiles_path);
    if (metatiles_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray data;
        for (Metatile *metatile : tileset->metatiles) {
            int numTiles = projectConfig.getTripleLayerMetatilesEnabled() ? 12 : 8;
            for (int i = 0; i < numTiles; i++) {
                uint16_t tile = metatile->tiles.at(i).rawValue();
                data.append(static_cast<char>(tile));
                data.append(static_cast<char>(tile >> 8));
            }
        }
        metatiles_file.write(data);
    } else {
        tileset->metatiles.clear();
        logError(QString("Could not open tileset metatiles file '%1'").arg(tileset->metatiles_path));
    }
}

void Project::saveTilesetTilesImage(Tileset *tileset) {
    exportIndexed4BPPPng(tileset->tilesImage, tileset->tilesImagePath);
}

void Project::saveTilesetPalettes(Tileset *tileset) {
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        QString filepath = tileset->palettePaths.at(i);
        PaletteUtil::writeJASC(filepath, tileset->palettes.at(i).toVector(), 0, 16);
    }
}

bool Project::loadLayoutTilesets(MapLayout *layout) {
    layout->tileset_primary = getTileset(layout->tileset_primary_label);
    if (!layout->tileset_primary) {
        QString defaultTileset = tilesetLabels["primary"].value(0, "gTileset_General");
        logWarn(QString("Map layout %1 has invalid primary tileset '%2'. Using default '%3'").arg(layout->id).arg(layout->tileset_primary_label).arg(defaultTileset));
        layout->tileset_primary_label = defaultTileset;
        layout->tileset_primary = getTileset(layout->tileset_primary_label);
        if (!layout->tileset_primary) {
            logError(QString("Failed to set default primary tileset."));
            return false;
        }
    }

    layout->tileset_secondary = getTileset(layout->tileset_secondary_label);
    if (!layout->tileset_secondary) {
        QString defaultTileset = tilesetLabels["secondary"].value(0, projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered ? "gTileset_PalletTown" : "gTileset_Petalburg");
        logWarn(QString("Map layout %1 has invalid secondary tileset '%2'. Using default '%3'").arg(layout->id).arg(layout->tileset_secondary_label).arg(defaultTileset));
        layout->tileset_secondary_label = defaultTileset;
        layout->tileset_secondary = getTileset(layout->tileset_secondary_label);
        if (!layout->tileset_secondary) {
            logError(QString("Failed to set default secondary tileset."));
            return false;
        }
    }
    return true;
}

Tileset* Project::loadTileset(QString label, Tileset *tileset) {
    const QStringList values = parser.getLabelValues(parser.parseAsm("data/tilesets/headers.inc"), label);
    if (values.isEmpty()) {
        return nullptr;
    }
    if (tileset == nullptr) {
        tileset = new Tileset;
    }
    tileset->name = label;
    tileset->is_compressed = values.value(0);
    tileset->is_secondary = values.value(1);
    tileset->padding = values.value(2);
    tileset->tiles_label = values.value(3);
    tileset->palettes_label = values.value(4);
    tileset->metatiles_label = values.value(5);
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        tileset->callback_label = values.value(6);
        tileset->metatile_attrs_label = values.value(7);
    } else {
        tileset->metatile_attrs_label = values.value(6);
        tileset->callback_label = values.value(7);
    }

    loadTilesetAssets(tileset);

    tilesetCache.insert(label, tileset);
    return tileset;
}

bool Project::loadBlockdata(MapLayout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->blockdata_path);
    layout->blockdata = readBlockdata(path);
    layout->lastCommitMapBlocks.blocks = layout->blockdata;
    layout->lastCommitMapBlocks.dimensions = QSize(layout->getWidth(), layout->getHeight());

    if (layout->blockdata.count() != layout->getWidth() * layout->getHeight()) {
        logWarn(QString("Layout blockdata length %1 does not match dimensions %2x%3 (should be %4). Resizing blockdata.")
                .arg(layout->blockdata.count())
                .arg(layout->getWidth())
                .arg(layout->getHeight())
                .arg(layout->getWidth() * layout->getHeight()));
        layout->blockdata.resize(layout->getWidth() * layout->getHeight());
    }
    return true;
}

void Project::setNewMapBlockdata(Map *map) {
    map->layout->blockdata.clear();
    for (int i = 0; i < map->getWidth() * map->getHeight(); i++) {
        map->layout->blockdata.append(qint16(0x3001));
    }
    map->layout->lastCommitMapBlocks.blocks = map->layout->blockdata;
    map->layout->lastCommitMapBlocks.dimensions = QSize(map->getWidth(), map->getHeight());
}

bool Project::loadLayoutBorder(MapLayout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->border_path);
    layout->border = readBlockdata(path);
    int borderLength = layout->getBorderWidth() * layout->getBorderHeight();
    if (layout->border.count() != borderLength) {
        logWarn(QString("Layout border blockdata length %1 must be %2. Resizing border blockdata.")
                .arg(layout->border.count())
                .arg(borderLength));
        layout->border.resize(borderLength);
    }
    return true;
}

void Project::setNewMapBorder(Map *map) {
    map->layout->border.clear();
    if (map->getBorderWidth() != DEFAULT_BORDER_WIDTH || map->getBorderHeight() != DEFAULT_BORDER_HEIGHT) {
        for (int i = 0; i < map->getBorderWidth() * map->getBorderHeight(); i++) {
            map->layout->border.append(0);
        }
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        map->layout->border.append(qint16(0x0014));
        map->layout->border.append(qint16(0x0015));
        map->layout->border.append(qint16(0x001C));
        map->layout->border.append(qint16(0x001D));
    } else {
        map->layout->border.append(qint16(0x01D4));
        map->layout->border.append(qint16(0x01D5));
        map->layout->border.append(qint16(0x01DC));
        map->layout->border.append(qint16(0x01DD));
    }
}

void Project::saveLayoutBorder(Map *map) {
    QString path = QString("%1/%2").arg(root).arg(map->layout->border_path);
    writeBlockdata(path, map->layout->border);
}

void Project::saveLayoutBlockdata(Map* map) {
    QString path = QString("%1/%2").arg(root).arg(map->layout->blockdata_path);
    writeBlockdata(path, map->layout->blockdata);
}

void Project::writeBlockdata(QString path, const Blockdata &blockdata) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray data = blockdata.serialize();
        file.write(data);
    } else {
        logError(QString("Failed to open blockdata file for writing: '%1'").arg(path));
    }
}

void Project::saveAllMaps() {
    for (auto *map : mapCache.values())
        saveMap(map);
}

void Project::saveMap(Map *map) {
    // Create/Modify a few collateral files for brand new maps.
    QString mapDataDir = QString(root + "/data/maps/%1").arg(map->name);
    if (!map->isPersistedToFile) {
        if (!QDir::root().mkdir(mapDataDir)) {
            logError(QString("Error: failed to create directory for new map: '%1'").arg(mapDataDir));
        }

        // Create file data/maps/<map_name>/scripts.inc
        QString text = this->getScriptDefaultString(projectConfig.getUsePoryScript(), map->name);
        saveTextFile(root + "/data/maps/" + map->name + "/scripts" + this->getScriptFileExtension(projectConfig.getUsePoryScript()), text);

        bool usesTextFile = projectConfig.getCreateMapTextFileEnabled();
        if (usesTextFile) {
            // Create file data/maps/<map_name>/text.inc
            saveTextFile(root + "/data/maps/" + map->name + "/text" + this->getScriptFileExtension(projectConfig.getUsePoryScript()), "\n");
        }

        // Simply append to data/event_scripts.s.
        text = QString("\n\t.include \"data/maps/%1/scripts.inc\"\n").arg(map->name);
        if (usesTextFile) {
            text += QString("\t.include \"data/maps/%1/text.inc\"\n").arg(map->name);
        }
        appendTextFile(root + "/data/event_scripts.s", text);

        if (map->needsLayoutDir) {
            QString newLayoutDir = QString(root + "/data/layouts/%1").arg(map->name);
            if (!QDir::root().mkdir(newLayoutDir)) {
                logError(QString("Error: failed to create directory for new layout: '%1'").arg(newLayoutDir));
            }
        }
    }

    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    QJsonDocument layoutsDoc;
    if (!parser.tryParseJsonFile(&layoutsDoc, layoutsFilepath)) {
        logError(QString("Failed to read map layouts from %1").arg(layoutsFilepath));
        return;
    }

    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::ReadWrite)) {
        logError(QString("Error: Could not open %1 for read/write").arg(layoutsFilepath));
        return;
    }

    // Append to "layouts" array in data/layouts/layouts.json.
    QJsonObject layoutsObj = layoutsDoc.object();
    QJsonArray layoutsArr = layoutsObj["layouts"].toArray();
    QJsonObject newLayoutObj;
    newLayoutObj["id"] = map->layout->id;
    newLayoutObj["name"] = map->layout->name;
    newLayoutObj["width"] = map->layout->width.toInt();
    newLayoutObj["height"] = map->layout->height.toInt();
    if (projectConfig.getUseCustomBorderSize()) {
        newLayoutObj["border_width"] = map->layout->border_width.toInt();
        newLayoutObj["border_height"] = map->layout->border_height.toInt();
    }
    newLayoutObj["primary_tileset"] = map->layout->tileset_primary_label;
    newLayoutObj["secondary_tileset"] = map->layout->tileset_secondary_label;
    newLayoutObj["border_filepath"] = map->layout->border_path;
    newLayoutObj["blockdata_filepath"] = map->layout->blockdata_path;
    layoutsArr.append(newLayoutObj);
    layoutsFile.write(layoutsDoc.toJson());
    layoutsFile.close();

    // Create map.json for map data.
    QString mapFilepath = QString("%1/map.json").arg(mapDataDir);
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(mapFilepath));
        return;
    }

    OrderedJson::object mapObj;
    // Header values.
    mapObj["id"] = map->constantName;
    mapObj["name"] = map->name;
    mapObj["layout"] = map->layout->id;
    mapObj["music"] = map->song;
    mapObj["region_map_section"] = map->location;
    mapObj["requires_flash"] = map->requiresFlash.toInt() > 0 || map->requiresFlash == "TRUE";
    mapObj["weather"] = map->weather;
    mapObj["map_type"] = map->type;
    if (projectConfig.getBaseGameVersion() != BaseGameVersion::pokeruby) {
        mapObj["allow_cycling"] = map->allowBiking.toInt() > 0 || map->allowBiking == "TRUE";
        mapObj["allow_escaping"] = map->allowEscapeRope.toInt() > 0 || map->allowEscapeRope == "TRUE";
        mapObj["allow_running"] = map->allowRunning.toInt() > 0 || map->allowRunning == "TRUE";
    }
    mapObj["show_map_name"] = map->show_location.toInt() > 0 || map->show_location == "TRUE";
    if (projectConfig.getFloorNumberEnabled()) {
        mapObj["floor_number"] = map->floorNumber;
    }
    mapObj["battle_scene"] = map->battle_scene;

    // Connections
    if (map->connections.length() > 0) {
        OrderedJson::array connectionsArr;
        for (MapConnection* connection : map->connections) {
            if (mapNamesToMapConstants.contains(connection->map_name)) {
                OrderedJson::object connectionObj;
                connectionObj["direction"] = connection->direction;
                connectionObj["offset"] = connection->offset.toInt();
                connectionObj["map"] = this->mapNamesToMapConstants.value(connection->map_name);
                connectionsArr.append(connectionObj);
            } else {
                logError(QString("Failed to write map connection. '%1' is not a valid map name").arg(connection->map_name));
            }
        }
        mapObj["connections"] = connectionsArr;
    } else {
        mapObj["connections"] = QJsonValue::Null;
    }

    if (map->sharedEventsMap.isEmpty()) {
        // Object events
        OrderedJson::array objectEventsArr;
        for (int i = 0; i < map->events[EventGroup::Object].length(); i++) {
            Event *event = map->events[EventGroup::Object].value(i);
            QString event_type = event->get("event_type");
            OrderedJson::object jsonObj;
            if (event_type == EventType::Object) {
                jsonObj = event->buildObjectEventJSON();
            } else if (event_type == EventType::CloneObject) {
                jsonObj = event->buildCloneObjectEventJSON(mapNamesToMapConstants);
            }
            objectEventsArr.push_back(jsonObj);
        }
        mapObj["object_events"] = objectEventsArr;

        // Warp events
        OrderedJson::array warpEventsArr;
        for (int i = 0; i < map->events[EventGroup::Warp].length(); i++) {
            Event *warp_event = map->events[EventGroup::Warp].value(i);
            OrderedJson::object warpObj = warp_event->buildWarpEventJSON(mapNamesToMapConstants);
            warpEventsArr.append(warpObj);
        }
        mapObj["warp_events"] = warpEventsArr;

        // Coord events
        OrderedJson::array coordEventsArr;
        for (int i = 0; i < map->events[EventGroup::Coord].length(); i++) {
            Event *event = map->events[EventGroup::Coord].value(i);
            QString event_type = event->get("event_type");
            if (event_type == EventType::Trigger) {
                OrderedJson::object triggerObj = event->buildTriggerEventJSON();
                coordEventsArr.append(triggerObj);
            } else if (event_type == EventType::WeatherTrigger) {
                OrderedJson::object weatherObj = event->buildWeatherTriggerEventJSON();
                coordEventsArr.append(weatherObj);
            }
        }
        mapObj["coord_events"] = coordEventsArr;

        // Bg Events
        OrderedJson::array bgEventsArr;
        for (int i = 0; i < map->events[EventGroup::Bg].length(); i++) {
            Event *event = map->events[EventGroup::Bg].value(i);
            QString event_type = event->get("event_type");
            if (event_type == EventType::Sign) {
                OrderedJson::object signObj = event->buildSignEventJSON();
                bgEventsArr.append(signObj);
            } else if (event_type == EventType::HiddenItem) {
                OrderedJson::object hiddenItemObj = event->buildHiddenItemEventJSON();
                bgEventsArr.append(hiddenItemObj);
            } else if (event_type == EventType::SecretBase) {
                OrderedJson::object secretBaseObj = event->buildSecretBaseEventJSON();
                bgEventsArr.append(secretBaseObj);
            }
        }
        mapObj["bg_events"] = bgEventsArr;
    } else {
        mapObj["shared_events_map"] = map->sharedEventsMap;
    }

    if (!map->sharedScriptsMap.isEmpty()) {
        mapObj["shared_scripts_map"] = map->sharedScriptsMap;
    }

    // Custom header fields.
    for (QString key : map->customHeaders.keys()) {
        mapObj[key] = map->customHeaders[key];
    }

    OrderedJson mapJson(mapObj);
    OrderedJsonDoc jsonDoc(&mapJson);
    jsonDoc.dump(&mapFile);
    mapFile.close();

    saveLayoutBorder(map);
    saveLayoutBlockdata(map);
    saveMapHealEvents(map);

    // Update global data structures with current map data.
    updateMapLayout(map);

    map->isPersistedToFile = true;
    map->hasUnsavedDataChanges = false;
    map->editHistory.setClean();
}

void Project::updateMapLayout(Map* map) {
    if (!mapLayoutsTableMaster.contains(map->layoutId)) {
        mapLayoutsTableMaster.append(map->layoutId);
    }

    // Deep copy
    MapLayout *layout = mapLayouts.value(map->layoutId);
    MapLayout *newLayout = new MapLayout();
    *newLayout = *layout;
    mapLayoutsMaster.insert(map->layoutId, newLayout);
}

void Project::saveAllDataStructures() {
    saveMapLayouts();
    saveMapGroups();
    saveMapConstantsHeader();
    saveWildMonData();
}

void Project::loadTilesetAssets(Tileset* tileset) {
    QString category = (tileset->is_secondary == "TRUE") ? "secondary" : "primary";
    if (tileset->name.isNull()) {
        return;
    }
    QRegularExpression re("([a-z])([A-Z0-9])");
    QString tilesetName = tileset->name;
    QString dir_path = root + "/data/tilesets/" + category + '/' + tilesetName.replace("gTileset_", "").replace(re, "\\1_\\2").toLower();

    const QList<QStringList> graphics = parser.parseAsm("data/tilesets/graphics.inc");
    const QStringList tiles_values = parser.getLabelValues(graphics, tileset->tiles_label);
    const QStringList palettes_values = parser.getLabelValues(graphics, tileset->palettes_label);

    QString tiles_path;
    if (!tiles_values.isEmpty()) {
        tiles_path = root + '/' + tiles_values.value(0).section('"', 1, 1);
    } else {
        tiles_path = dir_path + "/tiles.4bpp";
        if (tileset->is_compressed == "TRUE") {
            tiles_path += ".lz";
        }
    }

    if (!palettes_values.isEmpty()) {
        for (const auto &value : palettes_values) {
            tileset->palettePaths.append(this->fixPalettePath(root + '/' + value.section('"', 1, 1)));
        }
    } else {
        QString palettes_dir_path = dir_path + "/palettes";
        for (int i = 0; i < 16; i++) {
            tileset->palettePaths.append(palettes_dir_path + '/' + QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ".pal");
        }
    }

    const QList<QStringList> metatiles_macros = parser.parseAsm("data/tilesets/metatiles.inc");
    const QStringList metatiles_values = parser.getLabelValues(metatiles_macros, tileset->metatiles_label);
    if (!metatiles_values.isEmpty()) {
        tileset->metatiles_path = root + '/' + metatiles_values.value(0).section('"', 1, 1);
    } else {
        tileset->metatiles_path = dir_path + "/metatiles.bin";
    }
    const QStringList metatile_attrs_values = parser.getLabelValues(metatiles_macros, tileset->metatile_attrs_label);
    if (!metatile_attrs_values.isEmpty()) {
        tileset->metatile_attrs_path = root + '/' + metatile_attrs_values.value(0).section('"', 1, 1);
    } else {
        tileset->metatile_attrs_path = dir_path + "/metatile_attributes.bin";
    }

    tiles_path = fixGraphicPath(tiles_path);
    tileset->tilesImagePath = tiles_path;
    QImage image;
    if (QFile::exists(tileset->tilesImagePath)) {
        image = QImage(tileset->tilesImagePath);
    } else {
        image = QImage(8, 8, QImage::Format_Indexed8);
    }
    this->loadTilesetTiles(tileset, image);
    this->loadTilesetMetatiles(tileset);
    this->loadTilesetMetatileLabels(tileset);

    // palettes
    QList<QList<QRgb>> palettes;
    QList<QList<QRgb>> palettePreviews;
    for (int i = 0; i < tileset->palettePaths.length(); i++) {
        QList<QRgb> palette;
        QString path = tileset->palettePaths.value(i);
        QString text = parser.readTextFile(path);
        if (!text.isNull()) {
            QStringList lines = text.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
            if (lines.length() == 19 && lines[0] == "JASC-PAL" && lines[1] == "0100" && lines[2] == "16") {
                for (int j = 0; j < 16; j++) {
                    QStringList rgb = lines[j + 3].split(QRegularExpression(" "), Qt::SkipEmptyParts);
                    if (rgb.length() != 3) {
                        logWarn(QString("Invalid tileset palette RGB value: '%1'").arg(lines[j + 3]));
                        palette.append(qRgb((j - 3) * 16, (j - 3) * 16, (j - 3) * 16));
                    } else {
                        int red = rgb[0].toInt();
                        int green = rgb[1].toInt();
                        int blue = rgb[2].toInt();
                        QRgb color = qRgb(red, green, blue);
                        palette.append(color);
                    }
                }
            } else {
                logError(QString("Invalid JASC-PAL palette file for tileset: '%1'").arg(path));
                for (int j = 0; j < 16; j++) {
                    palette.append(qRgb(j * 16, j * 16, j * 16));
                }
            }
        } else {
            for (int j = 0; j < 16; j++) {
                palette.append(qRgb(j * 16, j * 16, j * 16));
            }
            logError(QString("Could not open tileset palette path '%1'").arg(path));
        }

        palettes.append(palette);
        palettePreviews.append(palette);
    }
    tileset->palettes = palettes;
    tileset->palettePreviews = palettePreviews;
}

void Project::loadTilesetTiles(Tileset *tileset, QImage image) {
    QList<QImage> tiles;
    int w = 8;
    int h = 8;
    for (int y = 0; y < image.height(); y += h)
    for (int x = 0; x < image.width(); x += w) {
        QImage tile = image.copy(x, y, w, h);
        tiles.append(tile);
    }
    tileset->tilesImage = image;
    tileset->tiles = tiles;
}

void Project::loadTilesetMetatiles(Tileset* tileset) {
    QFile metatiles_file(tileset->metatiles_path);
    if (metatiles_file.open(QIODevice::ReadOnly)) {
        QByteArray data = metatiles_file.readAll();
        int metatile_data_length = projectConfig.getTripleLayerMetatilesEnabled() ? 24 : 16;
        int num_metatiles = data.length() / metatile_data_length;
        int num_layers = projectConfig.getTripleLayerMetatilesEnabled() ? 3 : 2;
        QList<Metatile*> metatiles;
        for (int i = 0; i < num_metatiles; i++) {
            Metatile *metatile = new Metatile;
            int index = i * (2 * 4 * num_layers);
            for (int j = 0; j < 4 * num_layers; j++) {
                uint16_t tileRaw = static_cast<unsigned char>(data[index++]);
                tileRaw |= static_cast<unsigned char>(data[index++]) << 8;
                metatile->tiles.append(Tile(tileRaw));
            }
            metatiles.append(metatile);
        }
        tileset->metatiles = metatiles;
    } else {
        tileset->metatiles.clear();
        logError(QString("Could not open tileset metatiles file '%1'").arg(tileset->metatiles_path));
    }

    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::ReadOnly)) {
        QByteArray data = attrs_file.readAll();
        int num_metatiles = tileset->metatiles.count();

        BaseGameVersion version = projectConfig.getBaseGameVersion();
        int attrSize = Metatile::getAttributesSize(version);
        int num_metatileAttrs = data.length() / attrSize;
        if (num_metatiles != num_metatileAttrs) {
            logWarn(QString("Metatile count %1 does not match metatile attribute count %2 in %3").arg(num_metatiles).arg(num_metatileAttrs).arg(tileset->name));
            if (num_metatileAttrs > num_metatiles)
                num_metatileAttrs = num_metatiles;
        }

        for (int i = 0; i < num_metatileAttrs; i++) {
            uint32_t attributes = 0;
            for (int j = 0; j < attrSize; j++)
                attributes |= static_cast<unsigned char>(data.at(i * attrSize + j)) << (8 * j);
            tileset->metatiles.at(i)->setAttributes(attributes, version);
        }
    } else {
        logError(QString("Could not open tileset metatile attributes file '%1'").arg(tileset->metatile_attrs_path));
    }
}

void Project::loadTilesetMetatileLabels(Tileset* tileset) {
    QString tilesetPrefix = QString("METATILE_%1_").arg(QString(tileset->name).replace("gTileset_", ""));
    QString metatileLabelsFilename = "include/constants/metatile_labels.h";
    fileWatcher.addPath(root + "/" + metatileLabelsFilename);
    QMap<QString, int> labels = parser.readCDefines(metatileLabelsFilename, QStringList() << tilesetPrefix);

    for (QString labelName : labels.keys()) {
        int metatileId = labels[labelName];
        // subtract Project::num_tiles_primary from secondary metatiles
        Metatile *metatile = Tileset::getMetatile(metatileId - (tileset->is_secondary == "TRUE" ? Project::num_tiles_primary : 0), tileset, nullptr);
        if (metatile) {
            metatile->label = labelName.replace(tilesetPrefix, "");
        } else {
            QString hexString = QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper();
            logError(QString("Metatile 0x%1 cannot be found in tileset '%2'").arg(hexString, tileset->name));
        }
    }
}

Blockdata Project::readBlockdata(QString path) {
    Blockdata blockdata;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        for (int i = 0; (i + 1) < data.length(); i += 2) {
            uint16_t word = static_cast<uint16_t>((data[i] & 0xff) + ((data[i + 1] & 0xff) << 8));
            blockdata.append(word);
        }
    } else {
        logError(QString("Failed to open blockdata path '%1'").arg(path));
    }

    return blockdata;
}

Map* Project::getMap(QString map_name) {
    if (mapCache.contains(map_name)) {
        return mapCache.value(map_name);
    } else {
        Map *map = loadMap(map_name);
        return map;
    }
}

Tileset* Project::getTileset(QString label, bool forceLoad) {
    Tileset *existingTileset = nullptr;
    if (tilesetCache.contains(label)) {
        existingTileset = tilesetCache.value(label);
    }

    if (existingTileset && !forceLoad) {
        return existingTileset;
    } else {
        Tileset *tileset = loadTileset(label, existingTileset);
        return tileset;
    }
}

void Project::saveTextFile(QString path, QString text) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
    } else {
        logError(QString("Could not open '%1' for writing: ").arg(path) + file.errorString());
    }
}

void Project::appendTextFile(QString path, QString text) {
    QFile file(path);
    if (file.open(QIODevice::Append)) {
        file.write(text.toUtf8());
    } else {
        logError(QString("Could not open '%1' for appending: ").arg(path) + file.errorString());
    }
}

void Project::deleteFile(QString path) {
    QFile file(path);
    if (file.exists() && !file.remove()) {
        logError(QString("Could not delete file '%1': ").arg(path) + file.errorString());
    }
}

bool Project::readWildMonData() {
    extraEncounterGroups.clear();
    wildMonFields.clear();
    wildMonData.clear();
    encounterGroupLabels.clear();
    if (!projectConfig.getEncounterJsonActive()) {
        return true;
    }

    QString wildMonJsonFilepath = QString("%1/src/data/wild_encounters.json").arg(root);
    fileWatcher.addPath(wildMonJsonFilepath);

    OrderedJson::object wildMonObj;
    if (!parser.tryParseOrderedJsonFile(&wildMonObj, wildMonJsonFilepath)) {
        // Failing to read wild encounters data is not a critical error, just disable the
        // encounter editor and log a warning in case the user intended to have this data.
        projectConfig.setEncounterJsonActive(false);
        emit disableWildEncountersUI();
        logWarn(QString("Failed to read wild encounters from %1").arg(wildMonJsonFilepath));
        return true;
    }

    for (OrderedJson subObjectRef : wildMonObj["wild_encounter_groups"].array_items()) {
        OrderedJson::object subObject = subObjectRef.object_items();
        if (!subObject["for_maps"].bool_value()) {
            extraEncounterGroups.push_back(subObject);
            continue;
        }

        for (OrderedJson field : subObject["fields"].array_items()) {
            EncounterField encounterField;
            OrderedJson::object fieldObj = field.object_items();
            encounterField.name = fieldObj["type"].string_value();
            for (auto val : fieldObj["encounter_rates"].array_items()) {
                encounterField.encounterRates.append(val.int_value());
            }

            QList<QString> subGroups;
            for (auto groupPair : fieldObj["groups"].object_items()) {
                subGroups.append(groupPair.first);
            }
            for (QString group : subGroups) {
                OrderedJson::object groupsObj = fieldObj["groups"].object_items();
                for (auto slotNum : groupsObj[group].array_items()) {
                    encounterField.groups[group].append(slotNum.int_value());
                }
            }
            wildMonFields.append(encounterField);
        }

        auto encounters = subObject["encounters"].array_items();
        for (auto encounter : encounters) {
            OrderedJson::object encounterObj = encounter.object_items();
            QString mapConstant = encounterObj["map"].string_value();

            WildPokemonHeader header;

            for (EncounterField monField : wildMonFields) {
                QString field = monField.name;
                if (!encounterObj[field].is_null()) {
                    OrderedJson::object encounterFieldObj = encounterObj[field].object_items();
                    header.wildMons[field].active = true;
                    header.wildMons[field].encounterRate = encounterFieldObj["encounter_rate"].int_value();
                    for (auto mon : encounterFieldObj["mons"].array_items()) {
                        WildPokemon newMon;
                        OrderedJson::object monObj = mon.object_items();
                        newMon.minLevel = monObj["min_level"].int_value();
                        newMon.maxLevel = monObj["max_level"].int_value();
                        newMon.species = monObj["species"].string_value();
                        header.wildMons[field].wildPokemon.append(newMon);
                    }
                }
            }
            wildMonData[mapConstant].insert({encounterObj["base_label"].string_value(), header});
            encounterGroupLabels.append(encounterObj["base_label"].string_value());
        }
    }

    return true;
}

bool Project::readMapGroups() {
    mapConstantsToMapNames.clear();
    mapNamesToMapConstants.clear();
    mapGroups.clear();

    QString mapGroupsFilepath = QString("%1/data/maps/map_groups.json").arg(root);
    fileWatcher.addPath(mapGroupsFilepath);
    QJsonDocument mapGroupsDoc;
    if (!parser.tryParseJsonFile(&mapGroupsDoc, mapGroupsFilepath)) {
        logError(QString("Failed to read map groups from %1").arg(mapGroupsFilepath));
        return false;
    }

    QJsonObject mapGroupsObj = mapGroupsDoc.object();
    QJsonArray mapGroupOrder = mapGroupsObj["group_order"].toArray();

    QList<QStringList> groupedMaps;
    QStringList maps;
    QStringList groups;
    for (int groupIndex = 0; groupIndex < mapGroupOrder.size(); groupIndex++) {
        QString groupName = mapGroupOrder.at(groupIndex).toString();
        QJsonArray mapNames = mapGroupsObj.value(groupName).toArray();
        groupedMaps.append(QStringList());
        groups.append(groupName);
        for (int j = 0; j < mapNames.size(); j++) {
            QString mapName = mapNames.at(j).toString();
            mapGroups.insert(mapName, groupIndex);
            groupedMaps[groupIndex].append(mapName);
            maps.append(mapName);

            // Build the mapping and reverse mapping between map constants and map names.
            QString mapConstant = Map::mapConstantFromName(mapName);
            mapConstantsToMapNames.insert(mapConstant, mapName);
            mapNamesToMapConstants.insert(mapName, mapConstant);
        }
    }

    mapConstantsToMapNames.insert(NONE_MAP_CONSTANT, NONE_MAP_NAME);
    mapNamesToMapConstants.insert(NONE_MAP_NAME, NONE_MAP_CONSTANT);
    maps.append(NONE_MAP_NAME);

    groupNames = groups;
    groupedMapNames = groupedMaps;
    mapNames = maps;
    return true;
}

Map* Project::addNewMapToGroup(QString mapName, int groupNum, Map *newMap, bool existingLayout, bool importedMap) {
    mapNames.append(mapName);
    mapGroups.insert(mapName, groupNum);
    groupedMapNames[groupNum].append(mapName);

    newMap->isPersistedToFile = false;
    newMap->setName(mapName);

    mapConstantsToMapNames.insert(newMap->constantName, newMap->name);
    mapNamesToMapConstants.insert(newMap->name, newMap->constantName);
    if (!existingLayout) {
        mapLayouts.insert(newMap->layoutId, newMap->layout);
        mapLayoutsTable.append(newMap->layoutId);
        if (!importedMap) {
            setNewMapBlockdata(newMap);
        }
        if (newMap->layout->border.isEmpty()) {
            setNewMapBorder(newMap);
        }
    }

    loadLayoutTilesets(newMap->layout);
    setNewMapEvents(newMap);
    setNewMapConnections(newMap);

    return newMap;
}

QString Project::getNewMapName() {
    // Ensure default name doesn't already exist.
    int i = 0;
    QString newMapName;
    do {
        newMapName = QString("NewMap%1").arg(++i);
    } while (mapNames.contains(newMapName));

    return newMapName;
}

QStringList Project::getVisibilities() {
    // TODO
    QStringList names;
    for (int i = 0; i < 16; i++) {
        names.append(QString("%1").arg(i));
    }
    return names;
}

Project::DataQualifiers Project::getDataQualifiers(QString text, QString label) {
    Project::DataQualifiers qualifiers;

    QRegularExpression regex(QString("\\s*(?<static>static\\s*)?(?<const>const\\s*)?[A-Za-z0-9_\\s]*\\b%1\\b").arg(label));
    QRegularExpressionMatch match = regex.match(text);

    qualifiers.isStatic = match.captured("static").isNull() ? false : true;
    qualifiers.isConst = match.captured("const").isNull() ? false : true;

    return qualifiers;
}

QMap<QString, QStringList> Project::getTilesetLabels() {
    QMap<QString, QStringList> allTilesets;
    QStringList primaryTilesets;
    QStringList secondaryTilesets;
    allTilesets.insert("primary", primaryTilesets);
    allTilesets.insert("secondary", secondaryTilesets);
    QList<QString> tilesetLabelsOrdered;

    QString filename = "data/tilesets/headers.inc";
    QString headers_text = parser.readTextFile(root + "/" + filename);
    if (headers_text.isEmpty()) {
        logError(QString("Failed to read tileset labels from %1.").arg(filename));
        return QMap<QString, QStringList>();
    }

    QRegularExpression re("(?<label>[A-Za-z0-9_]*):{1,2}[A-Za-z0-9_@ ]*\\s+.+\\s+\\.byte\\s+(?<isSecondary>[A-Za-z0-9_]+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(headers_text);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString tilesetLabel = match.captured("label");
        QString secondaryTilesetValue = match.captured("isSecondary");

        if (secondaryTilesetValue != "1" && secondaryTilesetValue != "TRUE" 
         && secondaryTilesetValue != "0" && secondaryTilesetValue != "FALSE") {
            logWarn(QString("Unexpected secondary tileset flag found in %1. Expected 'TRUE', 'FALSE', '1', or '0', but found '%2'")
                    .arg(tilesetLabel).arg(secondaryTilesetValue));
            continue;
        }

        bool isSecondaryTileset = (secondaryTilesetValue == "TRUE" || secondaryTilesetValue == "1");
        if (isSecondaryTileset)
            allTilesets["secondary"].append(tilesetLabel);
        else
            allTilesets["primary"].append(tilesetLabel);
        tilesetLabelsOrdered.append(tilesetLabel);
    }
    this->tilesetLabels = allTilesets;
    this->tilesetLabelsOrdered = tilesetLabelsOrdered;
    return allTilesets;
}

bool Project::readTilesetProperties() {
    QStringList definePrefixes;
    definePrefixes << "\\bNUM_";
    QString filename = "include/fieldmap.h";
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> defines = parser.readCDefines(filename, definePrefixes);

    auto it = defines.find("NUM_TILES_IN_PRIMARY");
    if (it != defines.end()) {
        Project::num_tiles_primary = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_TILES_IN_PRIMARY' not found. Using default (%1) instead.")
                .arg(Project::num_tiles_primary));
    }
    it = defines.find("NUM_TILES_TOTAL");
    if (it != defines.end()) {
        Project::num_tiles_total = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_TILES_TOTAL' not found. Using default (%1) instead.")
                .arg(Project::num_tiles_total));
    }
    it = defines.find("NUM_METATILES_IN_PRIMARY");
    if (it != defines.end()) {
        Project::num_metatiles_primary = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_METATILES_IN_PRIMARY' not found. Using default (%1) instead.")
                .arg(Project::num_metatiles_primary));
    }
    it = defines.find("NUM_METATILES_TOTAL");
    if (it != defines.end()) {
        Project::num_metatiles_total = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_METATILES_TOTAL' not found. Using default (%1) instead.")
                .arg(Project::num_metatiles_total));
    }
    it = defines.find("NUM_PALS_IN_PRIMARY");
    if (it != defines.end()) {
        Project::num_pals_primary = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_PALS_IN_PRIMARY' not found. Using default (%1) instead.")
                .arg(Project::num_pals_primary));
    }
    it = defines.find("NUM_PALS_TOTAL");
    if (it != defines.end()) {
        Project::num_pals_total = it.value();
    }
    else {
        logWarn(QString("Value for tileset property 'NUM_PALS_TOTAL' not found. Using default (%1) instead.")
                .arg(Project::num_pals_total));
    }
    return true;
}

bool Project::readMaxMapDataSize() {
    QStringList definePrefixes;
    definePrefixes << "\\bMAX_";
    QString filename = "include/fieldmap.h"; // already in fileWatcher from readTilesetProperties
    QMap<QString, int> defines = parser.readCDefines(filename, definePrefixes);

    auto it = defines.find("MAX_MAP_DATA_SIZE");
    if (it != defines.end()) {
        int min = getMapDataSize(1, 1);
        if (it.value() >= min) {
            Project::max_map_data_size = it.value();
            calculateDefaultMapSize();
        } else {
            // must be large enough to support a 1x1 map
            logWarn(QString("Value for map property 'MAX_MAP_DATA_SIZE' is %1, must be at least %2. Using default (%3) instead.")
                    .arg(it.value())
                    .arg(min)
                    .arg(Project::max_map_data_size));
        }
    }
    else {
        logWarn(QString("Value for map property 'MAX_MAP_DATA_SIZE' not found. Using default (%1) instead.")
                .arg(Project::max_map_data_size));
    }
    return true;
}

bool Project::readRegionMapSections() {
    this->mapSectionNameToValue.clear();
    this->mapSectionValueToName.clear();

    QStringList prefixes = (QStringList() << "\\bMAPSEC_");
    QString filename = "include/constants/region_map_sections.h";
    fileWatcher.addPath(root + "/" + filename);
    this->mapSectionNameToValue = parser.readCDefines(filename, prefixes);
    if (this->mapSectionNameToValue.isEmpty()) {
        logError(QString("Failed to read region map sections from %1.").arg(filename));
        return false;
    }

    for (QString defineName : this->mapSectionNameToValue.keys()) {
        this->mapSectionValueToName.insert(this->mapSectionNameToValue[defineName], defineName);
    }
    return true;
}

bool Project::readHealLocations() {
    dataQualifiers.clear();
    healLocations.clear();
    QString filename = "src/data/heal_locations.h";
    fileWatcher.addPath(root + "/" + filename);
    QString text = parser.readTextFile(root + "/" + filename);
    text.replace(QRegularExpression("//.*?(\r\n?|\n)|/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption), "");

    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        dataQualifiers.insert("heal_locations", getDataQualifiers(text, "sSpawnPoints"));
        QRegularExpression spawnRegex("SPAWN_(?<id>[A-Za-z0-9_]+)\\s*- 1\\]\\s* = \\{MAP_GROUP[\\(\\s]+(?<map>[A-Za-z0-9_]+)[\\s\\)]+,\\s*MAP_NUM[\\(\\s]+(\\2)[\\s\\)]+,\\s*(?<x>[0-9A-Fa-fx]+),\\s*(?<y>[0-9A-Fa-fx]+)");
        QRegularExpression respawnMapRegex("SPAWN_(?<id>[A-Za-z0-9_]+)\\s*- 1\\]\\s* = \\{MAP_GROUP[\\(\\s]+(?<map>[A-Za-z0-9_]+)[\\s\\)]+,\\s*MAP_NUM[\\(\\s]+(\\2)[\\s\\)]+}");
        QRegularExpression respawnNPCRegex("SPAWN_(?<id>[A-Za-z0-9_]+)\\s*- 1\\]\\s* = (?<npc>[0-9]+)");
        QRegularExpressionMatchIterator spawns = spawnRegex.globalMatch(text);
        QRegularExpressionMatchIterator respawnMaps = respawnMapRegex.globalMatch(text);
        QRegularExpressionMatchIterator respawnNPCs = respawnNPCRegex.globalMatch(text);

        // This would be better if idName was used to look up data from the other two arrays
        // As it is, element total and order needs to be the same in the 3 arrays to work. This should always be true though
        for (int i = 1; spawns.hasNext(); i++) {
            QRegularExpressionMatch spawn = spawns.next();
            QRegularExpressionMatch respawnMap = respawnMaps.next();
            QRegularExpressionMatch respawnNPC = respawnNPCs.next();
            QString idName = spawn.captured("id");
            QString mapName = spawn.captured("map");
            QString respawnMapName = respawnMap.captured("map");
            unsigned x = spawn.captured("x").toUShort();
            unsigned y = spawn.captured("y").toUShort();
            unsigned npc = respawnNPC.captured("npc").toUShort();
            healLocations.append(HealLocation(idName, mapName, i, x, y, respawnMapName, npc));
        }
    } else {
        dataQualifiers.insert("heal_locations", getDataQualifiers(text, "sHealLocations"));

        QRegularExpression regex("HEAL_LOCATION_(?<id>[A-Za-z0-9_]+)\\s*- 1\\]\\s* = \\{MAP_GROUP[\\(\\s]+(?<map>[A-Za-z0-9_]+)[\\s\\)]+,\\s*MAP_NUM[\\(\\s]+(\\2)[\\s\\)]+,\\s*(?<x>[0-9A-Fa-fx]+),\\s*(?<y>[0-9A-Fa-fx]+)");
        QRegularExpressionMatchIterator iter = regex.globalMatch(text);
        for (int i = 1; iter.hasNext(); i++) {
            QRegularExpressionMatch match = iter.next();
            QString idName = match.captured("id");
            QString mapName = match.captured("map");
            unsigned x = match.captured("x").toUShort();
            unsigned y = match.captured("y").toUShort();
            healLocations.append(HealLocation(idName, mapName, i, x, y));
        }
    }
    return true;
}

bool Project::readItemNames() {
    QStringList prefixes("\\bITEM_(?!(B_)?USE_)");  // Exclude ITEM_USE_ and ITEM_B_USE_ constants
    QString filename = "include/constants/items.h";
    fileWatcher.addPath(root + "/" + filename);
    itemNames = parser.readCDefinesSorted(filename, prefixes);
    if (itemNames.isEmpty()) {
        logError(QString("Failed to read item constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readFlagNames() {
    // First read MAX_TRAINERS_COUNT, used to skip over trainer flags
    // If this fails flags may simply be out of order, no need to check for success
    QString opponentsFilename = "include/constants/opponents.h";
    fileWatcher.addPath(root + "/" + opponentsFilename);
    QMap<QString, int> maxTrainers = parser.readCDefines(opponentsFilename, QStringList() << "\\bMAX_");
    // Parse flags
    QStringList prefixes("\\bFLAG_");
    QString flagsFilename = "include/constants/flags.h";
    fileWatcher.addPath(root + "/" + flagsFilename);
    flagNames = parser.readCDefinesSorted(flagsFilename, prefixes, maxTrainers);
    if (flagNames.isEmpty()) {
        logError(QString("Failed to read flag constants from %1").arg(flagsFilename));
        return false;
    }
    return true;
}

bool Project::readVarNames() {
    QStringList prefixes("\\bVAR_");
    QString filename = "include/constants/vars.h";
    fileWatcher.addPath(root + "/" + filename);
    varNames = parser.readCDefinesSorted(filename, prefixes);
    if (varNames.isEmpty()) {
        logError(QString("Failed to read var constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMovementTypes() {
    QStringList prefixes("\\bMOVEMENT_TYPE_");
    QString filename = "include/constants/event_object_movement.h";
    fileWatcher.addPath(root + "/" + filename);
    movementTypes = parser.readCDefinesSorted(filename, prefixes);
    if (movementTypes.isEmpty()) {
        logError(QString("Failed to read movement type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readInitialFacingDirections() {
    QString filename = "src/event_object_movement.c";
    fileWatcher.addPath(root + "/" + filename);
    facingDirections = parser.readNamedIndexCArray(filename, "gInitialMovementTypeFacingDirections");
    if (facingDirections.isEmpty()) {
        logError(QString("Failed to read initial movement type facing directions from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMapTypes() {
    QStringList prefixes("\\bMAP_TYPE_");
    QString filename = "include/constants/map_types.h";
    fileWatcher.addPath(root + "/" + filename);
    mapTypes = parser.readCDefinesSorted(filename, prefixes);
    if (mapTypes.isEmpty()) {
        logError(QString("Failed to read map type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMapBattleScenes() {
    QStringList prefixes("\\bMAP_BATTLE_SCENE_");
    QString filename = "include/constants/map_types.h";
    fileWatcher.addPath(root + "/" + filename);
    mapBattleScenes = parser.readCDefinesSorted("include/constants/map_types.h", prefixes);
    if (mapBattleScenes.isEmpty()) {
        logError(QString("Failed to read map battle scene constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readWeatherNames() {
    QStringList prefixes("\\bWEATHER_");
    QString filename = "include/constants/weather.h";
    fileWatcher.addPath(root + "/" + filename);
    weatherNames = parser.readCDefinesSorted(filename, prefixes);
    if (weatherNames.isEmpty()) {
        logError(QString("Failed to read weather constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readCoordEventWeatherNames() {
    if (!projectConfig.getEventWeatherTriggerEnabled())
        return true;

    QStringList prefixes("\\bCOORD_EVENT_WEATHER_");
    QString filename = "include/constants/weather.h";
    fileWatcher.addPath(root + "/" + filename);
    coordEventWeatherNames = parser.readCDefinesSorted(filename, prefixes);
    if (coordEventWeatherNames.isEmpty()) {
        logError(QString("Failed to read coord event weather constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readSecretBaseIds() {
    if (!projectConfig.getEventSecretBaseEnabled())
        return true;

    QStringList prefixes("\\bSECRET_BASE_[A-Za-z0-9_]*_[0-9]+");
    QString filename = "include/constants/secret_bases.h";
    fileWatcher.addPath(root + "/" + filename);
    secretBaseIds = parser.readCDefinesSorted(filename, prefixes);
    if (secretBaseIds.isEmpty()) {
        logError(QString("Failed to read secret base id constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readBgEventFacingDirections() {
    QStringList prefixes("\\bBG_EVENT_PLAYER_FACING_");
    QString filename = "include/constants/event_bg.h";
    fileWatcher.addPath(root + "/" + filename);
    bgEventFacingDirections = parser.readCDefinesSorted(filename, prefixes);
    if (bgEventFacingDirections.isEmpty()) {
        logError(QString("Failed to read bg event facing direction constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readTrainerTypes() {
    QStringList prefixes("\\bTRAINER_TYPE_");
    QString filename = "include/constants/trainer_types.h";
    fileWatcher.addPath(root + "/" + filename);
    trainerTypes = parser.readCDefinesSorted(filename, prefixes);
    if (trainerTypes.isEmpty()) {
        logError(QString("Failed to read trainer type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();

    QStringList prefixes("\\bMB_");
    QString filename = "include/constants/metatile_behaviors.h";
    fileWatcher.addPath(root + "/" + filename);
    this->metatileBehaviorMap = parser.readCDefines(filename, prefixes);
    if (this->metatileBehaviorMap.isEmpty()) {
        logError(QString("Failed to read metatile behaviors from %1.").arg(filename));
        return false;
    }

    for (QString defineName : this->metatileBehaviorMap.keys()) {
        this->metatileBehaviorMapInverse.insert(this->metatileBehaviorMap[defineName], defineName);
    }
    return true;
}

bool Project::readSongNames() {
    QStringList songDefinePrefixes{ "\\bSE_", "\\bMUS_" };
    QString filename = "include/constants/songs.h";
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> songDefines = parser.readCDefines(filename, songDefinePrefixes);
    this->songNames = songDefines.keys();
    this->defaultSong = this->songNames.value(0, "MUS_DUMMY");
    if (this->songNames.isEmpty()) {
        logError(QString("Failed to read song names from %1.").arg(filename));
        return false;
    }
    return true;
}

bool Project::readObjEventGfxConstants() {
    QStringList objEventGfxPrefixes("\\bOBJ_EVENT_GFX_");
    QString filename = "include/constants/event_objects.h";
    fileWatcher.addPath(root + "/" + filename);
    this->gfxDefines = parser.readCDefines(filename, objEventGfxPrefixes);
    if (this->gfxDefines.isEmpty()) {
        logError(QString("Failed to read object event graphics constants from %1.").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMiscellaneousConstants() {
    miscConstants.clear();
    if (projectConfig.getEncounterJsonActive()) {
        QString filename = "include/constants/pokemon.h";
        fileWatcher.addPath(root + "/" + filename);
        QMap<QString, int> pokemonDefines = parser.readCDefines(filename, { "MIN_", "MAX_" });
        miscConstants.insert("max_level_define", pokemonDefines.value("MAX_LEVEL") > pokemonDefines.value("MIN_LEVEL") ? pokemonDefines.value("MAX_LEVEL") : 100);
        miscConstants.insert("min_level_define", pokemonDefines.value("MIN_LEVEL") < pokemonDefines.value("MAX_LEVEL") ? pokemonDefines.value("MIN_LEVEL") : 1);
    }

    QString filename = "include/constants/global.h";
    fileWatcher.addPath(root + "/" + filename);
    QStringList definePrefixes("\\bOBJECT_");
    QMap<QString, int> defines = parser.readCDefines(filename, definePrefixes);

    auto it = defines.find("OBJECT_EVENT_TEMPLATES_COUNT");
    if (it != defines.end()) {
        if (it.value() > 0) {
            Project::max_object_events = it.value();
        } else {
            logWarn(QString("Value for 'OBJECT_EVENT_TEMPLATES_COUNT' is %1, must be greater than 0. Using default (%2) instead.")
                    .arg(it.value())
                    .arg(Project::max_object_events));
        }
    }
    else {
        logWarn(QString("Value for 'OBJECT_EVENT_TEMPLATES_COUNT' not found. Using default (%1) instead.")
                .arg(Project::max_object_events));
    }

    return true;
}

bool Project::readEventScriptLabels() {
    for (const auto &filePath : getEventScriptsFilePaths())
        globalScriptLabels << ParseUtil::getGlobalScriptLabels(filePath);

    eventScriptLabelModel.setStringList(globalScriptLabels);
    eventScriptLabelCompleter.setModel(&eventScriptLabelModel);
    eventScriptLabelCompleter.setCaseSensitivity(Qt::CaseInsensitive);
    eventScriptLabelCompleter.setFilterMode(Qt::MatchContains);

    return true;
}

QString Project::fixPalettePath(QString path) {
    path = path.replace(QRegularExpression("\\.gbapal$"), ".pal");
    return path;
}

QString Project::fixGraphicPath(QString path) {
    path = path.replace(QRegularExpression("\\.lz$"), "");
    path = path.replace(QRegularExpression("\\.[1248]bpp$"), ".png");
    return path;
}

QString Project::getScriptFileExtension(bool usePoryScript) const {
    if(usePoryScript) {
        return ".pory";
    } else {
        return ".inc";
    }
}

QString Project::getScriptDefaultString(bool usePoryScript, QString mapName) const {
    if(usePoryScript)
        return QString("mapscripts %1_MapScripts {}\n").arg(mapName);
    else
        return QString("%1_MapScripts::\n\t.byte 0\n").arg(mapName);
}

QString Project::getMapScriptsFilePath(const QString &mapName) const {
    const bool usePoryscript = projectConfig.getUsePoryScript();
    auto path = QDir::cleanPath(root + "/data/maps/" + mapName + "/scripts");
    auto extension = getScriptFileExtension(usePoryscript);
    if (usePoryscript && !QFile::exists(path + extension))
        extension = getScriptFileExtension(false);
    path += extension;
    return path;
}

QStringList Project::getEventScriptsFilePaths() const {
    QStringList filePaths(QDir::cleanPath(root + "/data/event_scripts.s"));
    const QString scriptsDir = QDir::cleanPath(root + "/data/scripts");
    const QString mapsDir = QDir::cleanPath(root + "/data/maps");
    const bool usePoryscript = projectConfig.getUsePoryScript();

    if (usePoryscript) {
        QDirIterator it_pory_shared(scriptsDir, {"*.pory"}, QDir::Files);
        while (it_pory_shared.hasNext())
            filePaths << it_pory_shared.next();

        QDirIterator it_pory_maps(mapsDir, {"scripts.pory"}, QDir::Files, QDirIterator::Subdirectories);
        while (it_pory_maps.hasNext())
            filePaths << it_pory_maps.next();
    }

    QDirIterator it_inc_shared(scriptsDir, {"*.inc"}, QDir::Files);
    while (it_inc_shared.hasNext())
        filePaths << it_inc_shared.next();

    QDirIterator it_inc_maps(mapsDir, {"scripts.inc"}, QDir::Files, QDirIterator::Subdirectories);
    while (it_inc_maps.hasNext())
        filePaths << it_inc_maps.next();

    return filePaths;
}

QCompleter *Project::getEventScriptLabelCompleter(QStringList additionalScriptLabels) {
    additionalScriptLabels << globalScriptLabels;
    additionalScriptLabels.removeDuplicates();
    eventScriptLabelModel.setStringList(additionalScriptLabels);
    return &eventScriptLabelCompleter;
}

void Project::setEventPixmap(Event * event, bool forceLoad) {
    if (!event || (!event->pixmap.isNull() && !forceLoad))
        return;

    event->spriteWidth = 16;
    event->spriteHeight = 16;
    event->usingSprite = false;

    QString group_type = event->get("event_group_type");
    if (group_type == EventGroup::Object) {
        QString gfxName;
        QString movement;
        QString event_type = event->get("event_type");
        if (event_type == EventType::CloneObject) {
            // Try to get the targeted object to clone
            int eventIndex = event->getInt("target_local_id") - 1;
            Map * clonedMap = getMap(event->get("target_map"));
            Event * clonedEvent = clonedMap ? clonedMap->events[EventGroup::Object].value(eventIndex, nullptr) : nullptr;
            if (clonedEvent && clonedEvent->get("event_type") == EventType::Object) {
                // Get graphics data from cloned object
                gfxName = clonedEvent->get("sprite");
                movement = clonedEvent->get("movement_type");
            } else {
                // Invalid object specified, use default graphics data (as would be shown in-game)
                gfxName = gfxDefines.key(0);
                movement = movementTypes.first();
            }
            // Update clone object's sprite text to match target object
            event->put("sprite", gfxName);
        } else {
            // Get graphics data of regular object
            gfxName = event->get("sprite");
            movement = event->get("movement_type");
        }
        EventGraphics * eventGfx = eventGraphicsMap.value(gfxName, nullptr);
        if (!eventGfx || eventGfx->spritesheet.isNull()) {
            // No sprite associated with this gfx constant.
            // Use default sprite instead.
            event->pixmap = QPixmap(":/images/Entities_16x16.png").copy(0, 0, 16, 16);
        } else {
            event->setFrameFromMovement(facingDirections.value(movement));
            event->setPixmapFromSpritesheet(eventGfx->spritesheet, eventGfx->spriteWidth, eventGfx->spriteHeight, eventGfx->inanimate);
        }
    } else if (group_type == EventGroup::Warp) {
        event->pixmap = QPixmap(":/images/Entities_16x16.png").copy(16, 0, 16, 16);
    } else if (group_type == EventGroup::Coord) {
        event->pixmap = QPixmap(":/images/Entities_16x16.png").copy(32, 0, 16, 16);
    } else if (group_type == EventGroup::Bg) {
        event->pixmap = QPixmap(":/images/Entities_16x16.png").copy(48, 0, 16, 16);
    } else if (group_type == EventGroup::Heal) {
        event->pixmap = QPixmap(":/images/Entities_16x16.png").copy(64, 0, 16, 16);
    }
}

bool Project::readEventGraphics() {
    fileWatcher.addPaths(QStringList() << root + "/" + "src/data/object_events/object_event_graphics_info_pointers.h"
                                       << root + "/" + "src/data/object_events/object_event_graphics_info.h"
                                       << root + "/" + "src/data/object_events/object_event_pic_tables.h"
                                       << root + "/" + "src/data/object_events/object_event_graphics.h");

    QMap<QString, QString> pointerHash = parser.readNamedIndexCArray("src/data/object_events/object_event_graphics_info_pointers.h", "gObjectEventGraphicsInfoPointers");

    qDeleteAll(eventGraphicsMap);
    eventGraphicsMap.clear();
    QStringList gfxNames = gfxDefines.keys();
    QMap<QString, QMap<QString, QString>> gfxInfos = readObjEventGfxInfo();
    for (QString gfxName : gfxNames) {
        EventGraphics * eventGraphics = new EventGraphics;

        QString info_label = pointerHash[gfxName].replace("&", "");
        if (!gfxInfos.contains(info_label))
            continue;

        QMap<QString, QString>gfxInfoAttributes = gfxInfos[info_label];

        eventGraphics->inanimate = gfxInfoAttributes.value("inanimate") == "TRUE";
        QString pic_label = gfxInfoAttributes.value("images");
        QString dimensions_label = gfxInfoAttributes.value("oam");
        QString subsprites_label = gfxInfoAttributes.value("subspriteTables");

        QString gfx_label = parser.readCArray("src/data/object_events/object_event_pic_tables.h", pic_label).value(0);
        gfx_label = gfx_label.section(QRegularExpression("[\\(\\)]"), 1, 1);
        QString path = parser.readCIncbin("src/data/object_events/object_event_graphics.h", gfx_label);

        if (!path.isNull()) {
            path = fixGraphicPath(path);
            eventGraphics->spritesheet = QImage(root + "/" + path);
            if (!eventGraphics->spritesheet.isNull()) {
                // Infer the sprite dimensions from the OAM labels.
                QRegularExpression re("\\S+_(\\d+)x(\\d+)");
                QRegularExpressionMatch dimensionMatch = re.match(dimensions_label);
                QRegularExpressionMatch oamTablesMatch = re.match(subsprites_label);
                if (oamTablesMatch.hasMatch()) {
                    eventGraphics->spriteWidth = oamTablesMatch.captured(1).toInt();
                    eventGraphics->spriteHeight = oamTablesMatch.captured(2).toInt();
                } else if (dimensionMatch.hasMatch()) {
                    eventGraphics->spriteWidth = dimensionMatch.captured(1).toInt();
                    eventGraphics->spriteHeight = dimensionMatch.captured(2).toInt();
                } else {
                    eventGraphics->spriteWidth = eventGraphics->spritesheet.width();
                    eventGraphics->spriteHeight = eventGraphics->spritesheet.height();
                }
            }
        } else {
            eventGraphics->spritesheet = QImage();
            eventGraphics->spriteWidth = 16;
            eventGraphics->spriteHeight = 16;
        }
        eventGraphicsMap.insert(gfxName, eventGraphics);
    }
    return true;
}

QMap<QString, QMap<QString, QString>> Project::readObjEventGfxInfo() {
    // TODO: refactor this to be more general if we end up directly parsing C
    // for more use cases in the future.
    auto cParser = fex::Parser();
    auto tokens = fex::Lexer().LexFile((root + "/src/data/object_events/object_event_graphics_info.h").toStdString());
    auto gfxInfoObjects = cParser.ParseTopLevelObjects(tokens);
    QMap<QString, QMap<QString, QString>> gfxInfos;
    for (auto it = gfxInfoObjects.begin(); it != gfxInfoObjects.end(); it++) {
        QMap<QString, QString> values;
        int i = 0;
        for (const fex::ArrayValue &v : it->second.values()) {
            if (v.type() == fex::ArrayValue::Type::kValuePair) {
                QString key = QString::fromStdString(v.pair().first);
                QString value = QString::fromStdString(v.pair().second->string_value());
                values.insert(key, value);
            } else {
                // This is for backwards compatibility with the old-style version of
                // object_event_graphics_info.h, in which the structs didn't use
                // attribute names to specify each struct member.
                switch (i) {
                    case 8:
                        values.insert("inanimate", QString::fromStdString(v.string_value()));
                        break;
                    case 11:
                        values.insert("oam", QString::fromStdString(v.string_value()));
                        break;
                    case 12:
                        values.insert("subspriteTables", QString::fromStdString(v.string_value()));
                        break;
                    case 14:
                        values.insert("images", QString::fromStdString(v.string_value()));
                        break;
                }
            }
            i++;
        }
        gfxInfos.insert(QString::fromStdString(it->first), values);
    }

    return gfxInfos;
}

bool Project::readSpeciesIconPaths() {
    speciesToIconPath.clear();
    QString srcfilename = "src/pokemon_icon.c";
    QString incfilename = "src/data/graphics/pokemon.h";
    fileWatcher.addPath(root + "/" + srcfilename);
    fileWatcher.addPath(root + "/" + incfilename);
    QMap<QString, QString> monIconNames = parser.readNamedIndexCArray(srcfilename, "gMonIconTable");
    for (QString species : monIconNames.keys()) {
        QString path = parser.readCIncbin(incfilename, monIconNames.value(species));
        speciesToIconPath.insert(species, root + "/" + path.replace("4bpp", "png"));
    }
    return true;
}

void Project::saveMapHealEvents(Map *map) {
    // save heal event changes
    if (map->events[EventGroup::Heal].length() > 0) {
        for (Event *healEvent : map->events[EventGroup::Heal]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            healLocations[hl.index - 1] = hl;
        }
    }
    saveHealLocationStruct(map);
}

void Project::setNewMapEvents(Map *map) {
    map->events[EventGroup::Object].clear();
    map->events[EventGroup::Warp].clear();
    map->events[EventGroup::Heal].clear();
    map->events[EventGroup::Coord].clear();
    map->events[EventGroup::Bg].clear();
}

int Project::getNumTilesPrimary()
{
    return Project::num_tiles_primary;
}

int Project::getNumTilesTotal()
{
    return Project::num_tiles_total;
}

int Project::getNumMetatilesPrimary()
{
    return Project::num_metatiles_primary;
}

int Project::getNumMetatilesTotal()
{
    return Project::num_metatiles_total;
}

int Project::getNumPalettesPrimary()
{
    return Project::num_pals_primary;
}

int Project::getNumPalettesTotal()
{
    return Project::num_pals_total;
}

int Project::getMaxMapDataSize()
{
    return Project::max_map_data_size;
}

int Project::getMapDataSize(int width, int height)
{
    // + 15 and + 14 come from fieldmap.c in pokeruby/pokeemerald/pokefirered.
    return (width + 15) * (height + 14);
}

int Project::getDefaultMapSize()
{
    return Project::default_map_size;
}

int Project::getMaxMapWidth()
{
    return (getMaxMapDataSize() / (1 + 14)) - 15;
}

int Project::getMaxMapHeight()
{
    return (getMaxMapDataSize() / (1 + 15)) - 14;
}

bool Project::mapDimensionsValid(int width, int height) {
    return getMapDataSize(width, height) <= getMaxMapDataSize();
}

// Get largest possible square dimensions for a map up to maximum of 20x20 (arbitrary)
bool Project::calculateDefaultMapSize(){
    int max = getMaxMapDataSize();

    if (max >= getMapDataSize(20, 20)) {
        default_map_size = 20;
    } else if (max >= getMapDataSize(1, 1)) {
        // Below equation derived from max >= (x + 15) * (x + 14)
        // x^2 + 29x + (210 - max), then complete the square and simplify
        default_map_size = qFloor((qSqrt(4 * getMaxMapDataSize() + 1) - 29) / 2);
    } else {
        logError(QString("'MAX_MAP_DATA_SIZE' of %1 is too small to support a 1x1 map. Must be at least %2.")
                    .arg(max)
                    .arg(getMapDataSize(1, 1)));
        return false;
    }
    return true;
}

int Project::getMaxObjectEvents()
{
    return Project::max_object_events;
}
