#include "project.h"
#include "config.h"
#include "history.h"
#include "historyitem.h"
#include "log.h"
#include "parseutil.h"
#include "tile.h"
#include "tileset.h"
#include "event.h"

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

int Project::num_tiles_primary = 512;
int Project::num_tiles_total = 1024;
int Project::num_metatiles_primary = 512;
int Project::num_metatiles_total = 1024;
int Project::num_pals_primary = 6;
int Project::num_pals_total = 13;

Project::Project()
{
    groupNames = new QStringList;
    map_groups = new QMap<QString, int>;
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
    map_cache = new QMap<QString, Map*>;
    mapConstantsToMapNames = new QMap<QString, QString>;
    mapNamesToMapConstants = new QMap<QString, QString>;
    tileset_cache = new QMap<QString, Tileset*>;
}

QString Project::getProjectTitle() {
    if (!root.isNull()) {
        return root.section('/', -1);
    } else {
        return QString();
    }
}

Map* Project::loadMap(QString map_name) {
    Map *map;
    if (map_cache->contains(map_name)) {
        map = map_cache->value(map_name);
        // TODO: uncomment when undo/redo history is fully implemented for all actions.
        if (true/*map->hasUnsavedChanges()*/) {
            return map;
        }
    } else {
        map = new Map;
        map->setName(map_name);
    }

    if (!loadMapData(map))
        return nullptr;

    loadMapLayout(map);
    map->commit();
    map->metatileHistory.save();
    map_cache->insert(map_name, map);
    return map;
}

void Project::setNewMapConnections(Map *map) {
    map->connections.clear();
}

QList<QStringList>* Project::getLabelMacros(QList<QStringList> *list, QString label) {
    bool in_label = false;
    QList<QStringList> *new_list = new QList<QStringList>;
    for (int i = 0; i < list->length(); i++) {
        QStringList params = list->value(i);
        QString macro = params.value(0);
        if (macro == ".label") {
            if (params.value(1) == label) {
                in_label = true;
            } else if (in_label) {
                // If nothing has been read yet, assume the label
                // we're looking for is in a stack of labels.
                if (new_list->length() > 0) {
                    break;
                }
            }
        } else if (in_label) {
            new_list->append(params);
        }
    }
    return new_list;
}

// For if you don't care about filtering by macro,
// and just want all values associated with some label.
QStringList* Project::getLabelValues(QList<QStringList> *list, QString label) {
    list = getLabelMacros(list, label);
    QStringList *values = new QStringList;
    for (int i = 0; i < list->length(); i++) {
        QStringList params = list->value(i);
        QString macro = params.value(0);
        // Ignore .align
        if (macro == ".align")
            continue;
        if (macro == ".ifdef")
            continue;
        if (macro == ".ifndef")
            continue;
        for (int j = 1; j < params.length(); j++) {
            values->append(params.value(j));
        }
    }
    return values;
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
            {"allow_bike", true},
            {"allow_escape_rope", true},
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
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(mapFilepath));
        return false;
    }

    QByteArray mapData = mapFile.readAll();
    QJsonDocument mapDoc = QJsonDocument::fromJson(mapData);
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
        map->allowBiking = QString::number(mapObj["allow_bike"].toBool());
        map->allowEscapeRope = QString::number(mapObj["allow_escape_rope"].toBool());
        map->allowRunning = QString::number(mapObj["allow_running"].toBool());
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
    for (auto it = flyableMaps.begin(); it != flyableMaps.end(); it++) {

        HealLocation loc = *it;

        //if TRUE map is flyable / has healing location
        if (loc.name == QString(mapNamesToMapConstants->value(map->name)).remove(0,4)) {
            Event *heal = new Event;
            heal->put("map_name", map->name);
            heal->put("x", loc.x);
            heal->put("y", loc.y);
            heal->put("loc_name", loc.name);
            heal->put("index", loc.index);
            heal->put("elevation", 3); // TODO: change this?
            heal->put("destination_map_name", mapConstantsToMapNames->value(map->name));
            heal->put("event_group_type", "heal_event_group");
            heal->put("event_type", EventType::HealLocation);
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
    if (map_cache->contains(map_name)) {
        return map_cache->value(map_name)->layoutId;
    }

    QString mapFilepath = QString("%1/data/maps/%2/map.json").arg(root).arg(map_name);
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(mapFilepath));
        return QString::null;
    }

    QByteArray mapData = mapFile.readAll();
    QJsonDocument mapDoc = QJsonDocument::fromJson(mapData);
    QJsonObject mapObj = mapDoc.object();
    return mapObj["layout"].toString();
}

QString Project::readMapLocation(QString map_name) {
    if (map_cache->contains(map_name)) {
        return map_cache->value(map_name)->location;
    }

    QString mapFilepath = QString("%1/data/maps/%2/map.json").arg(root).arg(map_name);
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(mapFilepath));
        return QString::null;
    }

    QByteArray mapData = mapFile.readAll();
    QJsonDocument mapDoc = QJsonDocument::fromJson(mapData);
    QJsonObject mapObj = mapDoc.object();
    return mapObj["region_map_section"].toString();
}

void Project::setNewMapHeader(Map* map, int mapIndex) {
    map->song = "MUS_DAN02";
    map->layoutId = QString("%1").arg(mapIndex);
    map->location = "MAPSEC_LITTLEROOT_TOWN";
    map->requiresFlash = "FALSE";
    map->weather = "WEATHER_SUNNY";
    map->type = "MAP_TYPE_TOWN";
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        map->show_location = "TRUE";
    } else if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeemerald) {
        map->allowBiking = "1";
        map->allowEscapeRope = "0";
        map->allowRunning = "1";
        map->show_location = "1";
    }

    map->battle_scene = "MAP_BATTLE_SCENE_NORMAL";
}

void Project::loadMapLayout(Map* map) {
    if (!map->isPersistedToFile) {
        return;
    }

    if (!mapLayouts.contains(map->layoutId)) {
        logError(QString("Error: Map '%1' has an unknown layout '%2'").arg(map->name).arg(map->layoutId));
        return;
    } else {
        map->layout = mapLayouts[map->layoutId];
    }

    loadMapTilesets(map);
    loadBlockdata(map);
    loadMapBorder(map);
}

void Project::readMapLayouts() {
    mapLayouts.clear();
    mapLayoutsTable.clear();

    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(layoutsFilepath));
        return;
    }

    QByteArray layoutsData = layoutsFile.readAll();
    QJsonDocument layoutsDoc = QJsonDocument::fromJson(layoutsData);
    QJsonObject layoutsObj = layoutsDoc.object();
    layoutsLabel = layoutsObj["layouts_table_label"].toString();

    QJsonArray layouts = layoutsObj["layouts"].toArray();
    for (int i = 0; i < layouts.size(); i++) {
        QJsonObject layoutObj = layouts[i].toObject();
        MapLayout *layout = new MapLayout();
        layout->id = layoutObj["id"].toString();
        layout->name = layoutObj["name"].toString();
        layout->width = QString::number(layoutObj["width"].toInt());
        layout->height = QString::number(layoutObj["height"].toInt());
        layout->tileset_primary_label = layoutObj["primary_tileset"].toString();
        layout->tileset_secondary_label = layoutObj["secondary_tileset"].toString();
        layout->border_path = layoutObj["border_filepath"].toString();
        layout->blockdata_path = layoutObj["blockdata_filepath"].toString();
        mapLayouts.insert(layout->id, layout);
        mapLayoutsTable.append(layout->id);
    }

    // Deep copy
    mapLayoutsMaster = mapLayouts;
    mapLayoutsMaster.detach();
    mapLayoutsTableMaster = mapLayoutsTable;
    mapLayoutsTableMaster.detach();
}

void Project::saveMapLayouts() {
    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(layoutsFilepath));
        return;
    }

    QJsonObject layoutsObj;
    layoutsObj["layouts_table_label"] = layoutsLabel;

    QJsonArray layoutsArr;
    for (QString layoutId : mapLayoutsTableMaster) {
        MapLayout *layout = mapLayouts.value(layoutId);
        QJsonObject layoutObj;
        layoutObj["id"] = layout->id;
        layoutObj["name"] = layout->name;
        layoutObj["width"] = layout->width.toInt(nullptr, 0);
        layoutObj["height"] = layout->height.toInt(nullptr, 0);
        layoutObj["primary_tileset"] = layout->tileset_primary_label;
        layoutObj["secondary_tileset"] = layout->tileset_secondary_label;
        layoutObj["border_filepath"] = layout->border_path;
        layoutObj["blockdata_filepath"] = layout->blockdata_path;
        layoutsArr.append(layoutObj);
    }

    layoutsObj["layouts"] = layoutsArr;
    QJsonDocument layoutsDoc(layoutsObj);
    layoutsFile.write(layoutsDoc.toJson());
}

void Project::setNewMapLayout(Map* map) {
    MapLayout *layout = new MapLayout();
    layout->id = MapLayout::layoutConstantFromName(map->name);
    layout->name = QString("%1_Layout").arg(map->name);
    layout->width = "20";
    layout->height = "20";
    layout->border_path = QString("data/layouts/%1/border.bin").arg(map->name);
    layout->blockdata_path = QString("data/layouts/%1/map.bin").arg(map->name);
    layout->tileset_primary_label = "gTileset_General";
    layout->tileset_secondary_label = "gTileset_Petalburg";
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

    QJsonObject mapGroupsObj;
    mapGroupsObj["layouts_table_label"] = layoutsLabel;

    QJsonArray groupNamesArr;
    for (QString groupName : *this->groupNames) {
        groupNamesArr.append(groupName);
    }
    mapGroupsObj["group_order"] = groupNamesArr;

    int groupNum = 0;
    for (QStringList mapNames : groupedMapNames) {
        QJsonArray groupArr;
        for (QString mapName : mapNames) {
            groupArr.append(mapName);
        }
        mapGroupsObj[this->groupNames->at(groupNum)] = groupArr;
        groupNum++;
    }

    QJsonDocument mapGroupsDoc(mapGroupsObj);
    mapGroupsFile.write(mapGroupsDoc.toJson());
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
    saveTextFile(root + "/include/constants/map_groups.h", text);
}

// saves heal location coords in root + /src/data/heal_locations.h
// and indexes as defines in root + /include/constants/heal_locations.h
void Project::saveHealLocationStruct(Map *map) {
    QString tab = QString("    ");

    QString data_text = QString("static const struct HealLocation sHealLocations[] =\n{\n");

    QString constants_text = QString("#ifndef GUARD_CONSTANTS_HEAL_LOCATIONS_H\n");
    constants_text += QString("#define GUARD_CONSTANTS_HEAL_LOCATIONS_H\n\n");

    QMap<QString, int> flyableMapsDupes;
    QSet<QString> flyableMapsUnique;

    // set flyableMapsDupes and flyableMapsUnique
    for (auto it = flyableMaps.begin(); it != flyableMaps.end(); it++) {
        HealLocation loc = *it;
        QString xname = loc.name;
        if (flyableMapsUnique.contains(xname)) {
            flyableMapsDupes[xname] = 1;
        }
        flyableMapsUnique.insert(xname);
    }

    // set new location in flyableMapsList
    if (map->events["heal_event_group"].length() > 0) {
        for (Event *healEvent : map->events["heal_event_group"]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            flyableMaps[hl.index - 1] = hl;
        }
    }

    int i = 1;

    for (auto map_in : flyableMaps) {
        data_text += QString("    {MAP_GROUP(%1), MAP_NUM(%1), %2, %3},\n")
                     .arg(map_in.name)
                     .arg(map_in.x)
                     .arg(map_in.y);

        QString ending = QString("");

        // must add _1 / _2 for maps that have duplicates
        if (flyableMapsDupes.keys().contains(map_in.name)) {
            // map contains multiple heal locations
            ending += QString("_%1").arg(flyableMapsDupes[map_in.name]);
            flyableMapsDupes[map_in.name]++;
        }
        if (map_in.index != 0) {
            constants_text += QString("#define HEAL_LOCATION_%1 %2\n")
                              .arg(map_in.name + ending)
                              .arg(map_in.index);
        }
        else {
            constants_text += QString("#define HEAL_LOCATION_%1 %2\n")
                              .arg(map_in.name + ending)
                              .arg(i);
        }
        i++;
    }

    data_text += QString("};\n");
    constants_text += QString("\n#endif // GUARD_CONSTANTS_HEAL_LOCATIONS_H\n");

    saveTextFile(root + "/src/data/heal_locations.h", data_text);
    saveTextFile(root + "/include/constants/heal_locations.h", constants_text);
}

void Project::saveTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    saveTilesetMetatileAttributes(primaryTileset);
    saveTilesetMetatileAttributes(secondaryTileset);
    saveTilesetMetatiles(primaryTileset);
    saveTilesetMetatiles(secondaryTileset);
    saveTilesetTilesImage(primaryTileset);
    saveTilesetTilesImage(secondaryTileset);
    saveTilesetPalettes(primaryTileset, true);
    saveTilesetPalettes(secondaryTileset, false);
}

void Project::saveTilesetMetatileAttributes(Tileset *tileset) {
    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray data;
        for (Metatile *metatile : *tileset->metatiles) {
            data.append(static_cast<char>(metatile->behavior));
            data.append(static_cast<char>((metatile->layerType << 4) & 0xF0));
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
    tileset->tilesImage.save(tileset->tilesImagePath);
}

void Project::saveTilesetPalettes(Tileset *tileset, bool primary) {
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        QString filepath = tileset->palettePaths.at(i);
        QString content = "JASC-PAL\r\n";
        content += "0100\r\n";
        content += "16\r\n";
        for (int j = 0; j < 16; j++) {
            QRgb color = tileset->palettes->at(i).at(j);
            content += QString("%1 %2 %3\r\n")
                    .arg(qRed(color))
                    .arg(qGreen(color))
                    .arg(qBlue(color));
        }

        saveTextFile(filepath, content);
    }
}

void Project::loadMapTilesets(Map* map) {
    if (map->layout->has_unsaved_changes) {
        return;
    }

    map->layout->tileset_primary = getTileset(map->layout->tileset_primary_label);
    map->layout->tileset_secondary = getTileset(map->layout->tileset_secondary_label);
}

Tileset* Project::loadTileset(QString label, Tileset *tileset) {
    ParseUtil *parser = new ParseUtil;

    QString headers_text = readTextFile(root + "/data/tilesets/headers.inc");
    if (headers_text.isNull()) {
        return nullptr;
    }
    QStringList *values = getLabelValues(parser->parseAsm(headers_text), label);
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
    tileset->metatile_attrs_label = values->value(6);
    tileset->callback_label = values->value(7);

    loadTilesetAssets(tileset);

    tileset_cache->insert(label, tileset);
    return tileset;
}

void Project::loadBlockdata(Map* map) {
    if (!map->isPersistedToFile || map->layout->has_unsaved_changes) {
        return;
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
}

void Project::setNewMapBlockdata(Map* map) {
    Blockdata *blockdata = new Blockdata;
    for (int i = 0; i < map->getWidth() * map->getHeight(); i++) {
        blockdata->addBlock(qint16(0x3001));
    }
    map->layout->blockdata = blockdata;
}

void Project::loadMapBorder(Map *map) {
    if (!map->isPersistedToFile || map->layout->has_unsaved_changes) {
        return;
    }

    QString path = QString("%1/%2").arg(root).arg(map->layout->border_path);
    map->layout->border = readBlockdata(path);
}

void Project::setNewMapBorder(Map *map) {
    Blockdata *blockdata = new Blockdata;
    blockdata->addBlock(qint16(0x01D4));
    blockdata->addBlock(qint16(0x01D5));
    blockdata->addBlock(qint16(0x01DC));
    blockdata->addBlock(qint16(0x01DD));
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
    QList<QString> keys = map_cache->keys();
    for (int i = 0; i < keys.length(); i++) {
        QString key = keys.value(i);
        Map* map = map_cache->value(key);
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
        QString text = QString("%1_MapScripts::\n\t.byte 0\n").arg(map->name);
        saveTextFile(root + "/data/maps/" + map->name + "/scripts.inc", text);

        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
            // Create file data/maps/<map_name>/text.inc
            saveTextFile(root + "/data/maps/" + map->name + "/text.inc", "\n");
        }

        // Simply append to data/event_scripts.s.
        text = QString("\n\t.include \"data/maps/%1/scripts.inc\"\n").arg(map->name);
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
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

    // Append to "layouts" array in data/layouts/layouts.json.
    QString layoutsFilepath = QString("%1/data/layouts/layouts.json").arg(root);
    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::ReadWrite)) {
        logError(QString("Error: Could not open %1 for read/write").arg(layoutsFilepath));
        return;
    }
    QByteArray layoutsData = layoutsFile.readAll();
    QJsonDocument layoutsDoc = QJsonDocument::fromJson(layoutsData);
    QJsonObject layoutsObj = layoutsDoc.object();
    QJsonArray layoutsArr = layoutsObj["layouts"].toArray();
    QJsonObject newLayoutObj;
    newLayoutObj["id"] = map->layout->id;
    newLayoutObj["name"] = map->layout->name;
    newLayoutObj["width"] = map->layout->width.toInt();
    newLayoutObj["height"] = map->layout->height.toInt();
    newLayoutObj["primary_tileset"] = map->layout->tileset_primary_label;
    newLayoutObj["secondary_tileset"] = map->layout->tileset_secondary_label;
    newLayoutObj["border_filepath"] = map->layout->border_path;
    newLayoutObj["blockdata_filepath"] = map->layout->blockdata_path;
    layoutsArr.append(newLayoutObj);
    layoutsFile.write(layoutsDoc.toJson());

    // Create map.json for map data.
    QString mapFilepath = QString("%1/map.json").arg(mapDataDir);
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(mapFilepath));
        return;
    }

    QJsonObject mapObj;
    // Header values.
    mapObj["id"] = map->constantName;
    mapObj["name"] = map->name;
    mapObj["layout"] = map->layout->id;
    mapObj["music"] = map->song;
    mapObj["region_map_section"] = map->location;
    mapObj["requires_flash"] = map->requiresFlash.toInt() > 0 || map->requiresFlash == "TRUE";
    mapObj["weather"] = map->weather;
    mapObj["map_type"] = map->type;
    mapObj["allow_bike"] = map->allowBiking.toInt() > 0 || map->allowBiking == "TRUE";
    mapObj["allow_escape_rope"] = map->allowEscapeRope.toInt() > 0 || map->allowEscapeRope == "TRUE";
    mapObj["allow_running"] = map->allowRunning.toInt() > 0 || map->allowRunning == "TRUE";
    mapObj["show_map_name"] = map->show_location.toInt() > 0 || map->show_location == "TRUE";
    mapObj["battle_scene"] = map->battle_scene;

    // Connections
    if (map->connections.length() > 0) {
        QJsonArray connectionsArr;
        for (MapConnection* connection : map->connections) {
            if (mapNamesToMapConstants->contains(connection->map_name)) {
                QJsonObject connectionObj;
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
        QJsonArray objectEventsArr;
        for (int i = 0; i < map->events["object_event_group"].length(); i++) {
            Event *object_event = map->events["object_event_group"].value(i);
            QJsonObject eventObj = object_event->buildObjectEventJSON();
            objectEventsArr.append(eventObj);
        }
        mapObj["object_events"] = objectEventsArr;

        // Warp events
        QJsonArray warpEventsArr;
        for (int i = 0; i < map->events["warp_event_group"].length(); i++) {
            Event *warp_event = map->events["warp_event_group"].value(i);
            QJsonObject warpObj = warp_event->buildWarpEventJSON(mapNamesToMapConstants);
            warpEventsArr.append(warpObj);
        }
        mapObj["warp_events"] = warpEventsArr;

        // Coord events
        QJsonArray coordEventsArr;
        for (int i = 0; i < map->events["coord_event_group"].length(); i++) {
            Event *event = map->events["coord_event_group"].value(i);
            QString event_type = event->get("event_type");
            if (event_type == EventType::Trigger) {
                QJsonObject triggerObj = event->buildTriggerEventJSON();
                coordEventsArr.append(triggerObj);
            } else if (event_type == EventType::WeatherTrigger) {
                QJsonObject weatherObj = event->buildWeatherTriggerEventJSON();
                coordEventsArr.append(weatherObj);
            }
        }
        mapObj["coord_events"] = coordEventsArr;

        // Bg Events
        QJsonArray bgEventsArr;
        for (int i = 0; i < map->events["bg_event_group"].length(); i++) {
            Event *event = map->events["bg_event_group"].value(i);
            QString event_type = event->get("event_type");
            if (event_type == EventType::Sign) {
                QJsonObject signObj = event->buildSignEventJSON();
                bgEventsArr.append(signObj);
            } else if (event_type == EventType::HiddenItem) {
                QJsonObject hiddenItemObj = event->buildHiddenItemEventJSON();
                bgEventsArr.append(hiddenItemObj);
            } else if (event_type == EventType::SecretBase) {
                QJsonObject secretBaseObj = event->buildSecretBaseEventJSON();
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

    QJsonDocument mapDoc(mapObj);
    mapFile.write(mapDoc.toJson());

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
}

void Project::loadTilesetAssets(Tileset* tileset) {
    ParseUtil* parser = new ParseUtil;
    QString category = (tileset->is_secondary == "TRUE") ? "secondary" : "primary";
    if (tileset->name.isNull()) {
        return;
    }
    QString tilesetName = tileset->name;
    QString dir_path = root + "/data/tilesets/" + category + "/" + tilesetName.replace("gTileset_", "").toLower();

    QString graphics_text = readTextFile(root + "/data/tilesets/graphics.inc");
    QList<QStringList> *graphics = parser->parseAsm(graphics_text);
    QStringList *tiles_values = getLabelValues(graphics, tileset->tiles_label);
    QStringList *palettes_values = getLabelValues(graphics, tileset->palettes_label);

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

    QString metatiles_text = readTextFile(root + "/data/tilesets/metatiles.inc");
    QList<QStringList> *metatiles_macros = parser->parseAsm(metatiles_text);
    QStringList *metatiles_values = getLabelValues(metatiles_macros, tileset->metatiles_label);
    if (!metatiles_values->isEmpty()) {
        tileset->metatiles_path = root + "/" + metatiles_values->value(0).section('"', 1, 1);
    } else {
        tileset->metatiles_path = dir_path + "/metatiles.bin";
    }
    QStringList *metatile_attrs_values = getLabelValues(metatiles_macros, tileset->metatile_attrs_label);
    if (!metatile_attrs_values->isEmpty()) {
        tileset->metatile_attrs_path = root + "/" + metatile_attrs_values->value(0).section('"', 1, 1);
    } else {
        tileset->metatile_attrs_path = dir_path + "/metatile_attributes.bin";
    }

    tiles_path = fixGraphicPath(tiles_path);
    tileset->tilesImagePath = tiles_path;
    QImage image = QImage(tileset->tilesImagePath);
    this->loadTilesetTiles(tileset, image);
    this->loadTilesetMetatiles(tileset);

    // palettes
    QList<QList<QRgb>> *palettes = new QList<QList<QRgb>>;
    for (int i = 0; i < tileset->palettePaths.length(); i++) {
        QList<QRgb> palette;
        QString path = tileset->palettePaths.value(i);
        QString text = readTextFile(path);
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
    }
    tileset->palettes = palettes;
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
        }
    } else {
        logError(QString("Could not open tileset metatile attributes file '%1'").arg(tileset->metatile_attrs_path));
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
    if (map_cache->contains(map_name)) {
        return map_cache->value(map_name);
    } else {
        Map *map = loadMap(map_name);
        return map;
    }
}

Tileset* Project::getTileset(QString label, bool forceLoad) {
    Tileset *existingTileset = nullptr;
    if (tileset_cache->contains(label)) {
        existingTileset = tileset_cache->value(label);
    }

    if (existingTileset && !forceLoad) {
        return tileset_cache->value(label);
    } else {
        Tileset *tileset = loadTileset(label, existingTileset);
        return tileset;
    }
}

QString Project::readTextFile(QString path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1': ").arg(path) + file.errorString());
        return QString();
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString text = "";
    while (!in.atEnd()) {
        text += in.readLine() + "\n";
    }
    return text;
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

void Project::readMapGroups() {
    QString mapGroupsFilepath = QString("%1/data/maps/map_groups.json").arg(root);
    QFile mapGroupsFile(mapGroupsFilepath);
    if (!mapGroupsFile.open(QIODevice::ReadOnly)) {
        logError(QString("Error: Could not open %1 for reading").arg(mapGroupsFilepath));
        return;
    }

    QByteArray mapGroupsData = mapGroupsFile.readAll();
    QJsonDocument mapGroupsDoc = QJsonDocument::fromJson(mapGroupsData);
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
            map_groups->insert(mapName, groupIndex);
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

    QString hltext = readTextFile(root + QString("/src/data/heal_locations.h"));
    QList<HealLocation>* hl = ParseUtil().parseHealLocs(hltext);
    flyableMaps = *hl;
    delete hl;
}

Map* Project::addNewMapToGroup(QString mapName, int groupNum) {
    // Setup new map in memory, but don't write to file until map is actually saved later.
    mapNames->append(mapName);
    map_groups->insert(mapName, groupNum);
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
    map_cache->insert(mapName, map);

    return map;
}

Map* Project::addNewMapToGroup(QString mapName, int groupNum, Map *newMap, bool existingLayout) {
    mapNames->append(mapName);
    map_groups->insert(mapName, groupNum);
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

QList<QStringList>* Project::parseAsm(QString text) {
    ParseUtil *parser = new ParseUtil;
    return parser->parseAsm(text);
}

QStringList Project::getVisibilities() {
    // TODO
    QStringList names;
    for (int i = 0; i < 16; i++) {
        names.append(QString("%1").arg(i));
    }
    return names;
}

QMap<QString, QStringList> Project::getTilesetLabels() {
    QMap<QString, QStringList> allTilesets;
    QStringList primaryTilesets;
    QStringList secondaryTilesets;
    allTilesets.insert("primary", primaryTilesets);
    allTilesets.insert("secondary", secondaryTilesets);
    QString headers_text = readTextFile(root + "/data/tilesets/headers.inc");
    QList<QStringList>* commands = parseAsm(headers_text);
    int i = 0;
    while (i < commands->length()) {
        if (commands->at(i).length() != 2)
            continue;

        if (commands->at(i).at(0) == ".label") {
            QString tilesetLabel = commands->at(i).at(1);
            // Advance to command specifying whether or not it is a secondary tileset
            i += 2;
            if (commands->at(i).at(0) != ".byte") {
                logWarn(QString("Unexpected command found for secondary tileset flag in tileset '%1'. Expected '.byte', but found: '%2'").arg(tilesetLabel).arg(commands->at(i).at(0)));
                continue;
            }

            QString secondaryTilesetValue = commands->at(i).at(1);
            if (secondaryTilesetValue != "TRUE" && secondaryTilesetValue != "FALSE" && secondaryTilesetValue != "0" && secondaryTilesetValue != "1") {
                logWarn(QString("Unexpected secondary tileset flag found. Expected \"TRUE\", \"FALSE\", \"0\", or \"1\", but found: '%1'").arg(secondaryTilesetValue));
                continue;
            }

            bool isSecondaryTileset = (secondaryTilesetValue == "TRUE" || secondaryTilesetValue == "1");
            if (isSecondaryTileset)
                allTilesets["secondary"].append(tilesetLabel);
            else
                allTilesets["primary"].append(tilesetLabel);
        }

        i++;
    }

    return allTilesets;
}

void Project::readTilesetProperties() {
    QStringList defines;
    QString text = readTextFile(root + "/include/fieldmap.h");
    if (!text.isNull()) {
        bool error = false;
        QStringList definePrefixes;
        definePrefixes << "NUM_";
        QMap<QString, int> defines = readCDefines(text, definePrefixes);
        auto it = defines.find("NUM_TILES_IN_PRIMARY");
        if (it != defines.end()) {
            Project::num_tiles_primary = it.value();
        }
        else {
            error = true;
        }
        it = defines.find("NUM_TILES_TOTAL");
        if (it != defines.end()) {
            Project::num_tiles_total = it.value();
        }
        else {
            error = true;
        }
        it = defines.find("NUM_METATILES_IN_PRIMARY");
        if (it != defines.end()) {
            Project::num_metatiles_primary = it.value();
        }
        else {
            error = true;
        }
        it = defines.find("NUM_METATILES_TOTAL");
        if (it != defines.end()) {
            Project::num_metatiles_total = it.value();
        }
        else {
            error = true;
        }
        it = defines.find("NUM_PALS_IN_PRIMARY");
        if (it != defines.end()) {
            Project::num_pals_primary = it.value();
        }
        else {
            error = true;
        }
        it = defines.find("NUM_PALS_TOTAL");
        if (it != defines.end()) {
            Project::num_pals_total = it.value();
        }
        else {
            error = true;
        }

        if (error)
        {
            logError("Some global tileset values could not be loaded. Using default values instead.");
        }
    }
}

void Project::readRegionMapSections() {
    QString filepath = root + "/include/constants/region_map_sections.h";
    this->mapSectionNameToValue.clear();
    this->mapSectionValueToName.clear();
    QString text = readTextFile(filepath);
    if (!text.isNull()) {
        QStringList prefixes = (QStringList() << "MAPSEC_");
        this->mapSectionNameToValue = readCDefines(text, prefixes);
        for (QString defineName : this->mapSectionNameToValue.keys()) {
            this->mapSectionValueToName.insert(this->mapSectionNameToValue[defineName], defineName);
        }
    } else {
        logError(QString("Failed to read C defines file: '%1'").arg(filepath));
    }
}

void Project::readItemNames() {
    QString filepath = root + "/include/constants/items.h";
    QStringList prefixes = (QStringList() << "ITEM_");
    readCDefinesSorted(filepath, prefixes, itemNames);
}

void Project::readFlagNames() {
    QString filepath = root + "/include/constants/flags.h";
    QStringList prefixes = (QStringList() << "FLAG_");
    readCDefinesSorted(filepath, prefixes, flagNames);
}

void Project::readVarNames() {
    QString filepath = root + "/include/constants/vars.h";
    QStringList prefixes = (QStringList() << "VAR_");
    readCDefinesSorted(filepath, prefixes, varNames);
}

void Project::readMovementTypes() {
    QString filepath = root + "/include/constants/event_object_movement_constants.h";
    QStringList prefixes = (QStringList() << "MOVEMENT_TYPE_");
    readCDefinesSorted(filepath, prefixes, movementTypes);
}

void Project::readMapTypes() {
    QString filepath = root + "/include/constants/map_types.h";
    QStringList prefixes = (QStringList() << "MAP_TYPE_");
    readCDefinesSorted(filepath, prefixes, mapTypes);
}

void Project::readMapBattleScenes() {
    QString filepath = root + "/include/constants/map_types.h";
    QStringList prefixes = (QStringList() << "MAP_BATTLE_SCENE_");
    readCDefinesSorted(filepath, prefixes, mapBattleScenes);
}

void Project::readWeatherNames() {
    QString filepath = root + "/include/constants/weather.h";
    QStringList prefixes = (QStringList() << "WEATHER_");
    readCDefinesSorted(filepath, prefixes, weatherNames);
}

void Project::readCoordEventWeatherNames() {
    QString filepath = root + "/include/constants/weather.h";
    QStringList prefixes = (QStringList() << "COORD_EVENT_WEATHER_");
    readCDefinesSorted(filepath, prefixes, coordEventWeatherNames);
}

void Project::readSecretBaseIds() {
    QString filepath = root + "/include/constants/secret_bases.h";
    QStringList prefixes = (QStringList() << "SECRET_BASE_");
    readCDefinesSorted(filepath, prefixes, secretBaseIds);
}

void Project::readBgEventFacingDirections() {
    QString filepath = root + "/include/constants/bg_event_constants.h";
    QStringList prefixes = (QStringList() << "BG_EVENT_PLAYER_FACING_");
    readCDefinesSorted(filepath, prefixes, bgEventFacingDirections);
}

void Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();
    QString filepath = root + "/include/constants/metatile_behaviors.h";
    QString text = readTextFile(filepath);
    if (!text.isNull()) {
        QStringList prefixes = (QStringList() << "MB_");
        this->metatileBehaviorMap = readCDefines(text, prefixes);
        for (QString defineName : this->metatileBehaviorMap.keys()) {
            this->metatileBehaviorMapInverse.insert(this->metatileBehaviorMap[defineName], defineName);
        }
    } else {
        logError(QString("Failed to read C defines file: '%1'").arg(filepath));
    }
}

void Project::readCDefinesSorted(QString filepath, QStringList prefixes, QStringList* definesToSet) {
    QString text = readTextFile(filepath);
    if (!text.isNull()) {
        QMap<QString, int> defines = readCDefines(text, prefixes);

        // The defines should to be sorted by their underlying value, not alphabetically.
        // Reverse the map and read out the resulting keys in order.
        QMultiMap<int, QString> definesInverse;
        for (QString defineName : defines.keys()) {
            definesInverse.insert(defines[defineName], defineName);
        }
        *definesToSet = definesInverse.values();
    } else {
        logError(QString("Failed to read C defines file: '%1'").arg(filepath));
    }
}

QStringList Project::getSongNames() {
    QStringList names;
    QString text = readTextFile(root + "/include/constants/songs.h");
    if (!text.isNull()) {
        QStringList songDefinePrefixes;
        songDefinePrefixes << "SE_" << "MUS_";
        QMap<QString, int> songDefines = readCDefines(text, songDefinePrefixes);
        names = songDefines.keys();
    }
    return names;
}

QMap<QString, int> Project::getEventObjGfxConstants() {
    QMap<QString, int> constants;
    QString text = readTextFile(root + "/include/constants/event_objects.h");
    if (!text.isNull()) {
        QStringList eventObjGfxPrefixes;
        eventObjGfxPrefixes << "EVENT_OBJ_GFX_";
        constants = readCDefines(text, eventObjGfxPrefixes);
    }
    return constants;
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

    QString pointers_text = readTextFile(root + "/src/data/field_event_obj/event_object_graphics_info_pointers.h");
    QString info_text = readTextFile(root + "/src/data/field_event_obj/event_object_graphics_info.h");
    QString pic_text = readTextFile(root + "/src/data/field_event_obj/event_object_pic_tables.h");
    QString assets_text = readTextFile(root + "/src/data/field_event_obj/event_object_graphics.h");

    QStringList pointers = readCArray(pointers_text, "gEventObjectGraphicsInfoPointers");

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
            int sprite_id = constants.value(object->get("sprite"));

            QString info_label = pointers.value(sprite_id).replace("&", "");
            QStringList gfx_info = readCArray(info_text, info_label);
            QString pic_label = gfx_info.value(14);
            QString dimensions_label = gfx_info.value(11);
            QString subsprites_label = gfx_info.value(12);
            QString gfx_label = readCArray(pic_text, pic_label).value(0);
            gfx_label = gfx_label.section(QRegExp("[\\(\\)]"), 1, 1);
            QString path = readCIncbin(assets_text, gfx_label);

            if (!path.isNull()) {
                path = fixGraphicPath(path);
                QImage spritesheet(root + "/" + path);
                if (!spritesheet.isNull()) {
                    // Infer the sprite dimensions from the OAM labels.
                    int spriteWidth = spritesheet.width();
                    int spriteHeight = spritesheet.height();
                    QRegularExpression re("\\S+_(\\d+)x(\\d+)");
                    QRegularExpressionMatch dimensionMatch = re.match(dimensions_label);
                    if (dimensionMatch.hasMatch()) {
                        QRegularExpressionMatch oamTablesMatch = re.match(subsprites_label);
                        if (oamTablesMatch.hasMatch()) {
                            spriteWidth = dimensionMatch.captured(1).toInt();
                            spriteHeight = dimensionMatch.captured(2).toInt();
                        }
                    }
                    object->setPixmapFromSpritesheet(spritesheet, spriteWidth, spriteHeight);
                }
            }
        }
    }
}

void Project::saveMapHealEvents(Map *map) {
    // save heal event changes
    if (map->events["heal_event_group"].length() > 0) {
        for (Event *healEvent : map->events["heal_event_group"]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            flyableMaps[hl.index - 1] = hl;
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

QStringList Project::readCArray(QString text, QString label) {
    QStringList list;

    if (label.isNull()) {
        return list;
    }

    QRegExp *re = new QRegExp(QString("\\b%1\\b\\s*\\[?\\s*\\]?\\s*=\\s*\\{([^\\}]*)\\}").arg(label));
    int pos = re->indexIn(text);
    if (pos != -1) {
        QString body = re->cap(1);
        body = body.replace(QRegExp("\\s*"), "");
        list = body.split(',');
        /*
        QRegExp *inner = new QRegExp("&?\\b([A-Za-z0-9_\\(\\)]*)\\b,");
        int pos = 0;
        while ((pos = inner->indexIn(body, pos)) != -1) {
            list << inner->cap(1);
            pos += inner->matchedLength();
        }
        */
    }

    return list;
}

QString Project::readCIncbin(QString text, QString label) {
    QString path;

    if (label.isNull()) {
        return path;
    }

    QRegExp *re = new QRegExp(QString(
        "\\b%1\\b"
        "\\s*\\[?\\s*\\]?\\s*=\\s*"
        "INCBIN_[US][0-9][0-9]?"
        "\\(\"([^\"]*)\"\\)").arg(label));

    int pos = re->indexIn(text);
    if (pos != -1) {
        path = re->cap(1);
    }

    return path;
}

QMap<QString, int> Project::readCDefines(QString text, QStringList prefixes) {
    ParseUtil parser;
    text.replace(QRegularExpression("(//.*)|(\\/+\\*+[^*]*\\*+\\/+)"), "");
    QMap<QString, int> allDefines;
    QMap<QString, int> filteredDefines;
    QRegularExpression re("#define\\s+(?<defineName>\\w+)[^\\S\\n]+(?<defineValue>.+)");
    QRegularExpressionMatchIterator iter = re.globalMatch(text);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString name = match.captured("defineName");
        QString expression = match.captured("defineValue");
        if (expression == " ") continue;
        int value = parser.evaluateDefine(expression, &allDefines);
        allDefines.insert(name, value);
        for (QString prefix : prefixes) {
            if (name.startsWith(prefix)) {
                filteredDefines.insert(name, value);
            }
        }
    }
    return filteredDefines;
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
