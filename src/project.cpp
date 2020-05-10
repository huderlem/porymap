#include "project.h"
#include "config.h"
#include "history.h"
#include "historyitem.h"
#include "log.h"
#include "parseutil.h"
#include "paletteutil.h"
#include "tile.h"
#include "tileset.h"
#include "imageexport.h"
#include "map.h"

#include "orderedjson.h"

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

Project::Project(QWidget *parent) : parent(parent)
{
    groupNames = new QStringList;
    mapGroups = new QMap<QString, int>;
    mapNames = new QStringList;
    itemNames = new QStringList;
    flagNames = new QStringList;
    varNames = new QStringList;
    movementTypes = new QStringList;
    mapTypes = new QStringList;
    mapBattleScenes = new QStringList;
    weatherNames = new QStringList;
    coordEventWeatherNames = new QStringList;
    secretBaseIds = new QStringList;
    bgEventFacingDirections = new QStringList;
    trainerTypes = new QStringList;
    mapCache = new QMap<QString, Map*>;
    mapConstantsToMapNames = new QMap<QString, QString>;
    mapNamesToMapConstants = new QMap<QString, QString>;
    tilesetCache = new QMap<QString, Tileset*>;

    initSignals();
}

Project::~Project()
{
    delete this->groupNames;
    delete this->mapGroups;
    delete this->mapNames;
    delete this->itemNames;
    delete this->flagNames;
    delete this->varNames;
    delete this->weatherNames;
    delete this->coordEventWeatherNames;

    delete this->secretBaseIds;
    delete this->movementTypes;
    delete this->bgEventFacingDirections;
    delete this->mapBattleScenes;
    delete this->trainerTypes;
    delete this->mapTypes;

    delete this->mapConstantsToMapNames;
    delete this->mapNamesToMapConstants;
    
    clearMapCache();
    delete this->mapCache;
    clearTilesetCache();
    delete this->tilesetCache;
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

        QMessageBox notice(this->parent);
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
    for (QString mapName : mapCache->keys()) {
        Map *map = mapCache->take(mapName);
        if (map) delete map;
    }
}

void Project::clearTilesetCache() {
    for (QString tilesetName : tilesetCache->keys()) {
        Tileset *tileset = tilesetCache->take(tilesetName);
        if (tileset) delete tileset;
    }
}

Map* Project::loadMap(QString map_name) {
    Map *map;
    if (mapCache->contains(map_name)) {
        map = mapCache->value(map_name);
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

    map->commit();
    map->metatileHistory.save();
    mapCache->insert(map_name, map);
    return map;
}

void Project::setNewMapConnections(Map *map) {
    map->connections.clear();
}

QMap<QString, bool> Project::getTopLevelMapFields() {
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeemerald) {
        return QMap<QString, bool>
        {
            {"id", true},
            {"name", true},
            {"layout", true},
            {"music", true},
            {"region_map_section", true},
            {"requires_flash", true},
            {"weather", true},
            {"map_type", true},
            {"allow_cycling", true},
            {"allow_escaping", true},
            {"allow_running", true},
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
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        return QMap<QString, bool>
        {
            {"id", true},
            {"name", true},
            {"layout", true},
            {"music", true},
            {"region_map_section", true},
            {"requires_flash", true},
            {"weather", true},
            {"map_type", true},
            {"allow_cycling", true},
            {"allow_escaping", true},
            {"allow_running", true},
            {"show_map_name", true},
            {"floor_number", true},
            {"battle_scene", true},
            {"connections", true},
            {"object_events", true},
            {"warp_events", true},
            {"coord_events", true},
            {"bg_events", true},
            {"shared_events_map", true},
            {"shared_scripts_map", true},
        };
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        return QMap<QString, bool>
        {
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
    } else {
        logError("Invalid game version");
        return QMap<QString, bool>();
    }
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
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeemerald) {
        map->allowBiking = QString::number(mapObj["allow_cycling"].toBool());
        map->allowEscapeRope = QString::number(mapObj["allow_escaping"].toBool());
        map->allowRunning = QString::number(mapObj["allow_running"].toBool());
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        map->allowBiking = QString::number(mapObj["allow_cycling"].toBool());
        map->allowEscapeRope = QString::number(mapObj["allow_escaping"].toBool());
        map->allowRunning = QString::number(mapObj["allow_running"].toBool());
        map->floorNumber = mapObj["floor_number"].toInt();
    }
    map->sharedEventsMap = mapObj["shared_events_map"].toString();
    map->sharedScriptsMap = mapObj["shared_scripts_map"].toString();

    // Events
    map->events["object_event_group"].clear();
    QJsonArray objectEventsArr = mapObj["object_events"].toArray();
    for (int i = 0; i < objectEventsArr.size(); i++) {
        QJsonObject event = objectEventsArr[i].toObject();
        Event *object = new Event(event, EventType::Object);
        object->put("map_name", map->name);
        object->put("sprite", event["graphics_id"].toString());
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            object->put("in_connection", event["in_connection"].toBool());
        }
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
        object->put("event_group_type", "object_event_group");
        map->events["object_event_group"].append(object);
    }

    map->events["warp_event_group"].clear();
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
        if (mapConstantsToMapNames->contains(mapConstant)) {
            warp->put("destination_map_name", mapConstantsToMapNames->value(mapConstant));
            warp->put("event_group_type", "warp_event_group");
            map->events["warp_event_group"].append(warp);
        } else if (mapConstant == NONE_MAP_CONSTANT) {
            warp->put("destination_map_name", NONE_MAP_NAME);
            warp->put("event_group_type", "warp_event_group");
            map->events["warp_event_group"].append(warp);
        } else {
            logError(QString("Destination map constant '%1' is invalid for warp").arg(mapConstant));
        }
    }

    map->events["heal_event_group"].clear();
    for (auto it = healLocations.begin(); it != healLocations.end(); it++) {

        HealLocation loc = *it;

        //if TRUE map is flyable / has healing location
        if (loc.mapName == QString(mapNamesToMapConstants->value(map->name)).remove(0,4)) {
            Event *heal = new Event;
            heal->put("map_name", map->name);
            heal->put("x", loc.x);
            heal->put("y", loc.y);
            heal->put("loc_name", loc.mapName);
            heal->put("id_name", loc.idName);
            heal->put("index", loc.index);
            heal->put("elevation", 3); // TODO: change this?
            heal->put("destination_map_name", mapConstantsToMapNames->value(map->name));
            heal->put("event_group_type", "heal_event_group");
            heal->put("event_type", EventType::HealLocation);
            if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
                heal->put("respawn_map", mapConstantsToMapNames->value(QString("MAP_" + loc.respawnMap)));
                heal->put("respawn_npc", loc.respawnNPC);
            }
            map->events["heal_event_group"].append(heal);
        }

    }

    map->events["coord_event_group"].clear();
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
            coord->put("event_group_type", "coord_event_group");
            map->events["coord_event_group"].append(coord);
        } else if (type == "weather") {
            Event *coord = new Event(event, EventType::WeatherTrigger);
            coord->put("map_name", map->name);
            coord->put("x", QString::number(event["x"].toInt()));
            coord->put("y", QString::number(event["y"].toInt()));
            coord->put("elevation", QString::number(event["elevation"].toInt()));
            coord->put("weather", event["weather"].toString());
            coord->put("event_group_type", "coord_event_group");
            coord->put("event_type", EventType::WeatherTrigger);
            map->events["coord_event_group"].append(coord);
        } else {
            logError(QString("Map %1 coord_event %2 has invalid type '%3'. Must be 'trigger' or 'weather'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events["bg_event_group"].clear();
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
            bg->put("event_group_type", "bg_event_group");
            map->events["bg_event_group"].append(bg);
        } else if (type == "hidden_item") {
            Event *bg = new Event(event, EventType::HiddenItem);
            bg->put("map_name", map->name);
            bg->put("x", QString::number(event["x"].toInt()));
            bg->put("y", QString::number(event["y"].toInt()));
            bg->put("elevation", QString::number(event["elevation"].toInt()));
            bg->put("item", event["item"].toString());
            bg->put("flag", event["flag"].toString());
            if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
                bg->put("quantity", event["quantity"].toInt());
                bg->put("underfoot", event["underfoot"].toBool());
            }
            bg->put("event_group_type", "bg_event_group");
            map->events["bg_event_group"].append(bg);
        } else if (type == "secret_base") {
            Event *bg = new Event(event, EventType::SecretBase);
            bg->put("map_name", map->name);
            bg->put("x", QString::number(event["x"].toInt()));
            bg->put("y", QString::number(event["y"].toInt()));
            bg->put("elevation", QString::number(event["elevation"].toInt()));
            bg->put("secret_base_id", event["secret_base_id"].toString());
            bg->put("event_group_type", "bg_event_group");
            map->events["bg_event_group"].append(bg);
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
            if (mapConstantsToMapNames->contains(mapConstant)) {
                connection->map_name = mapConstantsToMapNames->value(mapConstant);
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
    if (mapCache->contains(map_name)) {
        return mapCache->value(map_name)->layoutId;
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
    if (mapCache->contains(map_name)) {
        return mapCache->value(map_name)->location;
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
    map->weather = weatherNames->value(0, "WEATHER_NONE");
    map->type = mapTypes->value(0, "MAP_TYPE_NONE");
    map->song = defaultSong;
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        map->show_location = "TRUE";
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeemerald) {
        map->allowBiking = "1";
        map->allowEscapeRope = "0";
        map->allowRunning = "1";
        map->show_location = "1";
    }  else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        map->allowBiking = "1";
        map->allowEscapeRope = "0";
        map->allowRunning = "1";
        map->show_location = "1";
        map->floorNumber = 0;
    }

    map->battle_scene = mapBattleScenes->value(0, "MAP_BATTLE_SCENE_NORMAL");
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

    // Force these to run even if one fails
    bool loadedTilesets = loadMapTilesets(map);
    bool loadedBlockdata = loadBlockdata(map);
    bool loadedBorder = loadMapBorder(map);

    return loadedTilesets 
        && loadedBlockdata 
        && loadedBorder;
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
    layout->width = "20";
    layout->height = "20";
    layout->border_width = DEFAULT_BORDER_WIDTH;
    layout->border_height = DEFAULT_BORDER_HEIGHT;
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
    for (QString groupName : *this->groupNames) {
        groupNamesArr.push_back(groupName);
    }
    mapGroupsObj["group_order"] = groupNamesArr;

    int groupNum = 0;
    for (QStringList mapNames : groupedMapNames) {
        OrderedJson::array groupArr;
        for (QString mapName : mapNames) {
            groupArr.push_back(mapName);
        }
        mapGroupsObj[this->groupNames->at(groupNum)] = groupArr;
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
        for (QString groupName : fieldInfo.groups.keys()) {
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
            for (QString fieldName : encounterHeader.wildMons.keys()) {
                OrderedJson::object fieldObject;
                WildMonInfo monInfo = encounterHeader.wildMons.value(fieldName);
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
    text += QString("\n");

    int groupNum = 0;
    for (QStringList mapNames : groupedMapNames) {
        text += QString("// Map Group %1\n").arg(groupNum);
        int maxLength = 0;
        for (QString mapName : mapNames) {
            QString mapConstantName = mapNamesToMapConstants->value(mapName);
            if (mapConstantName.length() > maxLength)
                maxLength = mapConstantName.length();
        }
        int groupIndex = 0;
        for (QString mapName : mapNames) {
            QString mapConstantName = mapNamesToMapConstants->value(mapName);
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
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
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
    if (map->events["heal_event_group"].length() > 0) {
        for (Event *healEvent : map->events["heal_event_group"]) {
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
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
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

    defines = parser.readCDefines("include/constants/metatile_labels.h", (QStringList() << "METATILE_"));

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
    for (int i = 0; i < primaryTileset->metatiles->size(); i++) {
        Metatile *metatile = primaryTileset->metatiles->at(i);
        if (metatile->label.size() != 0) {
            QString defineName = QString("%1%2").arg(primaryPrefix, metatile->label);
            defines.insert(defineName, i);
            definesFileModified = true;
        }
    }
    for (int i = 0; i < secondaryTileset->metatiles->size(); i++) {
        Metatile *metatile = secondaryTileset->metatiles->at(i);
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


    saveTextFile(root + "/include/constants/metatile_labels.h", outputText);
}

void Project::saveTilesetMetatileAttributes(Tileset *tileset) {
    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray data;

        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            for (Metatile *metatile : *tileset->metatiles) {
                data.append(static_cast<char>(metatile->behavior));
                data.append(static_cast<char>(metatile->behavior >> 8) |
                            static_cast<char>(metatile->terrainType << 1));
                data.append(static_cast<char>(0));
                data.append(static_cast<char>(metatile->encounterType) | 
                            static_cast<char>(metatile->layerType << 5));
            }
        } else {
            for (Metatile *metatile : *tileset->metatiles) {
                data.append(static_cast<char>(metatile->behavior));
                data.append(static_cast<char>((metatile->layerType << 4) & 0xF0));
            }
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
        for (Metatile *metatile : *tileset->metatiles) {
            for (int i = 0; i < 8; i++) {
                Tile tile = metatile->tiles->at(i);
                uint16_t value = static_cast<uint16_t>((tile.tile & 0x3ff)
                                                    | ((tile.xflip & 1) << 10)
                                                    | ((tile.yflip & 1) << 11)
                                                    | ((tile.palette & 0xf) << 12));
                data.append(static_cast<char>(value & 0xff));
                data.append(static_cast<char>((value >> 8) & 0xff));
            }
        }
        metatiles_file.write(data);
    } else {
        tileset->metatiles = new QList<Metatile*>;
        logError(QString("Could not open tileset metatiles file '%1'").arg(tileset->metatiles_path));
    }
}

void Project::saveTilesetTilesImage(Tileset *tileset) {
    exportIndexed4BPPPng(tileset->tilesImage, tileset->tilesImagePath);
}

void Project::saveTilesetPalettes(Tileset *tileset) {
    PaletteUtil paletteParser;
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        QString filepath = tileset->palettePaths.at(i);
        paletteParser.writeJASC(filepath, tileset->palettes->at(i).toVector(), 0, 16);
    }
}

bool Project::loadMapTilesets(Map* map) {
    if (map->layout->has_unsaved_changes) {
        return true;
    }

    map->layout->tileset_primary = getTileset(map->layout->tileset_primary_label);
    if (!map->layout->tileset_primary) {
        QString defaultTileset = tilesetLabels["primary"].value(0, "gTileset_General");
        logWarn(QString("Map layout %1 has invalid primary tileset '%2'. Using default '%3'").arg(map->layout->id).arg(map->layout->tileset_primary_label).arg(defaultTileset));
        map->layout->tileset_primary_label = defaultTileset;
        map->layout->tileset_primary = getTileset(map->layout->tileset_primary_label);
        if (!map->layout->tileset_primary) {
            logError(QString("Failed to set default primary tileset."));
            return false;
        }
    }

    map->layout->tileset_secondary = getTileset(map->layout->tileset_secondary_label);
    if (!map->layout->tileset_secondary) {
        QString defaultTileset = tilesetLabels["secondary"].value(0, projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered ? "gTileset_PalletTown" : "gTileset_Petalburg");
        logWarn(QString("Map layout %1 has invalid secondary tileset '%2'. Using default '%3'").arg(map->layout->id).arg(map->layout->tileset_secondary_label).arg(defaultTileset));
        map->layout->tileset_secondary_label = defaultTileset;
        map->layout->tileset_secondary = getTileset(map->layout->tileset_secondary_label);
        if (!map->layout->tileset_secondary) {
            logError(QString("Failed to set default secondary tileset."));
            return false;
        }
    }
    return true;
}

Tileset* Project::loadTileset(QString label, Tileset *tileset) {
    QStringList *values = parser.getLabelValues(parser.parseAsm("data/tilesets/headers.inc"), label);
    if (values->isEmpty()) {
        return nullptr;
    }
    if (tileset == nullptr) {
        tileset = new Tileset;
    }
    tileset->name = label;
    tileset->is_compressed = values->value(0);
    tileset->is_secondary = values->value(1);
    tileset->padding = values->value(2);
    tileset->tiles_label = values->value(3);
    tileset->palettes_label = values->value(4);
    tileset->metatiles_label = values->value(5);
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        tileset->callback_label = values->value(6);
        tileset->metatile_attrs_label = values->value(7);
    } else {
        tileset->metatile_attrs_label = values->value(6);
        tileset->callback_label = values->value(7);
    }

    loadTilesetAssets(tileset);

    tilesetCache->insert(label, tileset);
    return tileset;
}

bool Project::loadBlockdata(Map* map) {
    if (!map->isPersistedToFile || map->layout->has_unsaved_changes) {
        return true;
    }

    QString path = QString("%1/%2").arg(root).arg(map->layout->blockdata_path);
    map->layout->blockdata = readBlockdata(path);

    if (map->layout->blockdata->blocks->count() != map->getWidth() * map->getHeight()) {
        logWarn(QString("Layout blockdata length %1 does not match dimensions %2x%3 (should be %4). Resizing blockdata.")
                .arg(map->layout->blockdata->blocks->count())
                .arg(map->getWidth())
                .arg(map->getHeight())
                .arg(map->getWidth() * map->getHeight()));
        map->layout->blockdata->blocks->resize(map->getWidth() * map->getHeight());
    }
    return true;
}

void Project::setNewMapBlockdata(Map* map) {
    Blockdata *blockdata = new Blockdata;
    for (int i = 0; i < map->getWidth() * map->getHeight(); i++) {
        blockdata->addBlock(qint16(0x3001));
    }
    map->layout->blockdata = blockdata;
}

bool Project::loadMapBorder(Map *map) {
    if (!map->isPersistedToFile || map->layout->has_unsaved_changes) {
        return true;
    }

    QString path = QString("%1/%2").arg(root).arg(map->layout->border_path);
    map->layout->border = readBlockdata(path);
    int borderLength = map->getBorderWidth() * map->getBorderHeight();
    if (map->layout->border->blocks->count() != borderLength) {
        logWarn(QString("Layout border blockdata length %1 must be %2. Resizing border blockdata.")
                .arg(map->layout->border->blocks->count())
                .arg(borderLength));
        map->layout->border->blocks->resize(borderLength);
    }
    return true;
}

void Project::setNewMapBorder(Map *map) {
    Blockdata *blockdata = new Blockdata;
    if (map->getBorderWidth() != DEFAULT_BORDER_WIDTH || map->getBorderHeight() != DEFAULT_BORDER_HEIGHT) {
        for (int i = 0; i < map->getBorderWidth() * map->getBorderHeight(); i++) {
            blockdata->addBlock(0);
        }
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        blockdata->addBlock(qint16(0x0014));
        blockdata->addBlock(qint16(0x0015));
        blockdata->addBlock(qint16(0x001C));
        blockdata->addBlock(qint16(0x001D));
    } else {
        blockdata->addBlock(qint16(0x01D4));
        blockdata->addBlock(qint16(0x01D5));
        blockdata->addBlock(qint16(0x01DC));
        blockdata->addBlock(qint16(0x01DD));
    }
    map->layout->border = blockdata;
}

void Project::saveLayoutBorder(Map *map) {
    QString path = QString("%1/%2").arg(root).arg(map->layout->border_path);
    writeBlockdata(path, map->layout->border);
}

void Project::saveLayoutBlockdata(Map* map) {
    QString path = QString("%1/%2").arg(root).arg(map->layout->blockdata_path);
    writeBlockdata(path, map->layout->blockdata);
    map->metatileHistory.save();
}

void Project::writeBlockdata(QString path, Blockdata *blockdata) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray data = blockdata->serialize();
        file.write(data);
    } else {
        logError(QString("Failed to open blockdata file for writing: '%1'").arg(path));
    }
}

void Project::saveAllMaps() {
    QList<QString> keys = mapCache->keys();
    for (int i = 0; i < keys.length(); i++) {
        QString key = keys.value(i);
        Map* map = mapCache->value(key);
        saveMap(map);
    }
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

        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby || projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            // Create file data/maps/<map_name>/text.inc
            saveTextFile(root + "/data/maps/" + map->name + "/text" + this->getScriptFileExtension(projectConfig.getUsePoryScript()), "\n");
        }

        // Simply append to data/event_scripts.s.
        text = QString("\n\t.include \"data/maps/%1/scripts.inc\"\n").arg(map->name);
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby || projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
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
    mapObj["allow_cycling"] = map->allowBiking.toInt() > 0 || map->allowBiking == "TRUE";
    mapObj["allow_escaping"] = map->allowEscapeRope.toInt() > 0 || map->allowEscapeRope == "TRUE";
    mapObj["allow_running"] = map->allowRunning.toInt() > 0 || map->allowRunning == "TRUE";
    mapObj["show_map_name"] = map->show_location.toInt() > 0 || map->show_location == "TRUE";
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        mapObj["floor_number"] = map->floorNumber;
    }
    mapObj["battle_scene"] = map->battle_scene;

    // Connections
    if (map->connections.length() > 0) {
        OrderedJson::array connectionsArr;
        for (MapConnection* connection : map->connections) {
            if (mapNamesToMapConstants->contains(connection->map_name)) {
                OrderedJson::object connectionObj;
                connectionObj["direction"] = connection->direction;
                connectionObj["offset"] = connection->offset.toInt();
                connectionObj["map"] = this->mapNamesToMapConstants->value(connection->map_name);
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
        for (int i = 0; i < map->events["object_event_group"].length(); i++) {
            Event *object_event = map->events["object_event_group"].value(i);
            OrderedJson::object eventObj = object_event->buildObjectEventJSON();
            objectEventsArr.push_back(eventObj);
        }
        mapObj["object_events"] = objectEventsArr;

        // Warp events
        OrderedJson::array warpEventsArr;
        for (int i = 0; i < map->events["warp_event_group"].length(); i++) {
            Event *warp_event = map->events["warp_event_group"].value(i);
            OrderedJson::object warpObj = warp_event->buildWarpEventJSON(mapNamesToMapConstants);
            warpEventsArr.append(warpObj);
        }
        mapObj["warp_events"] = warpEventsArr;

        // Coord events
        OrderedJson::array coordEventsArr;
        for (int i = 0; i < map->events["coord_event_group"].length(); i++) {
            Event *event = map->events["coord_event_group"].value(i);
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
        for (int i = 0; i < map->events["bg_event_group"].length(); i++) {
            Event *event = map->events["bg_event_group"].value(i);
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
    map->layout->has_unsaved_changes = false;
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
    QString dir_path = root + "/data/tilesets/" + category + "/" + tilesetName.replace("gTileset_", "").replace(re, "\\1_\\2").toLower();

    QList<QStringList> *graphics = parser.parseAsm("data/tilesets/graphics.inc");
    QStringList *tiles_values = parser.getLabelValues(graphics, tileset->tiles_label);
    QStringList *palettes_values = parser.getLabelValues(graphics, tileset->palettes_label);

    QString tiles_path;
    if (!tiles_values->isEmpty()) {
        tiles_path = root + "/" + tiles_values->value(0).section('"', 1, 1);
    } else {
        tiles_path = dir_path + "/tiles.4bpp";
        if (tileset->is_compressed == "TRUE") {
            tiles_path += ".lz";
        }
    }

    if (!palettes_values->isEmpty()) {
        for (int i = 0; i < palettes_values->length(); i++) {
            QString value = palettes_values->value(i);
            tileset->palettePaths.append(this->fixPalettePath(root + "/" + value.section('"', 1, 1)));
        }
    } else {
        QString palettes_dir_path = dir_path + "/palettes";
        for (int i = 0; i < 16; i++) {
            tileset->palettePaths.append(palettes_dir_path + "/" + QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ".pal");
        }
    }

    QList<QStringList> *metatiles_macros = parser.parseAsm("data/tilesets/metatiles.inc");
    QStringList *metatiles_values = parser.getLabelValues(metatiles_macros, tileset->metatiles_label);
    if (!metatiles_values->isEmpty()) {
        tileset->metatiles_path = root + "/" + metatiles_values->value(0).section('"', 1, 1);
    } else {
        tileset->metatiles_path = dir_path + "/metatiles.bin";
    }
    QStringList *metatile_attrs_values = parser.getLabelValues(metatiles_macros, tileset->metatile_attrs_label);
    if (!metatile_attrs_values->isEmpty()) {
        tileset->metatile_attrs_path = root + "/" + metatile_attrs_values->value(0).section('"', 1, 1);
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
    QList<QList<QRgb>> *palettes = new QList<QList<QRgb>>;
    QList<QList<QRgb>> *palettePreviews = new QList<QList<QRgb>>;
    for (int i = 0; i < tileset->palettePaths.length(); i++) {
        QList<QRgb> palette;
        QString path = tileset->palettePaths.value(i);
        QString text = parser.readTextFile(path);
        if (!text.isNull()) {
            QStringList lines = text.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
            if (lines.length() == 19 && lines[0] == "JASC-PAL" && lines[1] == "0100" && lines[2] == "16") {
                for (int j = 0; j < 16; j++) {
                    QStringList rgb = lines[j + 3].split(QRegExp(" "), QString::SkipEmptyParts);
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

        palettes->append(palette);
        palettePreviews->append(palette);
    }
    tileset->palettes = palettes;
    tileset->palettePreviews = palettePreviews;
}

void Project::loadTilesetTiles(Tileset *tileset, QImage image) {
    QList<QImage> *tiles = new QList<QImage>;
    int w = 8;
    int h = 8;
    for (int y = 0; y < image.height(); y += h)
    for (int x = 0; x < image.width(); x += w) {
        QImage tile = image.copy(x, y, w, h);
        tiles->append(tile);
    }
    tileset->tilesImage = image;
    tileset->tiles = tiles;
}

void Project::loadTilesetMetatiles(Tileset* tileset) {
    QFile metatiles_file(tileset->metatiles_path);
    if (metatiles_file.open(QIODevice::ReadOnly)) {
        QByteArray data = metatiles_file.readAll();
        int num_metatiles = data.length() / 16;
        int num_layers = 2;
        QList<Metatile*> *metatiles = new QList<Metatile*>;
        for (int i = 0; i < num_metatiles; i++) {
            Metatile *metatile = new Metatile;
            int index = i * (2 * 4 * num_layers);
            for (int j = 0; j < 4 * num_layers; j++) {
                uint16_t word = data[index++] & 0xff;
                word += (data[index++] & 0xff) << 8;
                Tile tile;
                tile.tile = word & 0x3ff;
                tile.xflip = (word >> 10) & 1;
                tile.yflip = (word >> 11) & 1;
                tile.palette = (word >> 12) & 0xf;
                metatile->tiles->append(tile);
            }
            metatiles->append(metatile);
        }
        tileset->metatiles = metatiles;
    } else {
        tileset->metatiles = new QList<Metatile*>;
        logError(QString("Could not open tileset metatiles file '%1'").arg(tileset->metatiles_path));
    }

    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::ReadOnly)) {
        QByteArray data = attrs_file.readAll();
        int num_metatiles = tileset->metatiles->count();

        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            int num_metatileAttrs = data.length() / 4;
            if (num_metatiles != num_metatileAttrs) {
                logWarn(QString("Metatile count %1 does not match metatile attribute count %2 in %3").arg(num_metatiles).arg(num_metatileAttrs).arg(tileset->name));
                if (num_metatileAttrs > num_metatiles)
                    num_metatileAttrs = num_metatiles;
            }
            bool unusedAttribute = false;
            for (int i = 0; i < num_metatileAttrs; i++) {
                int value = (static_cast<unsigned char>(data.at(i * 4 + 3)) << 24) | 
                            (static_cast<unsigned char>(data.at(i * 4 + 2)) << 16) | 
                            (static_cast<unsigned char>(data.at(i * 4 + 1)) << 8) | 
                            (static_cast<unsigned char>(data.at(i * 4 + 0)));
                tileset->metatiles->at(i)->behavior = value & 0x1FF;
                tileset->metatiles->at(i)->terrainType = (value & 0x3E00) >> 9;
                tileset->metatiles->at(i)->encounterType = (value & 0x7000000) >> 24;
                tileset->metatiles->at(i)->layerType = (value & 0x60000000) >> 29;
                if (value & ~(0x67003FFF))
                    unusedAttribute = true;
            }
            if (unusedAttribute)
                logWarn(QString("Unrecognized metatile attributes in %1 will not be saved.").arg(tileset->metatile_attrs_path));
        } else {
            int num_metatileAttrs = data.length() / 2;
            if (num_metatiles != num_metatileAttrs) {
                logWarn(QString("Metatile count %1 does not match metatile attribute count %2 in %3").arg(num_metatiles).arg(num_metatileAttrs).arg(tileset->name));
                if (num_metatileAttrs > num_metatiles)
                    num_metatileAttrs = num_metatiles;
            }
            for (int i = 0; i < num_metatileAttrs; i++) {
                int value = (static_cast<unsigned char>(data.at(i * 2 + 1)) << 8) | static_cast<unsigned char>(data.at(i * 2));
                tileset->metatiles->at(i)->behavior = value & 0xFF;
                tileset->metatiles->at(i)->layerType = (value & 0xF000) >> 12;
                tileset->metatiles->at(i)->encounterType = 0;
                tileset->metatiles->at(i)->terrainType = 0;
            }
        }
    } else {
        logError(QString("Could not open tileset metatile attributes file '%1'").arg(tileset->metatile_attrs_path));
    }
}

void Project::loadTilesetMetatileLabels(Tileset* tileset) {
    QString tilesetPrefix = QString("METATILE_%1_").arg(QString(tileset->name).replace("gTileset_", ""));
    QMap<QString, int> labels = parser.readCDefines("include/constants/metatile_labels.h", QStringList() << tilesetPrefix);

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

Blockdata* Project::readBlockdata(QString path) {
    Blockdata *blockdata = new Blockdata;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        for (int i = 0; (i + 1) < data.length(); i += 2) {
            uint16_t word = static_cast<uint16_t>((data[i] & 0xff) + ((data[i + 1] & 0xff) << 8));
            blockdata->addBlock(word);
        }
    } else {
        logError(QString("Failed to open blockdata path '%1'").arg(path));
    }

    return blockdata;
}

Map* Project::getMap(QString map_name) {
    if (mapCache->contains(map_name)) {
        return mapCache->value(map_name);
    } else {
        Map *map = loadMap(map_name);
        return map;
    }
}

Tileset* Project::getTileset(QString label, bool forceLoad) {
    Tileset *existingTileset = nullptr;
    if (tilesetCache->contains(label)) {
        existingTileset = tilesetCache->value(label);
    }

    if (existingTileset && !forceLoad) {
        return tilesetCache->value(label);
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
    QJsonDocument wildMonsJsonDoc;
    if (!parser.tryParseJsonFile(&wildMonsJsonDoc, wildMonJsonFilepath)) {
        logError(QString("Failed to read wild encounters from %1").arg(wildMonJsonFilepath));
        return false;
    }

    QJsonObject wildMonObj = wildMonsJsonDoc.object();

    for (auto subObjectRef : wildMonObj["wild_encounter_groups"].toArray()) {
        QJsonObject subObject = subObjectRef.toObject();
        if (!subObject["for_maps"].toBool()) {
            QString err;
            QString subObjson = QJsonDocument(subObject).toJson();
            OrderedJson::object orderedSubObject = OrderedJson::parse(subObjson, err).object_items();
            extraEncounterGroups.push_back(orderedSubObject);
            if (!err.isEmpty()) {
                logWarn(QString("Encountered a problem while parsing extra encounter groups: %1").arg(err));
            }
            continue;
        }

        for (auto field : subObject["fields"].toArray()) {
            EncounterField encounterField;
            encounterField.name = field.toObject()["type"].toString();
            for (auto val : field.toObject()["encounter_rates"].toArray()) {
                encounterField.encounterRates.append(val.toInt());
            }
            for (QString group : field.toObject()["groups"].toObject().keys()) {
                for (auto slotNum : field.toObject()["groups"].toObject()[group].toArray()) {
                    encounterField.groups[group].append(slotNum.toInt());
                }
            }
            wildMonFields.append(encounterField);
        }

        QJsonArray encounters = subObject["encounters"].toArray();
        for (QJsonValue encounter : encounters) {
            QString mapConstant = encounter.toObject().value("map").toString();

            WildPokemonHeader header;

            for (EncounterField monField : wildMonFields) {
                QString field = monField.name;
                if (encounter.toObject().value(field) != QJsonValue::Undefined) {
                    header.wildMons[field].active = true;
                    header.wildMons[field].encounterRate = encounter.toObject().value(field).toObject().value("encounter_rate").toInt();
                    for (QJsonValue mon : encounter.toObject().value(field).toObject().value("mons").toArray()) {
                        WildPokemon newMon;
                        newMon.minLevel = mon.toObject().value("min_level").toInt();
                        newMon.maxLevel = mon.toObject().value("max_level").toInt();
                        newMon.species = mon.toObject().value("species").toString();
                        header.wildMons[field].wildPokemon.append(newMon);
                    }
                }
            }
            wildMonData[mapConstant].insert({encounter.toObject().value("base_label").toString(), header});
            encounterGroupLabels.append(encounter.toObject().value("base_label").toString());
        }
    }
    return true;
}

bool Project::readMapGroups() {
    mapConstantsToMapNames->clear();
    mapNamesToMapConstants->clear();
    mapGroups->clear();

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
    QStringList *maps = new QStringList;
    QStringList *groups = new QStringList;
    for (int groupIndex = 0; groupIndex < mapGroupOrder.size(); groupIndex++) {
        QString groupName = mapGroupOrder.at(groupIndex).toString();
        QJsonArray mapNames = mapGroupsObj.value(groupName).toArray();
        groupedMaps.append(QStringList());
        groups->append(groupName);
        for (int j = 0; j < mapNames.size(); j++) {
            QString mapName = mapNames.at(j).toString();
            mapGroups->insert(mapName, groupIndex);
            groupedMaps[groupIndex].append(mapName);
            maps->append(mapName);

            // Build the mapping and reverse mapping between map constants and map names.
            QString mapConstant = Map::mapConstantFromName(mapName);
            mapConstantsToMapNames->insert(mapConstant, mapName);
            mapNamesToMapConstants->insert(mapName, mapConstant);
        }
    }

    mapConstantsToMapNames->insert(NONE_MAP_CONSTANT, NONE_MAP_NAME);
    mapNamesToMapConstants->insert(NONE_MAP_NAME, NONE_MAP_CONSTANT);
    maps->append(NONE_MAP_NAME);

    groupNames = groups;
    groupedMapNames = groupedMaps;
    mapNames = maps;
    return true;
}

Map* Project::addNewMapToGroup(QString mapName, int groupNum) {
    // Setup new map in memory, but don't write to file until map is actually saved later.
    mapNames->append(mapName);
    mapGroups->insert(mapName, groupNum);
    groupedMapNames[groupNum].append(mapName);

    Map *map = new Map;
    map->isPersistedToFile = false;
    map->setName(mapName);
    mapConstantsToMapNames->insert(map->constantName, map->name);
    mapNamesToMapConstants->insert(map->name, map->constantName);
    setNewMapHeader(map, mapLayoutsTable.size() + 1);
    setNewMapLayout(map);
    loadMapTilesets(map);
    setNewMapBlockdata(map);
    setNewMapBorder(map);
    setNewMapEvents(map);
    setNewMapConnections(map);
    map->commit();
    map->metatileHistory.save();
    mapCache->insert(mapName, map);

    return map;
}

Map* Project::addNewMapToGroup(QString mapName, int groupNum, Map *newMap, bool existingLayout) {
    mapNames->append(mapName);
    mapGroups->insert(mapName, groupNum);
    groupedMapNames[groupNum].append(mapName);

    Map *map = new Map;
    map = newMap;

    map->isPersistedToFile = false;
    map->setName(mapName);

    mapConstantsToMapNames->insert(map->constantName, map->name);
    mapNamesToMapConstants->insert(map->name, map->constantName);
    if (!existingLayout) {
        mapLayouts.insert(map->layoutId, map->layout);
        mapLayoutsTable.append(map->layoutId);
        setNewMapBlockdata(map);
        setNewMapBorder(map);
    }

    loadMapTilesets(map);
    setNewMapEvents(map);
    setNewMapConnections(map);

    map->commit();
    map->metatileHistory.save();

    return map;
}

QString Project::getNewMapName() {
    // Ensure default name doesn't already exist.
    int i = 0;
    QString newMapName;
    do {
        newMapName = QString("NewMap%1").arg(++i);
    } while (mapNames->contains(newMapName));

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
    }
    this->tilesetLabels = allTilesets;
    return allTilesets;
}

bool Project::readTilesetProperties() {
    QStringList definePrefixes;
    definePrefixes << "NUM_";
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

bool Project::readRegionMapSections() {
    this->mapSectionNameToValue.clear();
    this->mapSectionValueToName.clear();

    QStringList prefixes = (QStringList() << "MAPSEC_");
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

    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
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
    itemNames->clear();
    QStringList prefixes = (QStringList() << "ITEM_");
    QString filename = "include/constants/items.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, itemNames);
    if (itemNames->isEmpty()) {
        logError(QString("Failed to read item constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readFlagNames() {
    flagNames->clear();
    QStringList prefixes = (QStringList() << "FLAG_");
    QString filename = "include/constants/flags.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, flagNames);
    if (flagNames->isEmpty()) {
        logError(QString("Failed to read flag constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readVarNames() {
    varNames->clear();
    QStringList prefixes = (QStringList() << "VAR_");
    QString filename = "include/constants/vars.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, varNames);
    if (varNames->isEmpty()) {
        logError(QString("Failed to read var constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMovementTypes() {
    movementTypes->clear();
    QStringList prefixes = (QStringList() << "MOVEMENT_TYPE_");
    QString filename = "include/constants/event_object_movement.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, movementTypes);
    if (movementTypes->isEmpty()) {
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
    mapTypes->clear();
    QStringList prefixes = (QStringList() << "MAP_TYPE_");    
    QString filename = "include/constants/map_types.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, mapTypes);
    if (mapTypes->isEmpty()) {
        logError(QString("Failed to read map type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMapBattleScenes() {
    mapBattleScenes->clear();
    QStringList prefixes = (QStringList() << "MAP_BATTLE_SCENE_");
    QString filename = "include/constants/map_types.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted("include/constants/map_types.h", prefixes, mapBattleScenes);
    if (mapBattleScenes->isEmpty()) {
        logError(QString("Failed to read map battle scene constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readWeatherNames() {
    weatherNames->clear();
    QStringList prefixes = (QStringList() << "\\bWEATHER_");
    QString filename = "include/constants/weather.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, weatherNames);
    if (weatherNames->isEmpty()) {
        logError(QString("Failed to read weather constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readCoordEventWeatherNames() {
    coordEventWeatherNames->clear();
    QStringList prefixes = (QStringList() << "COORD_EVENT_WEATHER_");
    QString filename = "include/constants/weather.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, coordEventWeatherNames);
    if (coordEventWeatherNames->isEmpty()) {
        logError(QString("Failed to read coord event weather constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readSecretBaseIds() {
    secretBaseIds->clear();
    QStringList prefixes = (QStringList() << "SECRET_BASE_[A-Za-z0-9_]*_[0-9]+");
    QString filename = "include/constants/secret_bases.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, secretBaseIds);
    if (secretBaseIds->isEmpty()) {
        logError(QString("Failed to read secret base id constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readBgEventFacingDirections() {
    bgEventFacingDirections->clear();
    QStringList prefixes = (QStringList() << "BG_EVENT_PLAYER_FACING_");
    QString filename = "include/constants/event_bg.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, bgEventFacingDirections);
    if (bgEventFacingDirections->isEmpty()) {
        logError(QString("Failed to read bg event facing direction constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readTrainerTypes() {
    trainerTypes->clear();
    QStringList prefixes = (QStringList() << "TRAINER_TYPE_");
    QString filename = "include/constants/trainer_types.h";
    fileWatcher.addPath(root + "/" + filename);
    parser.readCDefinesSorted(filename, prefixes, trainerTypes);
    if (trainerTypes->isEmpty()) {
        logError(QString("Failed to read trainer type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();

    QStringList prefixes = (QStringList() << "MB_");
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

QStringList Project::getSongNames() {
    QStringList songDefinePrefixes;
    songDefinePrefixes << "SE_" << "MUS_";
    QString filename = "include/constants/songs.h";
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> songDefines = parser.readCDefines(filename, songDefinePrefixes);
    QStringList names = songDefines.keys();
    this->defaultSong = names.value(0, "MUS_DUMMY");

    return names;
}

QMap<QString, int> Project::getEventObjGfxConstants() {
    QStringList eventObjGfxPrefixes;
    eventObjGfxPrefixes << "OBJ_EVENT_GFX_";

    QString filename = "include/constants/event_objects.h";
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> constants = parser.readCDefines(filename, eventObjGfxPrefixes);

    return constants;
}

bool Project::readMiscellaneousConstants() {
    miscConstants.clear();
    if (projectConfig.getEncounterJsonActive()) {
        QString filename = "include/constants/pokemon.h";
        fileWatcher.addPath(root + "/" + filename);
        QMap<QString, int> pokemonDefines = parser.readCDefines(filename, QStringList() << "MIN_" << "MAX_");
        miscConstants.insert("max_level_define", pokemonDefines.value("MAX_LEVEL") > pokemonDefines.value("MIN_LEVEL") ? pokemonDefines.value("MAX_LEVEL") : 100);
        miscConstants.insert("min_level_define", pokemonDefines.value("MIN_LEVEL") < pokemonDefines.value("MAX_LEVEL") ? pokemonDefines.value("MIN_LEVEL") : 1);
    }
    return true;
}

QString Project::fixPalettePath(QString path) {
    path = path.replace(QRegExp("\\.gbapal$"), ".pal");
    return path;
}

QString Project::fixGraphicPath(QString path) {
    path = path.replace(QRegExp("\\.lz$"), "");
    path = path.replace(QRegExp("\\.[1248]bpp$"), ".png");
    return path;
}

QString Project::getScriptFileExtension(bool usePoryScript) {
    if(usePoryScript) {
        return ".pory";
    } else {
        return ".inc";
    }
}

QString Project::getScriptDefaultString(bool usePoryScript, QString mapName) {
    if(usePoryScript)
        return QString("mapscripts %1_MapScripts {}").arg(mapName);
    else
        return QString("%1_MapScripts::\n\t.byte 0\n").arg(mapName);
}

void Project::loadEventPixmaps(QList<Event*> objects) {
    bool needs_update = false;
    for (Event *object : objects) {
        if (object->pixmap.isNull()) {
            needs_update = true;
            break;
        }
    }
    if (!needs_update) {
        return;
    }

    QMap<QString, int> constants = getEventObjGfxConstants();

    fileWatcher.addPaths(QStringList() << root + "/" + "src/data/object_events/object_event_graphics_info_pointers.h"
                                       << root + "/" + "src/data/object_events/object_event_graphics_info.h"
                                       << root + "/" + "src/data/object_events/object_event_pic_tables.h"
                                       << root + "/" + "src/data/object_events/object_event_graphics.h");

    QMap<QString, QString> pointerHash = parser.readNamedIndexCArray("src/data/object_events/object_event_graphics_info_pointers.h", "gObjectEventGraphicsInfoPointers");

    for (Event *object : objects) {
        if (!object->pixmap.isNull()) {
            continue;
        }

        object->spriteWidth = 16;
        object->spriteHeight = 16;
        object->usingSprite = false;
        QString event_type = object->get("event_type");
        if (event_type == EventType::Object) {
            object->pixmap = QPixmap(":/images/Entities_16x16.png").copy(0, 0, 16, 16);
        } else if (event_type == EventType::Warp) {
            object->pixmap = QPixmap(":/images/Entities_16x16.png").copy(16, 0, 16, 16);
        } else if (event_type == EventType::Trigger || event_type == EventType::WeatherTrigger) {
            object->pixmap = QPixmap(":/images/Entities_16x16.png").copy(32, 0, 16, 16);
        } else if (event_type == EventType::Sign || event_type == EventType::HiddenItem || event_type == EventType::SecretBase) {
            object->pixmap = QPixmap(":/images/Entities_16x16.png").copy(48, 0, 16, 16);
        } else if (event_type == EventType::HealLocation) {
            object->pixmap = QPixmap(":/images/Entities_16x16.png").copy(64, 0, 16, 16);
        }

        if (event_type == EventType::Object) {
            QString info_label = pointerHash[object->get("sprite")].replace("&", "");
            QStringList gfx_info = parser.readCArray("src/data/object_events/object_event_graphics_info.h", info_label);
            QString pic_label = gfx_info.value(14);
            QString dimensions_label = gfx_info.value(11);
            QString subsprites_label = gfx_info.value(12);
            QString gfx_label = parser.readCArray("src/data/object_events/object_event_pic_tables.h", pic_label).value(0);
            gfx_label = gfx_label.section(QRegExp("[\\(\\)]"), 1, 1);
            QString path = parser.readCIncbin("src/data/object_events/object_event_graphics.h", gfx_label);

            if (!path.isNull()) {
                path = fixGraphicPath(path);
                QImage spritesheet(root + "/" + path);
                if (!spritesheet.isNull()) {
                    // Infer the sprite dimensions from the OAM labels.
                    int spriteWidth, spriteHeight;
                    QRegularExpression re("\\S+_(\\d+)x(\\d+)");
                    QRegularExpressionMatch dimensionMatch = re.match(dimensions_label);
                    QRegularExpressionMatch oamTablesMatch = re.match(subsprites_label);
                    if (oamTablesMatch.hasMatch()) {
                        spriteWidth = oamTablesMatch.captured(1).toInt();
                        spriteHeight = oamTablesMatch.captured(2).toInt();
                    } else if (dimensionMatch.hasMatch()) {
                        spriteWidth = dimensionMatch.captured(1).toInt();
                        spriteHeight = dimensionMatch.captured(2).toInt();
                    } else {
                        spriteWidth = spritesheet.width();
                        spriteHeight = spritesheet.height();
                    }
                    object->setPixmapFromSpritesheet(spritesheet, spriteWidth, spriteHeight, object->frame, object->hFlip);
                }
            }
        }
    }
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
    if (map->events["heal_event_group"].length() > 0) {
        for (Event *healEvent : map->events["heal_event_group"]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            healLocations[hl.index - 1] = hl;
        }
    }
    saveHealLocationStruct(map);
}

void Project::setNewMapEvents(Map *map) {
    map->events["object_event_group"].clear();
    map->events["warp_event_group"].clear();
    map->events["heal_event_group"].clear();
    map->events["coord_event_group"].clear();
    map->events["bg_event_group"].clear();
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
