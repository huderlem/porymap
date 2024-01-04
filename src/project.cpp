#include "project.h"
#include "config.h"
#include "history.h"
#include "log.h"
#include "parseutil.h"
#include "paletteutil.h"
#include "tile.h"
#include "tileset.h"
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
        notice.setDefaultButton(QMessageBox::No);
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
    this->importExportPath = dir;
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

const QSet<QString> defaultTopLevelMapFields = {
    "id",
    "name",
    "layout",
    "music",
    "region_map_section",
    "requires_flash",
    "weather",
    "map_type",
    "show_map_name",
    "battle_scene",
    "connections",
    "object_events",
    "warp_events",
    "coord_events",
    "bg_events",
    "shared_events_map",
    "shared_scripts_map",
};

QSet<QString> Project::getTopLevelMapFields() {
    QSet<QString> topLevelMapFields = defaultTopLevelMapFields;
    if (projectConfig.getMapAllowFlagsEnabled()) {
        topLevelMapFields.insert("allow_cycling");
        topLevelMapFields.insert("allow_escaping");
        topLevelMapFields.insert("allow_running");
    }

    if (projectConfig.getFloorNumberEnabled()) {
        topLevelMapFields.insert("floor_number");
    }
    return topLevelMapFields;
}

bool Project::loadMapData(Map* map) {
    if (!map->isPersistedToFile) {
        return true;
    }

    QString mapFilepath = QString("%1/%3%2/map.json").arg(root).arg(map->name).arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders));
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map data from %1").arg(mapFilepath));
        return false;
    }

    QJsonObject mapObj = mapDoc.object();

    map->song          = ParseUtil::jsonToQString(mapObj["music"]);
    map->layoutId      = ParseUtil::jsonToQString(mapObj["layout"]);
    map->location      = ParseUtil::jsonToQString(mapObj["region_map_section"]);
    map->requiresFlash = ParseUtil::jsonToBool(mapObj["requires_flash"]);
    map->weather       = ParseUtil::jsonToQString(mapObj["weather"]);
    map->type          = ParseUtil::jsonToQString(mapObj["map_type"]);
    map->show_location = ParseUtil::jsonToBool(mapObj["show_map_name"]);
    map->battle_scene  = ParseUtil::jsonToQString(mapObj["battle_scene"]);

    if (projectConfig.getMapAllowFlagsEnabled()) {
        map->allowBiking   = ParseUtil::jsonToBool(mapObj["allow_cycling"]);
        map->allowEscaping = ParseUtil::jsonToBool(mapObj["allow_escaping"]);
        map->allowRunning  = ParseUtil::jsonToBool(mapObj["allow_running"]);
    }
    if (projectConfig.getFloorNumberEnabled()) {
        map->floorNumber = ParseUtil::jsonToInt(mapObj["floor_number"]);
    }
    map->sharedEventsMap  = ParseUtil::jsonToQString(mapObj["shared_events_map"]);
    map->sharedScriptsMap = ParseUtil::jsonToQString(mapObj["shared_scripts_map"]);

    // Events
    map->events[Event::Group::Object].clear();
    QJsonArray objectEventsArr = mapObj["object_events"].toArray();
    bool hasCloneObjects = projectConfig.getEventCloneObjectEnabled();
    for (int i = 0; i < objectEventsArr.size(); i++) {
        QJsonObject event = objectEventsArr[i].toObject();
        // If clone objects are not enabled then no type field is present
        QString type = hasCloneObjects ? ParseUtil::jsonToQString(event["type"]) : "object";
        if (type.isEmpty() || type == "object") {
            ObjectEvent *object = new ObjectEvent();
            object->loadFromJson(event, this);
            map->addEvent(object);
        } else if (type == "clone") {
            CloneObjectEvent *clone = new CloneObjectEvent();
            if (clone->loadFromJson(event, this)) {
                map->addEvent(clone);
            }
            else {
                delete clone;
            }
        } else {
            logError(QString("Map %1 object_event %2 has invalid type '%3'. Must be 'object' or 'clone'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events[Event::Group::Warp].clear();
    QJsonArray warpEventsArr = mapObj["warp_events"].toArray();
    for (int i = 0; i < warpEventsArr.size(); i++) {
        QJsonObject event = warpEventsArr[i].toObject();
        WarpEvent *warp = new WarpEvent();
        if (warp->loadFromJson(event, this)) {
            map->addEvent(warp);
        }
        else {
            delete warp;
        }
    }

    map->events[Event::Group::Coord].clear();
    QJsonArray coordEventsArr = mapObj["coord_events"].toArray();
    for (int i = 0; i < coordEventsArr.size(); i++) {
        QJsonObject event = coordEventsArr[i].toObject();
        QString type = ParseUtil::jsonToQString(event["type"]);
        if (type == "trigger") {
            TriggerEvent *coord = new TriggerEvent();
            coord->loadFromJson(event, this);
            map->addEvent(coord);
        } else if (type == "weather") {
            WeatherTriggerEvent *coord = new WeatherTriggerEvent();
            coord->loadFromJson(event, this);
            map->addEvent(coord);
        } else {
            logError(QString("Map %1 coord_event %2 has invalid type '%3'. Must be 'trigger' or 'weather'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events[Event::Group::Bg].clear();
    QJsonArray bgEventsArr = mapObj["bg_events"].toArray();
    for (int i = 0; i < bgEventsArr.size(); i++) {
        QJsonObject event = bgEventsArr[i].toObject();
        QString type = ParseUtil::jsonToQString(event["type"]);
        if (type == "sign") {
            SignEvent *bg = new SignEvent();
            bg->loadFromJson(event, this);
            map->addEvent(bg);
        } else if (type == "hidden_item") {
            HiddenItemEvent *bg = new HiddenItemEvent();
            bg->loadFromJson(event, this);
            map->addEvent(bg);
        } else if (type == "secret_base") {
            SecretBaseEvent *bg = new SecretBaseEvent();
            bg->loadFromJson(event, this);
            map->addEvent(bg);
        } else {
            logError(QString("Map %1 bg_event %2 has invalid type '%3'. Must be 'sign', 'hidden_item', or 'secret_base'.").arg(map->name).arg(i).arg(type));
        }
    }

    map->events[Event::Group::Heal].clear();
    
    const QString mapPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
    for (auto it = healLocations.begin(); it != healLocations.end(); it++) {
        HealLocation loc = *it;
        //if TRUE map is flyable / has healing location
        if (loc.mapName == Map::mapConstantFromName(map->name, false)) {
            HealLocationEvent *heal = new HealLocationEvent();
            heal->setMap(map);
            heal->setX(loc.x);
            heal->setY(loc.y);
            heal->setElevation(projectConfig.getDefaultElevation());
            heal->setLocationName(loc.mapName);
            heal->setIdName(loc.idName);
            heal->setIndex(loc.index);
            if (projectConfig.getHealLocationRespawnDataEnabled()) {
                heal->setRespawnMap(mapConstantsToMapNames.value(QString(mapPrefix + loc.respawnMap)));
                heal->setRespawnNPC(loc.respawnNPC);
            }
            map->events[Event::Group::Heal].append(heal);
        }
    }

    map->connections.clear();
    QJsonArray connectionsArr = mapObj["connections"].toArray();
    if (!connectionsArr.isEmpty()) {
        for (int i = 0; i < connectionsArr.size(); i++) {
            QJsonObject connectionObj = connectionsArr[i].toObject();
            MapConnection *connection = new MapConnection;
            connection->direction = ParseUtil::jsonToQString(connectionObj["direction"]);
            connection->offset    = ParseUtil::jsonToInt(connectionObj["offset"]);
            QString mapConstant   = ParseUtil::jsonToQString(connectionObj["map"]);
            if (mapConstantsToMapNames.contains(mapConstant)) {
                connection->map_name = mapConstantsToMapNames.value(mapConstant);
                map->connections.append(connection);
            } else {
                logError(QString("Failed to find connected map for map constant '%1'").arg(mapConstant));
            }
        }
    }

    // Check for custom fields
    QSet<QString> baseFields = this->getTopLevelMapFields();
    for (QString key : mapObj.keys()) {
        if (!baseFields.contains(key)) {
            map->customHeaders.insert(key, mapObj[key]);
        }
    }

    return true;
}

QString Project::readMapLayoutId(QString map_name) {
    if (mapCache.contains(map_name)) {
        return mapCache.value(map_name)->layoutId;
    }

    QString mapFilepath = QString("%1/%3%2/map.json").arg(root).arg(map_name).arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders));
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map layout id from %1").arg(mapFilepath));
        return QString();
    }

    QJsonObject mapObj = mapDoc.object();
    return ParseUtil::jsonToQString(mapObj["layout"]);
}

QString Project::readMapLocation(QString map_name) {
    if (mapCache.contains(map_name)) {
        return mapCache.value(map_name)->location;
    }

    QString mapFilepath = QString("%1/%3%2/map.json").arg(root).arg(map_name).arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders));
    QJsonDocument mapDoc;
    if (!parser.tryParseJsonFile(&mapDoc, mapFilepath)) {
        logError(QString("Failed to read map's region map section from %1").arg(mapFilepath));
        return QString();
    }

    QJsonObject mapObj = mapDoc.object();
    return ParseUtil::jsonToQString(mapObj["region_map_section"]);
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

    QString layoutsFilepath = projectConfig.getFilePath(ProjectFilePath::json_layouts);
    QString fullFilepath = QString("%1/%2").arg(root).arg(layoutsFilepath);
    fileWatcher.addPath(fullFilepath);
    QJsonDocument layoutsDoc;
    if (!parser.tryParseJsonFile(&layoutsDoc, fullFilepath)) {
        logError(QString("Failed to read map layouts from %1").arg(fullFilepath));
        return false;
    }

    QJsonObject layoutsObj = layoutsDoc.object();
    QJsonArray layouts = layoutsObj["layouts"].toArray();
    if (layouts.size() == 0) {
        logError(QString("'layouts' array is missing from %1.").arg(layoutsFilepath));
        return false;
    }

    layoutsLabel = ParseUtil::jsonToQString(layoutsObj["layouts_table_label"]);
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
    for (int i = 0; i < layouts.size(); i++) {
        QJsonObject layoutObj = layouts[i].toObject();
        if (layoutObj.isEmpty())
            continue;
        if (!parser.ensureFieldsExist(layoutObj, requiredFields)) {
            logError(QString("Layout %1 is missing field(s) in %2.").arg(i).arg(layoutsFilepath));
            return false;
        }
        MapLayout *layout = new MapLayout();
        layout->id = ParseUtil::jsonToQString(layoutObj["id"]);
        if (layout->id.isEmpty()) {
            logError(QString("Missing 'id' value on layout %1 in %2").arg(i).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        if (mapLayouts.contains(layout->id)) {
            logWarn(QString("Duplicate layout entry for %1 in %2 will be ignored.").arg(layout->id).arg(layoutsFilepath));
            delete layout;
            continue;
        }
        layout->name = ParseUtil::jsonToQString(layoutObj["name"]);
        if (layout->name.isEmpty()) {
            logError(QString("Missing 'name' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        int lwidth = ParseUtil::jsonToInt(layoutObj["width"]);
        if (lwidth <= 0) {
            logError(QString("Invalid 'width' value '%1' for %2 in %3. Must be greater than 0.").arg(lwidth).arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        layout->width = lwidth;
        int lheight = ParseUtil::jsonToInt(layoutObj["height"]);
        if (lheight <= 0) {
            logError(QString("Invalid 'height' value '%1' for %2 in %3. Must be greater than 0.").arg(lheight).arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        layout->height = lheight;
        if (useCustomBorderSize) {
            int bwidth = ParseUtil::jsonToInt(layoutObj["border_width"]);
            if (bwidth <= 0) {  // 0 is an expected border width/height that should be handled, GF used it for the RS layouts in FRLG
                logWarn(QString("Invalid 'border_width' value '%1' for %2 in %3. Must be greater than 0. Using default (%4) instead.")
                                .arg(bwidth).arg(layout->id).arg(layoutsFilepath).arg(DEFAULT_BORDER_WIDTH));
                bwidth = DEFAULT_BORDER_WIDTH;
            }
            layout->border_width = bwidth;
            int bheight = ParseUtil::jsonToInt(layoutObj["border_height"]);
            if (bheight <= 0) {
                logWarn(QString("Invalid 'border_height' value '%1' for %2 in %3. Must be greater than 0. Using default (%4) instead.")
                                .arg(bheight).arg(layout->id).arg(layoutsFilepath).arg(DEFAULT_BORDER_HEIGHT));
                bheight = DEFAULT_BORDER_HEIGHT;
            }
            layout->border_height = bheight;
        } else {
            layout->border_width = DEFAULT_BORDER_WIDTH;
            layout->border_height = DEFAULT_BORDER_HEIGHT;
        }
        layout->tileset_primary_label = ParseUtil::jsonToQString(layoutObj["primary_tileset"]);
        if (layout->tileset_primary_label.isEmpty()) {
            logError(QString("Missing 'primary_tileset' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        layout->tileset_secondary_label = ParseUtil::jsonToQString(layoutObj["secondary_tileset"]);
        if (layout->tileset_secondary_label.isEmpty()) {
            logError(QString("Missing 'secondary_tileset' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        layout->border_path = ParseUtil::jsonToQString(layoutObj["border_filepath"]);
        if (layout->border_path.isEmpty()) {
            logError(QString("Missing 'border_filepath' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            delete layout;
            return false;
        }
        layout->blockdata_path = ParseUtil::jsonToQString(layoutObj["blockdata_filepath"]);
        if (layout->blockdata_path.isEmpty()) {
            logError(QString("Missing 'blockdata_filepath' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            delete layout;
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
    QString layoutsFilepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::json_layouts);
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
        layoutObj["width"] = layout->width;
        layoutObj["height"] = layout->height;
        if (useCustomBorderSize) {
            layoutObj["border_width"] = layout->border_width;
            layoutObj["border_height"] = layout->border_height;
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

void Project::saveMapGroups() {
    QString mapGroupsFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_map_groups));
    QFile mapGroupsFile(mapGroupsFilepath);
    if (!mapGroupsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(mapGroupsFilepath));
        return;
    }

    OrderedJson::object mapGroupsObj;

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
    if (!userConfig.getEncounterJsonActive()) return;

    QString wildEncountersJsonFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_wild_encounters));
    QFile wildEncountersFile(wildEncountersJsonFilepath);
    if (!wildEncountersFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(wildEncountersJsonFilepath));
        return;
    }

    OrderedJson::object wildEncountersObject;
    OrderedJson::array wildEncounterGroups;

    OrderedJson::object monHeadersObject;
    monHeadersObject["label"] = projectConfig.getIdentifier(ProjectIdentifier::symbol_wild_encounters);
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
    text += QString("\n//\n// DO NOT MODIFY THIS FILE! It is auto-generated from %1\n//\n\n")
                    .arg(projectConfig.getFilePath(ProjectFilePath::json_map_groups));

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

    QString mapGroupFilepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::constants_map_groups);
    ignoreWatchedFileTemporarily(mapGroupFilepath);
    saveTextFile(mapGroupFilepath, text);
}

void Project::saveHealLocations(Map *map) {
    this->saveHealLocationsData(map);
    this->saveHealLocationsConstants();
}

// Saves heal location maps/coords/respawn data in root + /src/data/heal_locations.h
void Project::saveHealLocationsData(Map *map) {
    // Update heal locations from map
    if (map->events[Event::Group::Heal].length() > 0) {
        for (Event *healEvent : map->events[Event::Group::Heal]) {
            HealLocation hl = HealLocation::fromEvent(healEvent);
            this->healLocations[hl.index - 1] = hl;
        }
    }

    // Find any duplicate constant names
    QMap<QString, int> healLocationsDupes;
    QSet<QString> healLocationsUnique;
    for (auto hl : this->healLocations) {
        QString idName = hl.idName;
        if (healLocationsUnique.contains(idName))
            healLocationsDupes[idName] = 1;
        else
            healLocationsUnique.insert(idName);
    }

    // Create the definition text for each data table
    bool respawnEnabled = projectConfig.getHealLocationRespawnDataEnabled();
    const QString qualifiers = QString(healLocationDataQualifiers.isStatic ? "static " : "")
                             + QString(healLocationDataQualifiers.isConst ? "const " : "");

    QString locationTableText = QString("%1%2 %3[] =\n{\n").arg(qualifiers)
                                                           .arg(projectConfig.getIdentifier(ProjectIdentifier::symbol_heal_locations_type))
                                                           .arg(this->healLocationsTableName);
    QString respawnMapTableText, respawnNPCTableText;
    if (respawnEnabled) {
        respawnMapTableText = QString("\n%1%2[][2] =\n{\n").arg(qualifiers).arg(projectConfig.getIdentifier(ProjectIdentifier::symbol_spawn_maps));
        respawnNPCTableText = QString("\n%1%2[] =\n{\n").arg(qualifiers).arg(projectConfig.getIdentifier(ProjectIdentifier::symbol_spawn_npcs));
    }

    // Populate the data tables with the heal location data
    int i = 0;
    const QString emptyMapName = projectConfig.getIdentifier(ProjectIdentifier::define_map_empty);
    for (auto hl : this->healLocations) {
        // Add numbered suffix for duplicate constants
        if (healLocationsDupes.keys().contains(hl.idName)) {
            QString duplicateName = hl.idName;
            hl.idName += QString("_%1").arg(healLocationsDupes[duplicateName]);
            healLocationsDupes[duplicateName]++;
            this->healLocations[i].idName = hl.idName; // Update the name for writing constants later
        }

        // Add entry to map/coords table
        QString mapName = !hl.mapName.isEmpty() ? hl.mapName : emptyMapName;
        locationTableText += QString("    [%1 - 1] = {MAP_GROUP(%2), MAP_NUM(%2), %3, %4},\n")
                             .arg(hl.idName)
                             .arg(mapName)
                             .arg(hl.x)
                             .arg(hl.y);

        // Add entry to respawn map and npc tables
        if (respawnEnabled) {
            mapName = !hl.respawnMap.isEmpty() ? hl.respawnMap : emptyMapName;
            respawnMapTableText += QString("    [%1 - 1] = {MAP_GROUP(%2), MAP_NUM(%2)},\n")
                                   .arg(hl.idName)
                                   .arg(mapName);

            respawnNPCTableText += QString("    [%1 - 1] = %2,\n")
                                   .arg(hl.idName)
                                   .arg(hl.respawnNPC);
        }
        i++;
    }
    const QString tableEnd = QString("};\n");
    QString text = locationTableText + tableEnd;
    if (respawnEnabled)
        text += respawnMapTableText + tableEnd + respawnNPCTableText + tableEnd;

    QString filepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::data_heal_locations);
    ignoreWatchedFileTemporarily(filepath);
    saveTextFile(filepath, text);
}

// Saves heal location defines in root + /include/constants/heal_locations.h
void Project::saveHealLocationsConstants() {
    // Get existing defines, and create an inverted map so they'll be in sorted order for printing
    int nextDefineValue = 1;
    QMap<int, QString> valuesToNames = QMap<int, QString>();
    QStringList defineNames = this->healLocationNameToValue.keys();
    QList<int> defineValues = this->healLocationNameToValue.values();
    for (auto name : defineNames) {
        int value = this->healLocationNameToValue.value(name);
        if (valuesToNames.contains(value)) {
            do { // Redefine duplicate as first available value
                value = nextDefineValue++;
            } while (defineValues.contains(value));
        }
        valuesToNames.insert(value, name);
    }

    // Check for new id names in the heal locations list
    for (auto hl : this->healLocations) {
        if (this->healLocationNameToValue.contains(hl.idName))
            continue;
        int value;
        do { // Give new heal location first available value
            value = nextDefineValue++;
        } while (valuesToNames.contains(value));
        valuesToNames.insert(value, hl.idName);
    }

    // Include guards
    const QString guardName = "GUARD_CONSTANTS_HEAL_LOCATIONS_H";
    QString constantsText = QString("#ifndef %1\n#define %1\n\n").arg(guardName);

    // List defines in ascending order
    QMap<int, QString>::const_iterator i;
    for (i = valuesToNames.constBegin(); i != valuesToNames.constEnd(); i++)
        constantsText += QString("#define %1 %2\n").arg(i.value()).arg(i.key());

    constantsText += QString("\n#endif // %1\n").arg(guardName);

    QString filepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::constants_heal_locations);
    ignoreWatchedFileTemporarily(filepath);
    saveTextFile(filepath, constantsText);
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

void Project::updateTilesetMetatileLabels(Tileset *tileset) {
    // Erase old labels, then repopulate with new labels
    const QString prefix = tileset->getMetatileLabelPrefix();
    metatileLabelsMap[tileset->name].clear();
    for (int metatileId : tileset->metatileLabels.keys()) {
        if (tileset->metatileLabels[metatileId].isEmpty())
            continue;
        QString label = prefix + tileset->metatileLabels[metatileId];
        metatileLabelsMap[tileset->name][label] = metatileId;
    }
}

// Given a map of define names to define values, returns a formatted list of #defines
QString Project::buildMetatileLabelsText(const QMap<QString, uint16_t> defines) {
    QStringList labels = defines.keys();

    // Setup for pretty formatting.
    int longestLength = 0;
    for (QString label : labels) {
        if (label.size() > longestLength)
            longestLength = label.size();
    }

    // Generate defines text
    QString output = QString();
    for (QString label : labels) {
        QString line = QString("#define %1  %2\n")
            .arg(label, -1 * longestLength)
            .arg(Metatile::getMetatileIdString(defines[label]));
        output += line;
    }
    return output;
}

void Project::saveTilesetMetatileLabels(Tileset *primaryTileset, Tileset *secondaryTileset) {
    // Skip writing the file if there are no labels in both the new and old sets
    if (metatileLabelsMap[primaryTileset->name].size() == 0 && primaryTileset->metatileLabels.size() == 0
     && metatileLabelsMap[secondaryTileset->name].size() == 0 && secondaryTileset->metatileLabels.size() == 0)
        return;

    updateTilesetMetatileLabels(primaryTileset);
    updateTilesetMetatileLabels(secondaryTileset);

    // Recreate metatile labels file
    const QString guardName = "GUARD_METATILE_LABELS_H";
    QString outputText = QString("#ifndef %1\n#define %1\n").arg(guardName);

    for (QString tilesetName : metatileLabelsMap.keys()) {
        if (metatileLabelsMap[tilesetName].size() == 0)
            continue;
        outputText += QString("\n// %1\n").arg(tilesetName);
        outputText += buildMetatileLabelsText(metatileLabelsMap[tilesetName]);
    }

    if (unusedMetatileLabels.size() != 0) {
        // Append any defines originally read from the file that aren't associated with any tileset.
        outputText += QString("\n// Other\n");
        outputText += buildMetatileLabelsText(unusedMetatileLabels);
    }

    outputText += QString("\n#endif // %1\n").arg(guardName);

    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_labels);
    ignoreWatchedFileTemporarily(root + "/" + filename);
    saveTextFile(root + "/" + filename, outputText);
}

void Project::saveTilesetMetatileAttributes(Tileset *tileset) {
    QFile attrs_file(tileset->metatile_attrs_path);
    if (attrs_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QByteArray data;
        int attrSize = projectConfig.getMetatileAttributesSize();
        for (Metatile *metatile : tileset->metatiles) {
            uint32_t attributes = metatile->getAttributes();
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
        int numTiles = projectConfig.getNumTilesInMetatile();
        for (Metatile *metatile : tileset->metatiles) {
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
    // Only write the tiles image if it was changed.
    // Porymap will only ever change an existing tiles image by importing a new one.
    if (tileset->hasUnsavedTilesImage) {
        if (!tileset->tilesImage.save(tileset->tilesImagePath, "PNG")) {
            logError(QString("Failed to save tiles image '%1'").arg(tileset->tilesImagePath));
            return;
        }
        tileset->hasUnsavedTilesImage = false;
    }
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
        QString defaultTileset = this->getDefaultPrimaryTilesetLabel();
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
        QString defaultTileset = this->getDefaultSecondaryTilesetLabel();
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
    auto memberMap = Tileset::getHeaderMemberMap(this->usingAsmTilesets);
    if (this->usingAsmTilesets) {
        // Read asm tileset header. Backwards compatibility
        const QStringList values = parser.getLabelValues(parser.parseAsm(projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm)), label);
        if (values.isEmpty()) {
            return nullptr;
        }
        if (tileset == nullptr) {
            tileset = new Tileset;
        }
        tileset->name = label;
        tileset->is_secondary = ParseUtil::gameStringToBool(values.value(memberMap.key("isSecondary")));
        tileset->tiles_label = values.value(memberMap.key("tiles"));
        tileset->palettes_label = values.value(memberMap.key("palettes"));
        tileset->metatiles_label = values.value(memberMap.key("metatiles"));
        tileset->metatile_attrs_label = values.value(memberMap.key("metatileAttributes"));
    } else {
        // Read C tileset header
        const auto structs = parser.readCStructs(projectConfig.getFilePath(ProjectFilePath::tilesets_headers), label, memberMap);
        if (!structs.contains(label)) {
            return nullptr;
        }
        if (tileset == nullptr) {
            tileset = new Tileset;
        }
        const auto tilesetAttributes = structs[label];
        tileset->name = label;
        tileset->is_secondary = ParseUtil::gameStringToBool(tilesetAttributes.value("isSecondary"));
        tileset->tiles_label = tilesetAttributes.value("tiles");
        tileset->palettes_label = tilesetAttributes.value("palettes");
        tileset->metatiles_label = tilesetAttributes.value("metatiles");
        tileset->metatile_attrs_label = tilesetAttributes.value("metatileAttributes");
    }

    loadTilesetAssets(tileset);

    tilesetCache.insert(label, tileset);
    return tileset;
}

bool Project::loadBlockdata(MapLayout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->blockdata_path);
    layout->blockdata = readBlockdata(path);
    layout->lastCommitBlocks.blocks = layout->blockdata;
    layout->lastCommitBlocks.mapDimensions = QSize(layout->getWidth(), layout->getHeight());

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
    int width = map->getWidth();
    int height = map->getHeight();
    Block block(projectConfig.getDefaultMetatileId(), projectConfig.getDefaultCollision(), projectConfig.getDefaultElevation());
    for (int i = 0; i < width * height; i++) {
        map->layout->blockdata.append(block);
    }
    map->layout->lastCommitBlocks.blocks = map->layout->blockdata;
    map->layout->lastCommitBlocks.mapDimensions = QSize(width, height);
}

bool Project::loadLayoutBorder(MapLayout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->border_path);
    layout->border = readBlockdata(path);
    layout->lastCommitBlocks.border = layout->border;
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

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
    int width = map->getBorderWidth();
    int height = map->getBorderHeight();

    const QList<uint16_t> configMetatileIds = projectConfig.getNewMapBorderMetatileIds();
    if (configMetatileIds.length() != width * height) {
        // Border size doesn't match the number of default border metatiles.
        // Fill the border with empty metatiles.
        for (int i = 0; i < width * height; i++) {
            map->layout->border.append(0);
        }
    } else {
        // Fill the border with the default metatiles from the config.
        for (int i = 0; i < width * height; i++) {
            map->layout->border.append(configMetatileIds.at(i));
        }
    }

    map->layout->lastCommitBlocks.border = map->layout->border;
    map->layout->lastCommitBlocks.borderDimensions = QSize(width, height);
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
    QString basePath = projectConfig.getFilePath(ProjectFilePath::data_map_folders);
    QString mapDataDir = root + "/" + basePath + map->name;
    if (!map->isPersistedToFile) {
        if (!QDir::root().mkdir(mapDataDir)) {
            logError(QString("Error: failed to create directory for new map: '%1'").arg(mapDataDir));
        }

        // Create file data/maps/<map_name>/scripts.inc
        QString text = this->getScriptDefaultString(projectConfig.getUsePoryScript(), map->name);
        saveTextFile(mapDataDir + "/scripts" + this->getScriptFileExtension(projectConfig.getUsePoryScript()), text);

        bool usesTextFile = projectConfig.getCreateMapTextFileEnabled();
        if (usesTextFile) {
            // Create file data/maps/<map_name>/text.inc
            saveTextFile(mapDataDir + "/text" + this->getScriptFileExtension(projectConfig.getUsePoryScript()), "\n");
        }

        // Simply append to data/event_scripts.s.
        text = QString("\n\t.include \"%1%2/scripts.inc\"\n").arg(basePath, map->name);
        if (usesTextFile) {
            text += QString("\t.include \"%1%2/text.inc\"\n").arg(basePath, map->name);
        }
        appendTextFile(root + "/" + projectConfig.getFilePath(ProjectFilePath::data_event_scripts), text);

        if (map->needsLayoutDir) {
            QString newLayoutDir = QString(root + "/%1%2").arg(projectConfig.getFilePath(ProjectFilePath::data_layouts_folders), map->name);
            if (!QDir::root().mkdir(newLayoutDir)) {
                logError(QString("Error: failed to create directory for new layout: '%1'").arg(newLayoutDir));
            }
        }
    }

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
    mapObj["requires_flash"] = map->requiresFlash;
    mapObj["weather"] = map->weather;
    mapObj["map_type"] = map->type;
    if (projectConfig.getMapAllowFlagsEnabled()) {
        mapObj["allow_cycling"] = map->allowBiking;
        mapObj["allow_escaping"] = map->allowEscaping;
        mapObj["allow_running"] = map->allowRunning;
    }
    mapObj["show_map_name"] = map->show_location;
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
                connectionObj["map"] = this->mapNamesToMapConstants.value(connection->map_name);
                connectionObj["offset"] = connection->offset;
                connectionObj["direction"] = connection->direction;
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
        for (int i = 0; i < map->events[Event::Group::Object].length(); i++) {
            Event *event = map->events[Event::Group::Object].value(i);
            OrderedJson::object jsonObj = event->buildEventJson(this);
            objectEventsArr.push_back(jsonObj);
        }
        mapObj["object_events"] = objectEventsArr;


        // Warp events
        OrderedJson::array warpEventsArr;
        for (int i = 0; i < map->events[Event::Group::Warp].length(); i++) {
            Event *event = map->events[Event::Group::Warp].value(i);
            OrderedJson::object warpObj = event->buildEventJson(this);
            warpEventsArr.append(warpObj);
        }
        mapObj["warp_events"] = warpEventsArr;

        // Coord events
        OrderedJson::array coordEventsArr;
        for (int i = 0; i < map->events[Event::Group::Coord].length(); i++) {
            Event *event = map->events[Event::Group::Coord].value(i);
            OrderedJson::object triggerObj = event->buildEventJson(this);
            coordEventsArr.append(triggerObj);
        }
        mapObj["coord_events"] = coordEventsArr;

        // Bg Events
        OrderedJson::array bgEventsArr;
        for (int i = 0; i < map->events[Event::Group::Bg].length(); i++) {
            Event *event = map->events[Event::Group::Bg].value(i);
            OrderedJson::object bgObj = event->buildEventJson(this);
            bgEventsArr.append(bgObj);
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
        mapObj[key] = OrderedJson::fromQJsonValue(map->customHeaders[key]);
    }

    OrderedJson mapJson(mapObj);
    OrderedJsonDoc jsonDoc(&mapJson);
    jsonDoc.dump(&mapFile);
    mapFile.close();

    saveLayoutBorder(map);
    saveLayoutBlockdata(map);
    saveHealLocations(map);

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
    if (tileset->name.isNull()) {
        return;
    }
    this->readTilesetPaths(tileset);
    QImage image;
    if (QFile::exists(tileset->tilesImagePath)) {
        image = QImage(tileset->tilesImagePath).convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
        flattenTo4bppImage(&image);
    } else {
        image = QImage(8, 8, QImage::Format_Indexed8);
    }
    this->loadTilesetTiles(tileset, image);
    this->loadTilesetMetatiles(tileset);
    this->loadTilesetMetatileLabels(tileset);
    this->loadTilesetPalettes(tileset);
}

void Project::readTilesetPaths(Tileset* tileset) {
    // Parse the tileset data files to try and get explicit file paths for this tileset's assets
    const QString rootDir = this->root + "/";
    if (this->usingAsmTilesets) {
        // Read asm tileset data files. Backwards compatibility
        const QList<QStringList> graphics = parser.parseAsm(projectConfig.getFilePath(ProjectFilePath::tilesets_graphics_asm));
        const QList<QStringList> metatiles_macros = parser.parseAsm(projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles_asm));

        const QStringList tiles_values = parser.getLabelValues(graphics, tileset->tiles_label);
        const QStringList palettes_values = parser.getLabelValues(graphics, tileset->palettes_label);
        const QStringList metatiles_values = parser.getLabelValues(metatiles_macros, tileset->metatiles_label);
        const QStringList metatile_attrs_values = parser.getLabelValues(metatiles_macros, tileset->metatile_attrs_label);

        if (!tiles_values.isEmpty())
            tileset->tilesImagePath = this->fixGraphicPath(rootDir + tiles_values.value(0).section('"', 1, 1));
        if (!metatiles_values.isEmpty())
            tileset->metatiles_path = rootDir + metatiles_values.value(0).section('"', 1, 1);
        if (!metatile_attrs_values.isEmpty())
            tileset->metatile_attrs_path = rootDir + metatile_attrs_values.value(0).section('"', 1, 1);
        for (const auto &value : palettes_values)
            tileset->palettePaths.append(this->fixPalettePath(rootDir + value.section('"', 1, 1)));
    } else {
        // Read C tileset data files
        const QString graphicsFile = projectConfig.getFilePath(ProjectFilePath::tilesets_graphics);
        const QString metatilesFile = projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles);
        
        const QString tilesImagePath = parser.readCIncbin(graphicsFile, tileset->tiles_label);
        const QStringList palettePaths = parser.readCIncbinArray(graphicsFile, tileset->palettes_label);
        const QString metatilesPath = parser.readCIncbin(metatilesFile, tileset->metatiles_label);
        const QString metatileAttrsPath = parser.readCIncbin(metatilesFile, tileset->metatile_attrs_label);

        if (!tilesImagePath.isEmpty())
            tileset->tilesImagePath = this->fixGraphicPath(rootDir + tilesImagePath);
        if (!metatilesPath.isEmpty())
            tileset->metatiles_path = rootDir + metatilesPath;
        if (!metatileAttrsPath.isEmpty())
            tileset->metatile_attrs_path = rootDir + metatileAttrsPath;
        for (const auto &path : palettePaths)
            tileset->palettePaths.append(this->fixPalettePath(rootDir + path));
    }

    // Try to set default paths, if any weren't found by reading the files above
    QString defaultPath = rootDir + tileset->getExpectedDir();
    if (tileset->tilesImagePath.isEmpty())
        tileset->tilesImagePath = defaultPath + "/tiles.png";
    if (tileset->metatiles_path.isEmpty())
        tileset->metatiles_path = defaultPath + "/metatiles.bin";
    if (tileset->metatile_attrs_path.isEmpty())
        tileset->metatile_attrs_path = defaultPath + "/metatile_attributes.bin";
    if (tileset->palettePaths.isEmpty()) {
        QString palettes_dir_path = defaultPath + "/palettes/";
        for (int i = 0; i < 16; i++) {
            tileset->palettePaths.append(palettes_dir_path + QString("%1").arg(i, 2, 10, QLatin1Char('0')) + ".pal");
        }
    }
}

void Project::loadTilesetPalettes(Tileset* tileset) {
    QList<QList<QRgb>> palettes;
    QList<QList<QRgb>> palettePreviews;
    for (int i = 0; i < tileset->palettePaths.length(); i++) {
        QString path = tileset->palettePaths.value(i);
        bool error = false;
        QList<QRgb> palette = PaletteUtil::parse(path, &error);
        if (error) {
            for (int j = 0; j < 16; j++) {
                palette.append(qRgb(j * 16, j * 16, j * 16));
            }
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
        int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
        int bytesPerMetatile = 2 * tilesPerMetatile;
        int num_metatiles = data.length() / bytesPerMetatile;
        QList<Metatile*> metatiles;
        for (int i = 0; i < num_metatiles; i++) {
            Metatile *metatile = new Metatile;
            int index = i * bytesPerMetatile;
            for (int j = 0; j < tilesPerMetatile; j++) {
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
        int attrSize = projectConfig.getMetatileAttributesSize();
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
            tileset->metatiles.at(i)->setAttributes(attributes);
        }
    } else {
        logError(QString("Could not open tileset metatile attributes file '%1'").arg(tileset->metatile_attrs_path));
    }
}

QString Project::findMetatileLabelsTileset(QString label) {
    for (QString tilesetName : this->tilesetLabelsOrdered) {
        QString metatileLabelPrefix = Tileset::getMetatileLabelPrefix(tilesetName);
        if (label.startsWith(metatileLabelPrefix))
            return tilesetName;
    }
    return QString();
}

bool Project::readTilesetMetatileLabels() {
    metatileLabelsMap.clear();
    unusedMetatileLabels.clear();

    QString metatileLabelsFilename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_labels);
    fileWatcher.addPath(root + "/" + metatileLabelsFilename);

    const QStringList prefixes = {QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix))};
    QMap<QString, int> defines = parser.readCDefinesByPrefix(metatileLabelsFilename, prefixes);

    for (QString label : defines.keys()) {
        uint32_t metatileId = static_cast<uint32_t>(defines[label]);
        if (metatileId > Block::maxValue) {
            metatileId &= Block::maxValue;
            logWarn(QString("Value of metatile label '%1' truncated to %2").arg(label).arg(Metatile::getMetatileIdString(metatileId)));
        }
        QString tilesetName = findMetatileLabelsTileset(label);
        if (!tilesetName.isEmpty()) {
            metatileLabelsMap[tilesetName][label] = metatileId;
        } else {
            // This #define name does not match any existing tileset.
            // Save it separately to be outputted later.
            unusedMetatileLabels[label] = metatileId;
        }
    }

    return true;
}

void Project::loadTilesetMetatileLabels(Tileset* tileset) {
    QString metatileLabelPrefix = tileset->getMetatileLabelPrefix();

    // Reverse map for faster lookup by metatile id
    for (QString labelName : metatileLabelsMap[tileset->name].keys()) {
        auto metatileId = metatileLabelsMap[tileset->name][labelName];
        tileset->metatileLabels[metatileId] = labelName.replace(metatileLabelPrefix, "");
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
    if (!userConfig.getEncounterJsonActive()) {
        return true;
    }

    QString wildMonJsonFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_wild_encounters));
    fileWatcher.addPath(wildMonJsonFilepath);

    OrderedJson::object wildMonObj;
    if (!parser.tryParseOrderedJsonFile(&wildMonObj, wildMonJsonFilepath)) {
        // Failing to read wild encounters data is not a critical error, just disable the
        // encounter editor and log a warning in case the user intended to have this data.
        userConfig.setEncounterJsonActive(false);
        emit disableWildEncountersUI();
        logWarn(QString("Failed to read wild encounters from %1").arg(wildMonJsonFilepath));
        return true;
    }

    // For each encounter type, count the number of times each encounter rate value occurs.
    // The most common value will be used as the default for new groups.
    QMap<QString, QMap<int, int>> encounterRateFrequencyMaps;

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
            encounterRateFrequencyMaps.insert(encounterField.name, QMap<int, int>());
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
                    encounterRateFrequencyMaps[field][header.wildMons[field].encounterRate]++;
                    for (auto mon : encounterFieldObj["mons"].array_items()) {
                        WildPokemon newMon;
                        OrderedJson::object monObj = mon.object_items();
                        newMon.minLevel = monObj["min_level"].int_value();
                        newMon.maxLevel = monObj["max_level"].int_value();
                        newMon.species = monObj["species"].string_value();
                        header.wildMons[field].wildPokemon.append(newMon);
                    }
                    // If the user supplied too few pokémon for this group then we fill in the rest.
                    for (int i = header.wildMons[field].wildPokemon.length(); i < monField.encounterRates.length(); i++) {
                        WildPokemon newMon; // Keep default values
                        header.wildMons[field].wildPokemon.append(newMon);
                    }
                }
            }
            wildMonData[mapConstant].insert({encounterObj["base_label"].string_value(), header});
            encounterGroupLabels.append(encounterObj["base_label"].string_value());
        }
    }

    // For each encounter type, set default encounter rate to most common value.
    // Iterate over map of encounter type names to frequency maps...
    for (auto i = encounterRateFrequencyMaps.cbegin(), i_end = encounterRateFrequencyMaps.cend(); i != i_end; i++) {
        int frequency = 0;
        int rate = 1;
        const QMap<int, int> frequencyMap = i.value();
        // Iterate over frequency map (encounter rate to number of occurrences)...
        for (auto j = frequencyMap.cbegin(), j_end = frequencyMap.cend(); j != j_end; j++) {
            if (j.value() > frequency) {
                frequency = j.value();
                rate = j.key();
            }
        }
        setDefaultEncounterRate(i.key(), rate);
    }

    return true;
}

bool Project::readMapGroups() {
    mapConstantsToMapNames.clear();
    mapNamesToMapConstants.clear();
    mapGroups.clear();

    QString mapGroupsFilepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::json_map_groups);
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
        QString groupName = ParseUtil::jsonToQString(mapGroupOrder.at(groupIndex));
        QJsonArray mapNames = mapGroupsObj.value(groupName).toArray();
        groupedMaps.append(QStringList());
        groups.append(groupName);
        for (int j = 0; j < mapNames.size(); j++) {
            QString mapName = ParseUtil::jsonToQString(mapNames.at(j));
            mapGroups.insert(mapName, groupIndex);
            groupedMaps[groupIndex].append(mapName);
            maps.append(mapName);

            // Build the mapping and reverse mapping between map constants and map names.
            QString mapConstant = Map::mapConstantFromName(mapName);
            mapConstantsToMapNames.insert(mapConstant, mapName);
            mapNamesToMapConstants.insert(mapName, mapConstant);
        }
    }

    const QString defineName = this->getDynamicMapDefineName();
    mapConstantsToMapNames.insert(defineName, DYNAMIC_MAP_NAME);
    mapNamesToMapConstants.insert(DYNAMIC_MAP_NAME, defineName);
    maps.append(DYNAMIC_MAP_NAME);

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

Project::DataQualifiers Project::getDataQualifiers(QString text, QString label) {
    Project::DataQualifiers qualifiers;

    QRegularExpression regex(QString("\\s*(?<static>static\\s*)?(?<const>const\\s*)?[A-Za-z0-9_\\s]*\\b%1\\b").arg(label));
    QRegularExpressionMatch match = regex.match(text);

    qualifiers.isStatic = match.captured("static").isNull() ? false : true;
    qualifiers.isConst = match.captured("const").isNull() ? false : true;

    return qualifiers;
}

QString Project::getDefaultPrimaryTilesetLabel() {
    QString defaultLabel = projectConfig.getDefaultPrimaryTileset();
    if (!this->primaryTilesetLabels.contains(defaultLabel)) {
        QString firstLabel = this->primaryTilesetLabels.first();
        logWarn(QString("Unable to find default primary tileset '%1', using '%2' instead.").arg(defaultLabel).arg(firstLabel));
        defaultLabel = firstLabel;
    }
    return defaultLabel;
}

QString Project::getDefaultSecondaryTilesetLabel() {
    QString defaultLabel = projectConfig.getDefaultSecondaryTileset();
    if (!this->secondaryTilesetLabels.contains(defaultLabel)) {
        QString firstLabel = this->secondaryTilesetLabels.first();
        logWarn(QString("Unable to find default secondary tileset '%1', using '%2' instead.").arg(defaultLabel).arg(firstLabel));
        defaultLabel = firstLabel;
    }
    return defaultLabel;
}

void Project::appendTilesetLabel(QString label, QString isSecondaryStr) {
    bool ok;
    bool isSecondary = ParseUtil::gameStringToBool(isSecondaryStr, &ok);
    if (!ok) {
        logError(QString("Unable to convert value '%1' of isSecondary to bool for tileset %2.").arg(isSecondaryStr).arg(label));
        return;
    }
    QStringList * list = isSecondary ? &this->secondaryTilesetLabels : &this->primaryTilesetLabels;
    list->append(label);
    this->tilesetLabelsOrdered.append(label);
}

bool Project::readTilesetLabels() {
    QStringList primaryTilesets;
    QStringList secondaryTilesets;
    this->primaryTilesetLabels.clear();
    this->secondaryTilesetLabels.clear();
    this->tilesetLabelsOrdered.clear();

    QString filename = projectConfig.getFilePath(ProjectFilePath::tilesets_headers);
    QFileInfo fileInfo(this->root + "/" + filename);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        // If the tileset headers file is missing, the user may still have the old assembly format.
        this->usingAsmTilesets = true;
        QString asm_filename = projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm);
        QString text = parser.readTextFile(this->root + "/" + asm_filename);
        if (text.isEmpty()) {
            logError(QString("Failed to read tileset labels from '%1' or '%2'.").arg(filename).arg(asm_filename));
            return false;
        }
        static const QRegularExpression re("(?<label>[A-Za-z0-9_]*):{1,2}[A-Za-z0-9_@ ]*\\s+.+\\s+\\.byte\\s+(?<isSecondary>[A-Za-z0-9_]+)");
        QRegularExpressionMatchIterator iter = re.globalMatch(text);
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            appendTilesetLabel(match.captured("label"), match.captured("isSecondary"));
        }
        this->primaryTilesetLabels.sort();
        this->secondaryTilesetLabels.sort();
        this->tilesetLabelsOrdered.sort();
        filename = asm_filename; // For error reporting further down
    } else {
        this->usingAsmTilesets = false;
        const auto structs = parser.readCStructs(filename, "", Tileset::getHeaderMemberMap(this->usingAsmTilesets));
        QStringList labels = structs.keys();
        // TODO: This is alphabetical, AdvanceMap import wants the vanilla order in tilesetLabelsOrdered
        for (const auto tilesetLabel : labels){
            appendTilesetLabel(tilesetLabel, structs[tilesetLabel].value("isSecondary"));
        }
    }

    bool success = true;
    if (this->secondaryTilesetLabels.isEmpty()) {
        logError(QString("Failed to find any secondary tilesets in %1").arg(filename));
        success = false;
    }
    if (this->primaryTilesetLabels.isEmpty()) {
        logError(QString("Failed to find any primary tilesets in %1").arg(filename));
        success = false;
    }
    return success;
}

bool Project::readFieldmapProperties() {
    const QString numTilesPrimaryName = projectConfig.getIdentifier(ProjectIdentifier::define_tiles_primary);
    const QString numTilesTotalName = projectConfig.getIdentifier(ProjectIdentifier::define_tiles_total);
    const QString numMetatilesPrimaryName = projectConfig.getIdentifier(ProjectIdentifier::define_metatiles_primary);
    const QString numPalsPrimaryName = projectConfig.getIdentifier(ProjectIdentifier::define_pals_primary);
    const QString numPalsTotalName = projectConfig.getIdentifier(ProjectIdentifier::define_pals_total);
    const QString maxMapSizeName = projectConfig.getIdentifier(ProjectIdentifier::define_map_size);
    const QStringList names = {
        numTilesPrimaryName,
        numTilesTotalName,
        numMetatilesPrimaryName,
        numPalsPrimaryName,
        numPalsTotalName,
        maxMapSizeName,
    };
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_fieldmap);
    fileWatcher.addPath(root + "/" + filename);
    const QMap<QString, int> defines = parser.readCDefinesByName(filename, names);

    auto loadDefine = [defines](const QString name, int * dest) {
        auto it = defines.find(name);
        if (it != defines.end()) {
            *dest = it.value();
        } else {
            logWarn(QString("Value for tileset property '%1' not found. Using default (%2) instead.").arg(name).arg(*dest));
        }
    };
    loadDefine(numTilesPrimaryName,     &Project::num_tiles_primary);
    loadDefine(numTilesTotalName,       &Project::num_tiles_total);
    loadDefine(numMetatilesPrimaryName, &Project::num_metatiles_primary);
    loadDefine(numPalsPrimaryName,      &Project::num_pals_primary);
    loadDefine(numPalsTotalName,        &Project::num_pals_total);

    auto it = defines.find(maxMapSizeName);
    if (it != defines.end()) {
        int min = getMapDataSize(1, 1);
        if (it.value() >= min) {
            Project::max_map_data_size = it.value();
            calculateDefaultMapSize();
        } else {
            // must be large enough to support a 1x1 map
            logWarn(QString("Value for map property '%1' is %2, must be at least %3. Using default (%4) instead.")
                    .arg(maxMapSizeName)
                    .arg(it.value())
                    .arg(min)
                    .arg(Project::max_map_data_size));
        }
    }
    else {
        logWarn(QString("Value for map property '%1' not found. Using default (%2) instead.")
                .arg(maxMapSizeName)
                .arg(Project::max_map_data_size));
    }

    return true;
}

// Read data masks for Blocks and metatile attributes.
bool Project::readFieldmapMasks() {
    const QString metatileIdMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_metatile);
    const QString collisionMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_collision);
    const QString elevationMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_elevation);
    const QString behaviorMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_behavior);
    const QString layerTypeMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_layer);
    const QStringList searchNames = {
        metatileIdMaskName,
        collisionMaskName,
        elevationMaskName,
        behaviorMaskName,
        layerTypeMaskName,
    };
    QString globalFieldmap = projectConfig.getFilePath(ProjectFilePath::global_fieldmap);
    fileWatcher.addPath(root + "/" + globalFieldmap);
    QMap<QString, int> defines = parser.readCDefinesByName(globalFieldmap, searchNames);

    // These mask values are accessible via the settings editor for users who don't have these defines.
    // If users do have the defines we disable them in the settings editor and direct them to their project files.
    // Record the names we read so we know later which settings to disable.
    const QStringList defineNames = defines.keys();
    this->disabledSettingsNames = QSet<QString>(defineNames.constBegin(), defineNames.constEnd());

    // Avoid repeatedly writing the config file
    projectConfig.setSaveDisabled(true);

    // Read Block masks
    auto readBlockMask = [defines](const QString name, uint16_t *value) {
        auto it = defines.find(name);
        if (it == defines.end())
            return false;
        *value = static_cast<uint16_t>(it.value());
        if (*value != it.value()){
            logWarn(QString("Value for %1 truncated from '0x%2' to '0x%3'")
                .arg(name)
                .arg(QString::number(it.value(), 16).toUpper())
                .arg(QString::number(*value, 16).toUpper()));
        }
        return true;
    };
    uint16_t blockMask;
    if (readBlockMask(metatileIdMaskName, &blockMask))
        projectConfig.setBlockMetatileIdMask(blockMask);
    if (readBlockMask(collisionMaskName, &blockMask))
        projectConfig.setBlockCollisionMask(blockMask);
    if (readBlockMask(elevationMaskName, &blockMask))
        projectConfig.setBlockElevationMask(blockMask);

    // Read RSE metatile attribute masks
    auto it = defines.find(behaviorMaskName);
    if (it != defines.end())
        projectConfig.setMetatileBehaviorMask(static_cast<uint32_t>(it.value()));
    it = defines.find(layerTypeMaskName);
    if (it != defines.end())
        projectConfig.setMetatileLayerTypeMask(static_cast<uint32_t>(it.value()));

    // pokefirered keeps its attribute masks in a separate table, parse this too.
    const QString attrTableName = projectConfig.getIdentifier(ProjectIdentifier::symbol_attribute_table);
    const QString srcFieldmap = projectConfig.getFilePath(ProjectFilePath::fieldmap);
    const QMap<QString, QString> attrTable = parser.readNamedIndexCArray(srcFieldmap, attrTableName);
    if (!attrTable.isEmpty()) {
        const QString behaviorTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_behavior);
        const QString layerTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_layer);
        const QString encounterTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_encounter);
        const QString terrainTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_terrain);
        fileWatcher.addPath(root + "/" + srcFieldmap);

        bool ok;
        // Read terrain type mask
        uint32_t mask = attrTable.value(terrainTypeTableName).toUInt(&ok, 0);
        if (ok) {
            projectConfig.setMetatileTerrainTypeMask(mask);
            this->disabledSettingsNames.insert(terrainTypeTableName);
        }
        // Read encounter type mask
        mask = attrTable.value(encounterTypeTableName).toUInt(&ok, 0);
        if (ok) {
            projectConfig.setMetatileEncounterTypeMask(mask);
            this->disabledSettingsNames.insert(encounterTypeTableName);
        }
        // If we haven't already parsed behavior and layer type then try those too
        if (!this->disabledSettingsNames.contains(behaviorMaskName)) {
            // Read behavior mask
            mask = attrTable.value(behaviorTableName).toUInt(&ok, 0);
            if (ok) {
                projectConfig.setMetatileBehaviorMask(mask);
                this->disabledSettingsNames.insert(behaviorTableName);
            }
        }
        if (!this->disabledSettingsNames.contains(layerTypeMaskName)) {
            // Read layer type mask
            mask = attrTable.value(layerTypeTableName).toUInt(&ok, 0);
            if (ok) {
                projectConfig.setMetatileLayerTypeMask(mask);
                this->disabledSettingsNames.insert(layerTypeTableName);
            }
        }
    }
    projectConfig.setSaveDisabled(false);
    return true;
}

bool Project::readRegionMapSections() {
    this->mapSectionNameToValue.clear();
    this->mapSectionValueToName.clear();

    const QStringList prefixes = {QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix))};
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_region_map_sections);
    fileWatcher.addPath(root + "/" + filename);
    this->mapSectionNameToValue = parser.readCDefinesByPrefix(filename, prefixes);
    if (this->mapSectionNameToValue.isEmpty()) {
        logError(QString("Failed to read region map sections from %1.").arg(filename));
        return false;
    }

    for (QString defineName : this->mapSectionNameToValue.keys()) {
        this->mapSectionValueToName.insert(this->mapSectionNameToValue[defineName], defineName);
    }
    return true;
}

// Read the constants to preserve any "unused" heal locations when writing the file later
bool Project::readHealLocationConstants() {
    this->healLocationNameToValue.clear();
    const QStringList prefixes = {
        QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_heal_locations_prefix)),
        QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_spawn_prefix))
    };
    QString constantsFilename = projectConfig.getFilePath(ProjectFilePath::constants_heal_locations);
    fileWatcher.addPath(root + "/" + constantsFilename);
    this->healLocationNameToValue = parser.readCDefinesByPrefix(constantsFilename, prefixes);
    // No need to check if empty, not finding any heal location constants is ok
    return true;
}

// TODO: Simplify using the new C struct parsing functions (and indexed array parsing functions)
bool Project::readHealLocations() {
    this->healLocations.clear();

    if (!this->readHealLocationConstants())
        return false;

    QString filename = projectConfig.getFilePath(ProjectFilePath::data_heal_locations);
    fileWatcher.addPath(root + "/" + filename);
    QString text = parser.readTextFile(root + "/" + filename);

    // Strip comments
    static const QRegularExpression re_comments("//.*?(\r\n?|\n)|/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption);
    text.replace(re_comments, "");

    bool respawnEnabled = projectConfig.getHealLocationRespawnDataEnabled();

    // Search for the name of the main Heal Locations table
    const QRegularExpression tableNameExpr(QString("%1\\s+(?<name>[A-Za-z0-9_]+)\\[").arg(projectConfig.getIdentifier(ProjectIdentifier::symbol_heal_locations_type)));
    const QRegularExpressionMatch tableNameMatch = tableNameExpr.match(text);
    if (tableNameMatch.hasMatch()) {
        // Found table name, record it and its qualifiers for output when saving.
        this->healLocationsTableName = tableNameMatch.captured("name");
        this->healLocationDataQualifiers = this->getDataQualifiers(text, this->healLocationsTableName);
    } else {
        // No table name found, initialize default name for output when saving.
        this->healLocationsTableName = respawnEnabled ? projectConfig.getIdentifier(ProjectIdentifier::symbol_spawn_points)
                                                      : projectConfig.getIdentifier(ProjectIdentifier::symbol_heal_locations);
        this->healLocationDataQualifiers = { .isStatic = true, .isConst = true };
    }

    // Create regex pattern for the constants (ex: "SPAWN_PALLET_TOWN" or "HEAL_LOCATION_PETALBURG_CITY")
    const QString spawnPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_spawn_prefix);
    const QString healLocPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_heal_locations_prefix);
    const QRegularExpression constantsExpr(QString("\\b(%1|%2)[A-Za-z0-9_]+").arg(spawnPrefix).arg(healLocPrefix));

    // Find all the unique heal location constants used in the data tables.
    // Porymap doesn't care whether or not a constant appeared in the heal locations constants file.
    // Any data entry without a designated initializer using one of these constants will be silently discarded.
    // Any data entry that repeats a designated initializer will also be discarded.
    QStringList constants = QStringList();
    QRegularExpressionMatchIterator constantsMatch = constantsExpr.globalMatch(text);
    while (constantsMatch.hasNext())
        constants << constantsMatch.next().captured();
    constants.removeDuplicates();

    // Pattern for a map value pair (ex: "MAP_GROUP(PALLET_TOWN), MAP_NUM(PALLET_TOWN)")
    const QString mapPattern = "MAP_GROUP[\\(\\s]+(?<map>[A-Za-z0-9_]+)[\\s\\)]+,\\s*MAP_NUM[\\(\\s]+(\\1)[\\s\\)]+";
    // Pattern for an x, y number pair
    const QString coordPattern = "\\s*(?<x>[0-9A-Fa-fx]+),\\s*(?<y>[0-9A-Fa-fx]+)"; 

    for (const auto idName : constants) {
        // Create regex pattern for e.g. "SPAWN_PALLET_TOWN - 1] = "
        const QString initializerPattern = QString("%1\\s*-\\s*1\\s*\\]\\s*=\\s*").arg(idName);

        // Expression for location data, e.g. "SPAWN_PALLET_TOWN - 1] = {MAP_GROUP(PALLET_TOWN), MAP_NUM(PALLET_TOWN), x, y}"
        QRegularExpression locationRegex(QString("%1\\{%2,%3}").arg(initializerPattern).arg(mapPattern).arg(coordPattern));
        QRegularExpressionMatch match = locationRegex.match(text);

        // Read location data
        HealLocation healLocation;
        if (match.hasMatch()) {
            QString mapName = match.captured("map");
            int x = match.captured("x").toInt(nullptr, 0);
            int y = match.captured("y").toInt(nullptr, 0);
            healLocation = HealLocation(idName, mapName, this->healLocations.size() + 1, x, y);
        } else {
            // This heal location has data, but is missing from the location table and won't be displayed by Porymap.
            // Add a dummy entry, and preserve the rest of its data for the user anyway
            healLocation = HealLocation(idName, "", this->healLocations.size() + 1, 0, 0);
        }

        // Read respawn data
        if (respawnEnabled) {
            // Expression for respawn map data, e.g. "SPAWN_PALLET_TOWN - 1] = {MAP_GROUP(PALLET_TOWN_PLAYERS_HOUSE_1F), MAP_NUM(PALLET_TOWN_PLAYERS_HOUSE_1F)}"
            QRegularExpression respawnMapRegex(QString("%1\\{%2}").arg(initializerPattern).arg(mapPattern));
            match = respawnMapRegex.match(text);
            if (match.hasMatch())
                healLocation.respawnMap = match.captured("map");

            // Expression for respawn npc data, e.g. "SPAWN_PALLET_TOWN - 1] = 1"
            QRegularExpression respawnNPCRegex(QString("%1(?<npc>[0-9]+)").arg(initializerPattern));
            match = respawnNPCRegex.match(text);
            if (match.hasMatch())
                healLocation.respawnNPC = match.captured("npc").toInt(nullptr, 0);
        }

        this->healLocations.append(healLocation);
    }
    // No need to check if empty, not finding any heal locations is ok
    return true;
}

bool Project::readItemNames() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_items)};  
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_items);
    fileWatcher.addPath(root + "/" + filename);
    itemNames = parser.readCDefineNames(filename, prefixes);
    if (itemNames.isEmpty()) {
        logError(QString("Failed to read item constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readFlagNames() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_flags)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_flags);
    fileWatcher.addPath(root + "/" + filename);
    flagNames = parser.readCDefineNames(filename, prefixes);
    if (flagNames.isEmpty()) {
        logError(QString("Failed to read flag constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readVarNames() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_vars)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_vars);
    fileWatcher.addPath(root + "/" + filename);
    varNames = parser.readCDefineNames(filename, prefixes);
    if (varNames.isEmpty()) {
        logError(QString("Failed to read var constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMovementTypes() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_movement_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_event_movement);
    fileWatcher.addPath(root + "/" + filename);
    movementTypes = parser.readCDefineNames(filename, prefixes);
    if (movementTypes.isEmpty()) {
        logError(QString("Failed to read movement type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readInitialFacingDirections() {
    QString filename = projectConfig.getFilePath(ProjectFilePath::initial_facing_table);
    fileWatcher.addPath(root + "/" + filename);
    facingDirections = parser.readNamedIndexCArray(filename, projectConfig.getIdentifier(ProjectIdentifier::symbol_facing_directions));
    if (facingDirections.isEmpty()) {
        logError(QString("Failed to read initial movement type facing directions from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMapTypes() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_map_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    fileWatcher.addPath(root + "/" + filename);
    mapTypes = parser.readCDefineNames(filename, prefixes);
    if (mapTypes.isEmpty()) {
        logError(QString("Failed to read map type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMapBattleScenes() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_battle_scenes)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    fileWatcher.addPath(root + "/" + filename);
    mapBattleScenes = parser.readCDefineNames(filename, prefixes);
    if (mapBattleScenes.isEmpty()) {
        logError(QString("Failed to read map battle scene constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readWeatherNames() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_weather)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    fileWatcher.addPath(root + "/" + filename);
    weatherNames = parser.readCDefineNames(filename, prefixes);
    if (weatherNames.isEmpty()) {
        logError(QString("Failed to read weather constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readCoordEventWeatherNames() {
    if (!projectConfig.getEventWeatherTriggerEnabled())
        return true;

    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_coord_event_weather)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    fileWatcher.addPath(root + "/" + filename);
    coordEventWeatherNames = parser.readCDefineNames(filename, prefixes);
    if (coordEventWeatherNames.isEmpty()) {
        logWarn(QString("Failed to read coord event weather constants from %1. Disabling Weather Trigger events.").arg(filename));
        projectConfig.setEventWeatherTriggerEnabled(false);
    }
    return true;
}

bool Project::readSecretBaseIds() {
    if (!projectConfig.getEventSecretBaseEnabled())
        return true;

    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_secret_bases)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_secret_bases);
    fileWatcher.addPath(root + "/" + filename);
    secretBaseIds = parser.readCDefineNames(filename, prefixes);
    if (secretBaseIds.isEmpty()) {
        logWarn(QString("Failed to read secret base id constants from '%1'. Disabling Secret Base events.").arg(filename));
        projectConfig.setEventSecretBaseEnabled(false);
    }
    return true;
}

bool Project::readBgEventFacingDirections() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_sign_facing_directions)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_event_bg);
    fileWatcher.addPath(root + "/" + filename);
    bgEventFacingDirections = parser.readCDefineNames(filename, prefixes);
    if (bgEventFacingDirections.isEmpty()) {
        logError(QString("Failed to read bg event facing direction constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readTrainerTypes() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_trainer_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_trainer_types);
    fileWatcher.addPath(root + "/" + filename);
    trainerTypes = parser.readCDefineNames(filename, prefixes);
    if (trainerTypes.isEmpty()) {
        logError(QString("Failed to read trainer type constants from %1").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();

    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_behaviors)};
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_behaviors);
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> defines = parser.readCDefinesByPrefix(filename, prefixes);
    if (defines.isEmpty()) {
        // Not having any metatile behavior names is ok (their values will be displayed instead).
        // If the user's metatiles can have nonzero values then warn them, as they likely want names.
        if (projectConfig.getMetatileBehaviorMask())
            logWarn(QString("Failed to read metatile behaviors from %1.").arg(filename));
        return true;
    }

    for (auto i = defines.cbegin(), end = defines.cend(); i != end; i++) {
        uint32_t value = static_cast<uint32_t>(i.value());
        this->metatileBehaviorMap.insert(i.key(), value);
        this->metatileBehaviorMapInverse.insert(value, i.key());
    }

    return true;
}

bool Project::readSongNames() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_music)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_songs);
    fileWatcher.addPath(root + "/" + filename);
    this->songNames = parser.readCDefineNames(filename, prefixes);
    if (this->songNames.isEmpty()) {
        logError(QString("Failed to read song names from %1.").arg(filename));
        return false;
    }
    this->defaultSong = this->songNames.value(0);
    // Song names don't have a very useful order (esp. if we include SE_* values), so sort them alphabetically.
    this->songNames.sort();
    return true;
}

bool Project::readObjEventGfxConstants() {
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_obj_event_gfx)};
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_events);
    fileWatcher.addPath(root + "/" + filename);
    this->gfxDefines = parser.readCDefinesByPrefix(filename, prefixes);
    if (this->gfxDefines.isEmpty()) {
        logError(QString("Failed to read object event graphics constants from %1.").arg(filename));
        return false;
    }
    return true;
}

bool Project::readMiscellaneousConstants() {
    miscConstants.clear();
    if (userConfig.getEncounterJsonActive()) {
        const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_pokemon);
        const QString minLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_min_level);
        const QString maxLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_max_level);
        fileWatcher.addPath(root + "/" + filename);
        QMap<QString, int> pokemonDefines = parser.readCDefinesByName(filename, {minLevelName, maxLevelName});
        miscConstants.insert("max_level_define", pokemonDefines.value(maxLevelName) > pokemonDefines.value(minLevelName) ? pokemonDefines.value(maxLevelName) : 100);
        miscConstants.insert("min_level_define", pokemonDefines.value(minLevelName) < pokemonDefines.value(maxLevelName) ? pokemonDefines.value(minLevelName) : 1);
    }

    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_global);
    const QString maxObjectEventsName = projectConfig.getIdentifier(ProjectIdentifier::define_obj_event_count);
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> defines = parser.readCDefinesByName(filename, {maxObjectEventsName});

    auto it = defines.find(maxObjectEventsName);
    if (it != defines.end()) {
        if (it.value() > 0) {
            Project::max_object_events = it.value();
        } else {
            logWarn(QString("Value for '%1' is %2, must be greater than 0. Using default (%3) instead.")
                    .arg(maxObjectEventsName)
                    .arg(it.value())
                    .arg(Project::max_object_events));
        }
    }
    else {
        logWarn(QString("Value for '%1' not found. Using default (%2) instead.")
                .arg(maxObjectEventsName)
                .arg(Project::max_object_events));
    }

    return true;
}

QStringList Project::getGlobalScriptLabels() {
    return this->eventScriptLabelModel.stringList();
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
    static const QRegularExpression re_gbapal("\\.gbapal$");
    path = path.replace(re_gbapal, ".pal");
    return path;
}

QString Project::fixGraphicPath(QString path) {
    static const QRegularExpression re_lz("\\.lz$");
    path = path.replace(re_lz, "");
    static const QRegularExpression re_bpp("\\.[1248]bpp$");
    path = path.replace(re_bpp, ".png");
    return path;
}

QString Project::getScriptFileExtension(bool usePoryScript) {
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

QStringList Project::getEventScriptsFilePaths() const {
    QStringList filePaths(QDir::cleanPath(root + "/" + projectConfig.getFilePath(ProjectFilePath::data_event_scripts)));
    const QString scriptsDir = QDir::cleanPath(root + "/" + projectConfig.getFilePath(ProjectFilePath::data_scripts_folders));
    const QString mapsDir = QDir::cleanPath(root + "/" + projectConfig.getFilePath(ProjectFilePath::data_map_folders));
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

void Project::setEventPixmap(Event *event, bool forceLoad) {
    if (event && (event->getPixmap().isNull() || forceLoad))
        event->loadPixmap(this);
}

bool Project::readEventGraphics() {
    fileWatcher.addPaths(QStringList() << root + "/" + projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_pointers)
                                       << root + "/" + projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_info)
                                       << root + "/" + projectConfig.getFilePath(ProjectFilePath::data_obj_event_pic_tables)
                                       << root + "/" + projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx));

    const QString pointersFilepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_pointers);
    const QString pointersName = projectConfig.getIdentifier(ProjectIdentifier::symbol_obj_event_gfx_pointers);
    QMap<QString, QString> pointerHash = parser.readNamedIndexCArray(pointersFilepath, pointersName);

    qDeleteAll(eventGraphicsMap);
    eventGraphicsMap.clear();
    QStringList gfxNames = gfxDefines.keys();

    // The positions of each of the required members for the gfx info struct.
    // For backwards compatibility if the struct doesn't use initializers.
    static const auto gfxInfoMemberMap = QHash<int, QString>{
        {8, "inanimate"},
        {11, "oam"},
        {12, "subspriteTables"},
        {14, "images"},
    };

    QString filepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_info);
    const auto gfxInfos = parser.readCStructs(filepath, "", gfxInfoMemberMap);

    QMap<QString, QStringList> picTables = parser.readCArrayMulti(projectConfig.getFilePath(ProjectFilePath::data_obj_event_pic_tables));
    QMap<QString, QString> graphicIncbins = parser.readCIncbinMulti(projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx));

    for (QString gfxName : gfxNames) {
        EventGraphics * eventGraphics = new EventGraphics;

        QString info_label = pointerHash[gfxName].replace("&", "");
        if (!gfxInfos.contains(info_label))
            continue;

        const auto gfxInfoAttributes = gfxInfos[info_label];

        eventGraphics->inanimate = ParseUtil::gameStringToBool(gfxInfoAttributes.value("inanimate"));
        QString pic_label = gfxInfoAttributes.value("images");
        QString dimensions_label = gfxInfoAttributes.value("oam");
        QString subsprites_label = gfxInfoAttributes.value("subspriteTables");

        QString gfx_label = picTables[pic_label].value(0);
        static const QRegularExpression re_parens("[\\(\\)]");
        gfx_label = gfx_label.section(re_parens, 1, 1);
        QString path = graphicIncbins[gfx_label];

        if (!path.isNull()) {
            path = fixGraphicPath(path);
            eventGraphics->spritesheet = QImage(root + "/" + path);
            if (!eventGraphics->spritesheet.isNull()) {
                // Infer the sprite dimensions from the OAM labels.
                static const QRegularExpression re("\\S+_(\\d+)x(\\d+)");
                QRegularExpressionMatch dimensionMatch = re.match(dimensions_label);
                QRegularExpressionMatch oamTablesMatch = re.match(subsprites_label);
                if (oamTablesMatch.hasMatch()) {
                    eventGraphics->spriteWidth = oamTablesMatch.captured(1).toInt(nullptr, 0);
                    eventGraphics->spriteHeight = oamTablesMatch.captured(2).toInt(nullptr, 0);
                } else if (dimensionMatch.hasMatch()) {
                    eventGraphics->spriteWidth = dimensionMatch.captured(1).toInt(nullptr, 0);
                    eventGraphics->spriteHeight = dimensionMatch.captured(2).toInt(nullptr, 0);
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

bool Project::readSpeciesIconPaths() {
    this->speciesToIconPath.clear();

    // Read map of species constants to icon names
    const QString srcfilename = projectConfig.getFilePath(ProjectFilePath::pokemon_icon_table);
    fileWatcher.addPath(root + "/" + srcfilename);
    const QString tableName = projectConfig.getIdentifier(ProjectIdentifier::symbol_pokemon_icon_table);
    const QMap<QString, QString> monIconNames = parser.readNamedIndexCArray(srcfilename, tableName);

    // Read map of icon names to filepaths
    const QString incfilename = projectConfig.getFilePath(ProjectFilePath::data_pokemon_gfx);
    fileWatcher.addPath(root + "/" + incfilename);
    const QMap<QString, QString> iconIncbins = parser.readCIncbinMulti(incfilename);

    // Read species constants. If this fails we can get them from the icon table (but we shouldn't rely on it).
    const QStringList prefixes = {projectConfig.getIdentifier(ProjectIdentifier::regex_species)};
    const QString constantsFilename = projectConfig.getFilePath(ProjectFilePath::constants_species);
    fileWatcher.addPath(root + "/" + constantsFilename);
    QStringList speciesNames = parser.readCDefineNames(constantsFilename, prefixes);
    if (speciesNames.isEmpty())
        speciesNames = monIconNames.keys();

    // For each species, use the information gathered above to find the icon image.
    bool missingIcons = false;
    for (auto species : speciesNames) {
        QString path = QString();
        if (monIconNames.contains(species) && iconIncbins.contains(monIconNames.value(species))) {
            // We have the icon filepath from the icon table
            path = QString("%1/%2").arg(root).arg(this->fixGraphicPath(iconIncbins[monIconNames.value(species)]));
        } else {
            // Failed to read icon filepath from the icon table, check filepaths where icons are normally located.
            // Try to use the icon name (if we have it) to determine the directory, then try the species name.
            // The name permuting is overkill, but it's making up for some of the fragility in the way we find icon paths.
            QStringList possibleDirNames;
            if (monIconNames.contains(species)) {
                // Ex: For 'gMonIcon_QuestionMark' try 'question_mark'
                static const QRegularExpression re("([a-z])([A-Z0-9])");
                QString iconName = monIconNames.value(species);
                iconName = iconName.mid(iconName.indexOf("_") + 1); // jump past prefix ('gMonIcon')
                possibleDirNames.append(iconName.replace(re, "\\1_\\2").toLower());
            }

            // Ex: For 'SPECIES_FOO_BAR_BAZ' try 'foo_bar_baz'
            possibleDirNames.append(species.mid(8).toLower());

            // Permute paths with underscores.
            // Ex: Try 'foo_bar/baz', 'foo/bar_baz', 'foobarbaz', 'foo_bar', and 'foo'
            QStringList permutedNames;
            for (auto dir : possibleDirNames) {
                if (!dir.contains("_")) continue;
                for (int i = dir.indexOf("_"); i > -1; i = dir.indexOf("_", i + 1)) {
                    QString temp = dir;
                    permutedNames.prepend(temp.replace(i, 1, "/"));
                    permutedNames.append(dir.left(i)); // Prepend the others so the most generic name ('foo') ends up last
                }
                permutedNames.prepend(dir.remove("_"));
            }
            possibleDirNames.append(permutedNames);

            possibleDirNames.removeDuplicates();
            for (auto dir : possibleDirNames) {
                if (dir.isEmpty()) continue;
                const QString stdPath = QString("%1/%2%3/icon.png")
                                                .arg(root)
                                                .arg(projectConfig.getFilePath(ProjectFilePath::pokemon_gfx))
                                                .arg(dir);
                if (QFile::exists(stdPath)) {
                    // Icon found at a normal filepath
                    path = stdPath;
                    break;
                }
            }

            if (path.isEmpty() && projectConfig.getPokemonIconPath(species).isEmpty()) {
                // Failed to find icon, this species will use a placeholder icon.
                logWarn(QString("Failed to find Pokémon icon for '%1'").arg(species));
                missingIcons = true;
            }
        }
        this->speciesToIconPath.insert(species, path);
    }

    // Logging this alongside every warning (if there are multiple) is obnoxious, just do it once at the end.
    if (missingIcons) logInfo("Pokémon icon filepaths can be specified under 'Options->Project Settings'");

    return true;
}

void Project::setNewMapEvents(Map *map) {
    map->events[Event::Group::Object].clear();
    map->events[Event::Group::Warp].clear();
    map->events[Event::Group::Heal].clear();
    map->events[Event::Group::Coord].clear();
    map->events[Event::Group::Bg].clear();
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
    return Block::getMaxMetatileId() + 1;
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
        logError(QString("'%1' of %2 is too small to support a 1x1 map. Must be at least %3.")
                    .arg(projectConfig.getIdentifier(ProjectIdentifier::define_map_size))
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

QString Project::getDynamicMapDefineName() {
    const QString prefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
    return prefix + projectConfig.getIdentifier(ProjectIdentifier::define_map_dynamic);
}

void Project::setImportExportPath(QString filename)
{
    this->importExportPath = QFileInfo(filename).absolutePath();
}

// If the provided filepath is an absolute path to an existing file, return filepath.
// If not, and the provided filepath is a relative path from the project dir to an existing file, return the relative path.
// Otherwise return empty string.
QString Project::getExistingFilepath(QString filepath) {
    if (filepath.isEmpty() || QFile::exists(filepath))
        return filepath;

    filepath = QDir::cleanPath(projectConfig.getProjectDir() + QDir::separator() + filepath);
    if (QFile::exists(filepath))
        return filepath;

    return QString();
}

// The values of some config fields can limit the values of other config fields
// (for example, metatile attributes size limits the metatile attribute masks).
// Others depend on information in the project (for example the default metatile ID
// can be limited by fieldmap defines)
// Once we've read data from the project files we can adjust these accordingly.
void Project::applyParsedLimits() {
    // Avoid repeatedly writing the config file
    projectConfig.setSaveDisabled(true);

    uint32_t maxMask = Metatile::getMaxAttributesMask();
    projectConfig.setMetatileBehaviorMask(projectConfig.getMetatileBehaviorMask() & maxMask);
    projectConfig.setMetatileTerrainTypeMask(projectConfig.getMetatileTerrainTypeMask() & maxMask);
    projectConfig.setMetatileEncounterTypeMask(projectConfig.getMetatileEncounterTypeMask() & maxMask);
    projectConfig.setMetatileLayerTypeMask(projectConfig.getMetatileLayerTypeMask() & maxMask);

    Block::setLayout();
    Metatile::setLayout(this);

    Project::num_metatiles_primary = qMin(Project::num_metatiles_primary, Block::getMaxMetatileId() + 1);
    projectConfig.setDefaultMetatileId(qMin(projectConfig.getDefaultMetatileId(), Block::getMaxMetatileId()));
    projectConfig.setDefaultElevation(qMin(projectConfig.getDefaultElevation(), Block::getMaxElevation()));
    projectConfig.setDefaultCollision(qMin(projectConfig.getDefaultCollision(), Block::getMaxCollision()));
    projectConfig.setCollisionSheetHeight(qMin(projectConfig.getCollisionSheetHeight(), Block::getMaxElevation() + 1));
    projectConfig.setCollisionSheetWidth(qMin(projectConfig.getCollisionSheetWidth(), Block::getMaxCollision() + 1));

    projectConfig.setSaveDisabled(false);
    projectConfig.save();
}
