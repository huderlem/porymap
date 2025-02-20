#include "project.h"
#include "config.h"
#include "history.h"
#include "log.h"
#include "parseutil.h"
#include "paletteutil.h"
#include "tile.h"
#include "tileset.h"
#include "map.h"
#include "filedialog.h"
#include "validator.h"
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

int Project::num_tiles_primary = 512;
int Project::num_tiles_total = 1024;
int Project::num_metatiles_primary = 512;
int Project::num_pals_primary = 6;
int Project::num_pals_total = 13;
int Project::max_map_data_size = 10240; // 0x2800
int Project::default_map_dimension = 20;

Project::Project(QObject *parent) :
    QObject(parent)
{
    QObject::connect(&this->fileWatcher, &QFileSystemWatcher::fileChanged, this, &Project::recordFileChange);
}

Project::~Project()
{
    clearMapCache();
    clearTilesetCache();
    clearMapLayouts();
    clearEventGraphics();
    clearHealLocations();
}

void Project::set_root(QString dir) {
    this->root = dir;
    FileDialog::setDirectory(dir);
    this->parser.set_root(dir);
}

// Before attempting the initial project load we should check for a few notable files.
// If all are missing then we can warn the user, they may have accidentally selected the wrong folder.
bool Project::sanityCheck() {
    // The goal with the file selection is to pick files that are important enough that any reasonable project would have
    // at least 1 in the expected location, but unique enough that they're unlikely to overlap with a completely unrelated
    // directory (e.g. checking for 'data/maps/' is a bad choice because it's too generic, pokeyellow would pass for instance)
    static const QSet<ProjectFilePath> pathsToCheck = {
        ProjectFilePath::json_map_groups,
        ProjectFilePath::json_layouts,
        ProjectFilePath::tilesets_headers,
        ProjectFilePath::global_fieldmap,
    };
    for (auto pathId : pathsToCheck) {
        const QString path = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(pathId));
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isFile())
            return true;
    }
    return false;
}

bool Project::load() {
    this->disabledSettingsNames.clear();
    bool success = readMapLayouts()
                && readRegionMapSections()
                && readItemNames()
                && readFlagNames()
                && readVarNames()
                && readMovementTypes()
                && readInitialFacingDirections()
                && readMapTypes()
                && readMapBattleScenes()
                && readWeatherNames()
                && readCoordEventWeatherNames()
                && readSecretBaseIds() 
                && readBgEventFacingDirections()
                && readTrainerTypes()
                && readMetatileBehaviors()
                && readFieldmapProperties()
                && readFieldmapMasks()
                && readTilesetLabels()
                && readTilesetMetatileLabels()
                && readMiscellaneousConstants()
                && readSpeciesIconPaths()
                && readWildMonData()
                && readEventScriptLabels()
                && readObjEventGfxConstants()
                && readEventGraphics()
                && readSongNames()
                && readMapGroups()
                && readHealLocations();

    if (success) {
        // No need to do this if something failed to load.
        // (and in fact we shouldn't, because they contain
        //  assumptions that some things have loaded correctly).
        initNewLayoutSettings();
        initNewMapSettings();
        applyParsedLimits();
    }
    return success;
}

QString Project::getProjectTitle() const {
    if (!root.isNull()) {
        return root.section('/', -1);
    } else {
        return QString();
    }
}

void Project::clearMapCache() {
    qDeleteAll(this->mapCache);
    this->mapCache.clear();
}

void Project::clearTilesetCache() {
    qDeleteAll(this->tilesetCache);
    this->tilesetCache.clear();
}

Map* Project::loadMap(QString mapName) {
    if (mapName == getDynamicMapName())
        return nullptr;

    Map *map;
    if (mapCache.contains(mapName)) {
        map = mapCache.value(mapName);
        // TODO: uncomment when undo/redo history is fully implemented for all actions.
        if (true/*map->hasUnsavedChanges()*/) {
            return map;
        }
    } else {
        map = new Map;
        map->setName(mapName);
    }

    if (!(loadMapData(map) && loadMapLayout(map))){
        delete map;
        return nullptr;
    }

    // If the map's MAPSEC value in the header changes, update our global array to keep it in sync.
    connect(map->header(), &MapHeader::locationChanged, [this, map] { this->mapNameToMapSectionName.insert(map->name(), map->header()->location()); });

    mapCache.insert(mapName, map);
    emit mapLoaded(map);
    return map;
}

void Project::initTopLevelMapFields() {
    static const QSet<QString> defaultTopLevelMapFields = {
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
    this->topLevelMapFields = defaultTopLevelMapFields;
    if (projectConfig.mapAllowFlagsEnabled) {
        this->topLevelMapFields.insert("allow_cycling");
        this->topLevelMapFields.insert("allow_escaping");
        this->topLevelMapFields.insert("allow_running");
    }
    if (projectConfig.floorNumberEnabled) {
        this->topLevelMapFields.insert("floor_number");
    }
}

bool Project::readMapJson(const QString &mapName, QJsonDocument * out) {
    const QString mapFilepath = QString("%1%2/map.json").arg(projectConfig.getFilePath(ProjectFilePath::data_map_folders)).arg(mapName);
    if (!parser.tryParseJsonFile(out, QString("%1/%2").arg(this->root).arg(mapFilepath))) {
        logError(QString("Failed to read map data from %1").arg(mapFilepath));
        return false;
    }
    return true;
}

bool Project::loadMapEvent(Map *map, const QJsonObject &json, Event::Type defaultType) {
    QString typeString = ParseUtil::jsonToQString(json["type"]);
    Event::Type type = typeString.isEmpty() ? defaultType : Event::typeFromString(typeString);
    Event* event = Event::create(type);
    if (!event) {
        return false;
    }
    if (!event->loadFromJson(json, this)) {
        delete event;
        return false;
    }
    map->addEvent(event);
    return true;
}

bool Project::loadMapData(Map* map) {
    if (!map->isPersistedToFile()) {
        return true;
    }

    QJsonDocument mapDoc;
    if (!readMapJson(map->name(), &mapDoc))
        return false;

    QJsonObject mapObj = mapDoc.object();

    // We should already know the map constant ID from the initial project launch, but we'll ensure it's correct here anyway.
    map->setConstantName(ParseUtil::jsonToQString(mapObj["id"]));
    this->mapNamesToMapConstants.insert(map->name(), map->constantName());
    this->mapConstantsToMapNames.insert(map->constantName(), map->name());

    map->header()->setSong(ParseUtil::jsonToQString(mapObj["music"]));
    map->setLayoutId(ParseUtil::jsonToQString(mapObj["layout"]));
    map->header()->setLocation(ParseUtil::jsonToQString(mapObj["region_map_section"]));
    map->header()->setRequiresFlash(ParseUtil::jsonToBool(mapObj["requires_flash"]));
    map->header()->setWeather(ParseUtil::jsonToQString(mapObj["weather"]));
    map->header()->setType(ParseUtil::jsonToQString(mapObj["map_type"]));
    map->header()->setShowsLocationName(ParseUtil::jsonToBool(mapObj["show_map_name"]));
    map->header()->setBattleScene(ParseUtil::jsonToQString(mapObj["battle_scene"]));

    if (projectConfig.mapAllowFlagsEnabled) {
        map->header()->setAllowsBiking(ParseUtil::jsonToBool(mapObj["allow_cycling"]));
        map->header()->setAllowsEscaping(ParseUtil::jsonToBool(mapObj["allow_escaping"]));
        map->header()->setAllowsRunning(ParseUtil::jsonToBool(mapObj["allow_running"]));
    }
    if (projectConfig.floorNumberEnabled) {
        map->header()->setFloorNumber(ParseUtil::jsonToInt(mapObj["floor_number"]));
    }
    map->setSharedEventsMap(ParseUtil::jsonToQString(mapObj["shared_events_map"]));
    map->setSharedScriptsMap(ParseUtil::jsonToQString(mapObj["shared_scripts_map"]));

    // Events
    map->resetEvents();
    static const QMap<QString, Event::Type> defaultEventTypes = {
        // Map of the expected keys for each event group, and the default type of that group.
        // If the default type is Type::None then each event must specify its type, or its an error.
        {"object_events", Event::Type::Object},
        {"warp_events",   Event::Type::Warp},
        {"coord_events",  Event::Type::None},
        {"bg_events",     Event::Type::None},
    };
    for (auto i = defaultEventTypes.constBegin(); i != defaultEventTypes.constEnd(); i++) {
        QString eventGroupKey = i.key();
        Event::Type defaultType = i.value();
        const QJsonArray eventsJsonArr = mapObj[eventGroupKey].toArray();
        for (int i = 0; i < eventsJsonArr.size(); i++) {
            if (!loadMapEvent(map, eventsJsonArr.at(i).toObject(), defaultType)) {
                logError(QString("Failed to load event for %1, in %2 at index %3.").arg(map->name()).arg(eventGroupKey).arg(i));
            }
        }
    }

    // Heal locations are global. Populate the Map's heal location events using our global array.
    const QList<HealLocationEvent*> hlEvents = this->healLocations.value(map->constantName());
    for (const auto &event : hlEvents) {
        map->addEvent(event->duplicate());
    }

    map->deleteConnections();
    QJsonArray connectionsArr = mapObj["connections"].toArray();
    if (!connectionsArr.isEmpty()) {
        for (int i = 0; i < connectionsArr.size(); i++) {
            QJsonObject connectionObj = connectionsArr[i].toObject();
            const QString direction = ParseUtil::jsonToQString(connectionObj["direction"]);
            const int offset = ParseUtil::jsonToInt(connectionObj["offset"]);
            const QString mapConstant = ParseUtil::jsonToQString(connectionObj["map"]);
            map->loadConnection(new MapConnection(this->mapConstantsToMapNames.value(mapConstant, mapConstant), direction, offset));
        }
    }

    QMap<QString, QJsonValue> customAttributes;
    for (auto i = mapObj.constBegin(); i != mapObj.constEnd(); i++) {
        if (!this->topLevelMapFields.contains(i.key())) {
            customAttributes.insert(i.key(), i.value());
        }
    }
    map->setCustomAttributes(customAttributes);

    return true;
}

Map *Project::createNewMap(const Project::NewMapSettings &settings, const Map* toDuplicate) {
    Map *map = toDuplicate ? new Map(*toDuplicate) : new Map;
    map->setName(settings.name);
    map->setHeader(settings.header);
    map->setNeedsHealLocation(settings.canFlyTo);

    // Generate a unique MAP constant.
    map->setConstantName(toUniqueIdentifier(Map::mapConstantFromName(map->name())));

    Layout *layout = this->mapLayouts.value(settings.layout.id);
    if (!layout) {
        // Layout doesn't already exist, create it.
        layout = createNewLayout(settings.layout, toDuplicate ? toDuplicate->layout() : nullptr);
        if (!layout) {
            // Layout creation failed.
            delete map;
            return nullptr;
        }
    } else {
        // This layout already exists. Make sure it's loaded.
        loadLayout(layout);
    }
    map->setLayout(layout);

    // Make sure we keep the order of the map names the same as in the map group order.
    int mapNamePos;
    if (this->groupNames.contains(settings.group)) {
        mapNamePos = 0;
        for (const auto &name : this->groupNames) {
            mapNamePos += this->groupNameToMapNames[name].length();
            if (name == settings.group)
                break;
        }
    } else {
        // Adding map to a map group that doesn't exist yet.
        // Create the group, and we already know the map will be last in the list.
        if (isValidNewIdentifier(settings.group)) {
            addNewMapGroup(settings.group);
        }
        mapNamePos = this->mapNames.length();
    }

    const QString location = map->header()->location();
    if (!this->mapSectionIdNames.contains(location) && isValidNewIdentifier(location)) {
        // Unrecognized MAPSEC name, we can automatically add a new MAPSEC for it.
        addNewMapsec(location);
    }

    this->mapNames.insert(mapNamePos, map->name());
    this->groupNameToMapNames[settings.group].append(map->name());
    this->mapConstantsToMapNames.insert(map->constantName(), map->name());
    this->mapNamesToMapConstants.insert(map->name(), map->constantName());
    this->mapNameToLayoutId.insert(map->name(), map->layoutId());
    this->mapNameToMapSectionName.insert(map->name(), map->header()->location());

    map->setIsPersistedToFile(false);
    this->mapCache.insert(map->name(), map);

    emit mapCreated(map, settings.group);

    return map;
}

Layout *Project::createNewLayout(const Layout::Settings &settings, const Layout *toDuplicate) {
    if (this->layoutIds.contains(settings.id))
        return nullptr;

    Layout *layout = toDuplicate ? new Layout(*toDuplicate) : new Layout();
    layout->id = settings.id;
    layout->name = settings.name;
    layout->width = settings.width;
    layout->height = settings.height;
    layout->border_width = settings.borderWidth;
    layout->border_height = settings.borderHeight;
    layout->tileset_primary_label = settings.primaryTilesetLabel;
    layout->tileset_secondary_label = settings.secondaryTilesetLabel;

    // If a special folder name was specified (as in the case when we're creating a layout for a new map) then use that name.
    // Otherwise the new layout's folder name will just be the layout's name.
    const QString folderName = !settings.folderName.isEmpty() ? settings.folderName : layout->name;
    const QString folderPath = projectConfig.getFilePath(ProjectFilePath::data_layouts_folders) + folderName;
    layout->border_path = folderPath + "/border.bin";
    layout->blockdata_path = folderPath + "/map.bin";

    // Create a new directory for the layout, if it doesn't already exist.
    const QString fullPath = QString("%1/%2").arg(this->root).arg(folderPath);
    if (!QDir::root().mkpath(fullPath)) {
        logError(QString("Failed to create directory for new layout: '%1'").arg(fullPath));
        delete layout;
        return nullptr;
    }

    this->mapLayouts.insert(layout->id, layout);
    this->layoutIds.append(layout->id);

    if (layout->blockdata.isEmpty()) {
        // Fill layout using default fill settings
        setNewLayoutBlockdata(layout);
    }
    if (layout->border.isEmpty()) {
        // Fill border using default fill settings
        setNewLayoutBorder(layout);
    }

    saveLayout(layout); // TODO: Ideally we shouldn't automatically save new layouts

    emit layoutCreated(layout);

    return layout;
}

bool Project::loadLayout(Layout *layout) {
    if (!layout->loaded) {
        // Force these to run even if one fails
        bool loadedTilesets = loadLayoutTilesets(layout);
        bool loadedBlockdata = loadBlockdata(layout);
        bool loadedBorder = loadLayoutBorder(layout);

        if (loadedTilesets && loadedBlockdata && loadedBorder) {
            layout->loaded = true;
            return true;
        } else {
            return false;
        }
    }
    return true;
}

Layout *Project::loadLayout(QString layoutId) {
    Layout *layout = this->mapLayouts.value(layoutId);
    if (!layout || !loadLayout(layout)) {
        logError(QString("Failed to load layout '%1'").arg(layoutId));
        return nullptr;
    }
    return layout;
}

bool Project::loadMapLayout(Map* map) {
    if (!map->isPersistedToFile()) {
        return true;
    }

    Layout *layout = this->mapLayouts.value(map->layoutId());
    if (!layout) {
        logError(QString("Map '%1' has an unknown layout '%2'").arg(map->name()).arg(map->layoutId()));
        return false;
    }
    map->setLayout(layout);

    if (map->hasUnsavedChanges()) {
        return true;
    } else {
        return loadLayout(map->layout());
    }
}

void Project::clearMapLayouts() {
    qDeleteAll(this->mapLayouts);
    this->mapLayouts.clear();
    qDeleteAll(this->mapLayoutsMaster);
    this->mapLayoutsMaster.clear();
    this->layoutIds.clear();
    this->layoutIdsMaster.clear();
}

bool Project::readMapLayouts() {
    clearMapLayouts();

    const QString layoutsFilepath = projectConfig.getFilePath(ProjectFilePath::json_layouts);
    const QString fullFilepath = QString("%1/%2").arg(this->root).arg(layoutsFilepath);
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

    this->layoutsLabel = ParseUtil::jsonToQString(layoutsObj["layouts_table_label"]);
    if (this->layoutsLabel.isEmpty()) {
        this->layoutsLabel = "gMapLayouts";
        logWarn(QString("'layouts_table_label' value is missing from %1. Defaulting to %2")
                 .arg(layoutsFilepath)
                 .arg(layoutsLabel));
    }

    for (int i = 0; i < layouts.size(); i++) {
        QJsonObject layoutObj = layouts[i].toObject();
        if (layoutObj.isEmpty())
            continue;
        Layout *layout = new Layout();
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
        if (projectConfig.useCustomBorderSize) {
            int bwidth = ParseUtil::jsonToInt(layoutObj["border_width"]);
            if (bwidth <= 0) {  // 0 is an expected border width/height that should be handled, GF used it for the RS layouts in FRLG
                bwidth = DEFAULT_BORDER_WIDTH;
            }
            layout->border_width = bwidth;
            int bheight = ParseUtil::jsonToInt(layoutObj["border_height"]);
            if (bheight <= 0) {
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

        this->mapLayouts.insert(layout->id, layout);
        this->mapLayoutsMaster.insert(layout->id, layout->copy());
        this->layoutIds.append(layout->id);
        this->layoutIdsMaster.append(layout->id);
    }

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
    layoutsObj["layouts_table_label"] = this->layoutsLabel;

    OrderedJson::array layoutsArr;
    for (const QString &layoutId : this->layoutIdsMaster) {
        Layout *layout = this->mapLayoutsMaster.value(layoutId);
        OrderedJson::object layoutObj;
        layoutObj["id"] = layout->id;
        layoutObj["name"] = layout->name;
        layoutObj["width"] = layout->width;
        layoutObj["height"] = layout->height;
        if (projectConfig.useCustomBorderSize) {
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
    this->modifiedFileTimestamps.insert(filepath, QDateTime::currentMSecsSinceEpoch() + 5000);
}

void Project::recordFileChange(const QString &filepath) {
    if (this->modifiedFiles.contains(filepath)) {
        // We already recorded a change to this file
        return;
    }

    if (this->modifiedFileTimestamps.contains(filepath)) {
        if (QDateTime::currentMSecsSinceEpoch() < this->modifiedFileTimestamps[filepath]) {
            // We're still ignoring changes to this file
            return;
        }
        this->modifiedFileTimestamps.remove(filepath);
    }

    this->modifiedFiles.insert(filepath);
    emit fileChanged(filepath);
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

    for (const auto &groupName : this->groupNames) {
        OrderedJson::array groupArr;
        for (const auto &mapName : this->groupNameToMapNames.value(groupName)) {
            groupArr.push_back(mapName);
        }
        mapGroupsObj[groupName] = groupArr;
    }

    ignoreWatchedFileTemporarily(mapGroupsFilepath);

    OrderedJson mapGroupJson(mapGroupsObj);
    OrderedJsonDoc jsonDoc(&mapGroupJson);
    jsonDoc.dump(&mapGroupsFile);
    mapGroupsFile.close();
}

void Project::saveRegionMapSections() {
    const QString filepath = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(ProjectFilePath::json_region_map_entries));
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing").arg(filepath));
        return;
    }

    const QString emptyMapsecName = getEmptyMapsecName();
    OrderedJson::array mapSectionArray;
    for (const auto &idName : this->mapSectionIdNamesSaveOrder) {
        OrderedJson::object mapSectionObj;
        mapSectionObj["id"] = idName;

        if (this->mapSectionDisplayNames.contains(idName)) {
            mapSectionObj["name"] = this->mapSectionDisplayNames.value(idName);
        }

        if (this->regionMapEntries.contains(idName)) {
            MapSectionEntry entry = this->regionMapEntries.value(idName);
            mapSectionObj["x"] = entry.x;
            mapSectionObj["y"] = entry.y;
            mapSectionObj["width"] = entry.width;
            mapSectionObj["height"] = entry.height;
        }

        mapSectionArray.append(mapSectionObj);
    }

    OrderedJson::object object;
    object["map_sections"] = mapSectionArray;

    ignoreWatchedFileTemporarily(filepath);
    OrderedJson json(object);
    OrderedJsonDoc jsonDoc(&json);
    jsonDoc.dump(&file);
    file.close();
}

void Project::saveWildMonData() {
    if (!this->wildEncountersLoaded) return;

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

// For a map with a constant of 'MAP_FOO', returns a unique 'HEAL_LOCATION_FOO'.
// Because of how event ID names are checked it doesn't guarantee that the name
// won't be in-use by some map that hasn't been loaded yet.
QString Project::getNewHealLocationName(const Map* map) const {
    if (!map) return QString();

    QString idName = map->constantName();
    const QString mapPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
    if (idName.startsWith(mapPrefix)) {
        idName.remove(0, mapPrefix.length());
    }
    return toUniqueIdentifier(projectConfig.getIdentifier(ProjectIdentifier::define_heal_locations_prefix) + idName);
}

void Project::saveHealLocations() {
    const QString filepath = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(ProjectFilePath::json_heal_locations));
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing").arg(filepath));
        return;
    }

    // Build the JSON data for output.
    QMap<QString, QList<OrderedJson::object>> idNameToJson;
    for (auto i = this->healLocations.constBegin(); i != this->healLocations.constEnd(); i++) {
        const QString mapConstant = i.key();
        for (const auto &event : i.value()) {
            // Heal location events don't need to track the "map" field, we're already tracking it either with
            // the keys in the healLocations map or by virtue of the event being added to a particular Map object.
            // The global JSON data needs this field, so we add it back here.
            auto eventJson = event->buildEventJson(this);
            eventJson["map"] = mapConstant;
            idNameToJson[event->getIdName()].append(eventJson);
        }
    }

    // We store Heal Locations in a QMap with map name keys. This makes retrieval by map name easy,
    // but it will sort them alphabetically. Project::healLocationSaveOrder lets us better support
    // the (perhaps unlikely) user who has designed something with assumptions about the order of the data.
    // This also avoids a bunch of diff noise on the first save from Porymap reordering the data.
    OrderedJson::array eventJsonArr;
    for (const auto &idName : this->healLocationSaveOrder) {
        if (!idNameToJson.value(idName).isEmpty()) {
            eventJsonArr.push_back(idNameToJson[idName].takeFirst());
        }
    }
    // Save any heal locations that weren't covered above (should be any new data).
    for (auto i = idNameToJson.constBegin(); i != idNameToJson.constEnd(); i++) {
        for (const auto &object : i.value()) {
            eventJsonArr.push_back(object);
        }
    }

    OrderedJson::object object;
    object["heal_locations"] = eventJsonArr;

    ignoreWatchedFileTemporarily(filepath);
    OrderedJson json(object);
    OrderedJsonDoc jsonDoc(&json);
    jsonDoc.dump(&file);
    file.close();
}

void Project::saveTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    saveTilesetMetatileLabels(primaryTileset, secondaryTileset);
    if (primaryTileset)
        primaryTileset->save();
    if (secondaryTileset)
        secondaryTileset->save();
}

void Project::updateTilesetMetatileLabels(Tileset *tileset) {
    // Erase old labels, then repopulate with new labels
    const QString prefix = tileset->getMetatileLabelPrefix();
    this->metatileLabelsMap[tileset->name].clear();
    for (int metatileId : tileset->metatileLabels.keys()) {
        if (tileset->metatileLabels[metatileId].isEmpty())
            continue;
        QString label = prefix + tileset->metatileLabels[metatileId];
        this->metatileLabelsMap[tileset->name][label] = metatileId;
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

bool Project::loadLayoutTilesets(Layout *layout) {
    layout->tileset_primary = getTileset(layout->tileset_primary_label);
    if (!layout->tileset_primary) {
        QString defaultTileset = this->getDefaultPrimaryTilesetLabel();
        logWarn(QString("%1 has invalid primary tileset '%2'. Using default '%3'").arg(layout->name).arg(layout->tileset_primary_label).arg(defaultTileset));
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
        logWarn(QString("%1 has invalid secondary tileset '%2'. Using default '%3'").arg(layout->name).arg(layout->tileset_secondary_label).arg(defaultTileset));
        layout->tileset_secondary_label = defaultTileset;
        layout->tileset_secondary = getTileset(layout->tileset_secondary_label);
        if (!layout->tileset_secondary) {
            logError(QString("Failed to set default secondary tileset."));
            return false;
        }
    }
    return true;
}

// TODO: We are parsing the tileset headers file whenever we load a tileset for the first time.
//       At a minimum this means we're parsing the file three times per session (twice here for the first map's tilesets, once on launch in Project::readTilesetLabels).
//       We can cache the header data instead and only parse it once on launch.
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
        auto structs = parser.readCStructs(projectConfig.getFilePath(ProjectFilePath::tilesets_headers), label, memberMap);
        if (!structs.contains(label)) {
            return nullptr;
        }
        if (tileset == nullptr) {
            tileset = new Tileset;
        }
        auto tilesetAttributes = structs[label];
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

bool Project::loadBlockdata(Layout *layout) {
    bool ok = true;
    QString path = QString("%1/%2").arg(root).arg(layout->blockdata_path);
    auto blockdata = readBlockdata(path, &ok);
    if (!ok) {
        logError(QString("Failed to load layout blockdata from '%1'").arg(path));
        return false;
    }

    layout->blockdata = blockdata;
    layout->lastCommitBlocks.blocks = blockdata;
    layout->lastCommitBlocks.layoutDimensions = QSize(layout->getWidth(), layout->getHeight());

    if (layout->blockdata.count() != layout->getWidth() * layout->getHeight()) {
        logWarn(QString("%1 blockdata length %2 does not match dimensions %3x%4 (should be %5). Resizing blockdata.")
                .arg(layout->name)
                .arg(layout->blockdata.count())
                .arg(layout->getWidth())
                .arg(layout->getHeight())
                .arg(layout->getWidth() * layout->getHeight()));
        layout->blockdata.resize(layout->getWidth() * layout->getHeight());
    }
    return true;
}

void Project::setNewLayoutBlockdata(Layout *layout) {
    layout->blockdata.clear();
    int width = layout->getWidth();
    int height = layout->getHeight();
    Block block(projectConfig.defaultMetatileId, projectConfig.defaultCollision, projectConfig.defaultElevation);
    for (int i = 0; i < width * height; i++) {
        layout->blockdata.append(block);
    }
    layout->lastCommitBlocks.blocks = layout->blockdata;
    layout->lastCommitBlocks.layoutDimensions = QSize(width, height);
}

bool Project::loadLayoutBorder(Layout *layout) {
    bool ok = true;
    QString path = QString("%1/%2").arg(root).arg(layout->border_path);
    auto blockdata = readBlockdata(path, &ok);
    if (!ok) {
        logError(QString("Failed to load layout border from '%1'").arg(path));
        return false;
    }

    layout->border = blockdata;
    layout->lastCommitBlocks.border = blockdata;
    layout->lastCommitBlocks.borderDimensions = QSize(layout->getBorderWidth(), layout->getBorderHeight());

    int borderLength = layout->getBorderWidth() * layout->getBorderHeight();
    if (layout->border.count() != borderLength) {
        logWarn(QString("%1 border blockdata length %2 must be %3. Resizing border blockdata.")
                .arg(layout->name)
                .arg(layout->border.count())
                .arg(borderLength));
        layout->border.resize(borderLength);
    }
    return true;
}

void Project::setNewLayoutBorder(Layout *layout) {
    layout->border.clear();
    int width = layout->getBorderWidth();
    int height = layout->getBorderHeight();

    if (projectConfig.newMapBorderMetatileIds.length() != width * height) {
        // Border size doesn't match the number of default border metatiles.
        // Fill the border with empty metatiles.
        for (int i = 0; i < width * height; i++) {
            layout->border.append(0);
        }
    } else {
        // Fill the border with the default metatiles from the config.
        for (int i = 0; i < width * height; i++) {
            layout->border.append(projectConfig.newMapBorderMetatileIds.at(i));
        }
    }

    layout->lastCommitBlocks.border = layout->border;
    layout->lastCommitBlocks.borderDimensions = QSize(width, height);
}

void Project::saveLayoutBorder(Layout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->border_path);
    writeBlockdata(path, layout->border);
}

void Project::saveLayoutBlockdata(Layout *layout) {
    QString path = QString("%1/%2").arg(root).arg(layout->blockdata_path);
    writeBlockdata(path, layout->blockdata);
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
    const QString folderPath = projectConfig.getFilePath(ProjectFilePath::data_map_folders) + map->name();
    const QString fullPath = QString("%1/%2").arg(this->root).arg(folderPath);
    if (!map->isPersistedToFile()) {
        if (!QDir::root().mkpath(fullPath)) {
            logError(QString("Failed to create directory for new map: '%1'").arg(fullPath));
        }

        // Create file data/maps/<map_name>/scripts.inc
        QString text = this->getScriptDefaultString(projectConfig.usePoryScript, map->name());
        saveTextFile(fullPath + "/scripts" + this->getScriptFileExtension(projectConfig.usePoryScript), text);

        if (projectConfig.createMapTextFileEnabled) {
            // Create file data/maps/<map_name>/text.inc
            saveTextFile(fullPath + "/text" + this->getScriptFileExtension(projectConfig.usePoryScript), "\n");
        }

        // Simply append to data/event_scripts.s.
        text = QString("\n\t.include \"%1/scripts.inc\"\n").arg(folderPath);
        if (projectConfig.createMapTextFileEnabled) {
            text += QString("\t.include \"%1/text.inc\"\n").arg(folderPath);
        }
        appendTextFile(root + "/" + projectConfig.getFilePath(ProjectFilePath::data_event_scripts), text);
    }

    // Create map.json for map data.
    QString mapFilepath = fullPath + "/map.json";
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(mapFilepath));
        return;
    }

    OrderedJson::object mapObj;
    // Header values.
    mapObj["id"] = map->constantName();
    mapObj["name"] = map->name();
    mapObj["layout"] = map->layout()->id;
    mapObj["music"] = map->header()->song();
    mapObj["region_map_section"] = map->header()->location();
    mapObj["requires_flash"] = map->header()->requiresFlash();
    mapObj["weather"] = map->header()->weather();
    mapObj["map_type"] = map->header()->type();
    if (projectConfig.mapAllowFlagsEnabled) {
        mapObj["allow_cycling"] = map->header()->allowsBiking();
        mapObj["allow_escaping"] = map->header()->allowsEscaping();
        mapObj["allow_running"] = map->header()->allowsRunning();
    }
    mapObj["show_map_name"] = map->header()->showsLocationName();
    if (projectConfig.floorNumberEnabled) {
        mapObj["floor_number"] = map->header()->floorNumber();
    }
    mapObj["battle_scene"] = map->header()->battleScene();

    // Connections
    const auto connections = map->getConnections();
    if (connections.length() > 0) {
        OrderedJson::array connectionsArr;
        for (const auto &connection : connections) {
            OrderedJson::object connectionObj;
            connectionObj["map"] = this->mapNamesToMapConstants.value(connection->targetMapName(), connection->targetMapName());
            connectionObj["offset"] = connection->offset();
            connectionObj["direction"] = connection->direction();
            connectionsArr.append(connectionObj);
        }
        mapObj["connections"] = connectionsArr;
    } else {
        mapObj["connections"] = OrderedJson();
    }

    if (map->sharedEventsMap().isEmpty()) {
        // Object events
        OrderedJson::array objectEventsArr;
        for (const auto &event : map->getEvents(Event::Group::Object)){
            objectEventsArr.push_back(event->buildEventJson(this));
        }
        mapObj["object_events"] = objectEventsArr;


        // Warp events
        OrderedJson::array warpEventsArr;
        for (const auto &event : map->getEvents(Event::Group::Warp)) {
            warpEventsArr.push_back(event->buildEventJson(this));
        }
        mapObj["warp_events"] = warpEventsArr;

        // Coord events
        OrderedJson::array coordEventsArr;
        for (const auto &event : map->getEvents(Event::Group::Coord)) {
            coordEventsArr.push_back(event->buildEventJson(this));
        }
        mapObj["coord_events"] = coordEventsArr;

        // Bg Events
        OrderedJson::array bgEventsArr;
        for (const auto &event : map->getEvents(Event::Group::Bg)) {
            bgEventsArr.push_back(event->buildEventJson(this));
        }
        mapObj["bg_events"] = bgEventsArr;
    } else {
        mapObj["shared_events_map"] = map->sharedEventsMap();
    }

    if (!map->sharedScriptsMap().isEmpty()) {
        mapObj["shared_scripts_map"] = map->sharedScriptsMap();
    }

    // Update the global heal locations array using the Map's heal location events.
    // This won't get saved to disc until Project::saveHealLocations is called.
    QList<HealLocationEvent*> hlEvents;
    for (const auto &event : map->getEvents(Event::Group::Heal)) {
        auto hl = static_cast<HealLocationEvent*>(event);
        hlEvents.append(static_cast<HealLocationEvent*>(hl->duplicate()));
    }
    qDeleteAll(this->healLocations[map->constantName()]);
    this->healLocations[map->constantName()] = hlEvents;

    // Custom header fields.
    const auto customAttributes = map->customAttributes();
    for (auto i = customAttributes.constBegin(); i != customAttributes.constEnd(); i++) {
        mapObj[i.key()] = OrderedJson::fromQJsonValue(i.value());
    }

    OrderedJson mapJson(mapObj);
    OrderedJsonDoc jsonDoc(&mapJson);
    jsonDoc.dump(&mapFile);
    mapFile.close();

    saveLayout(map->layout());

    map->setClean();
}

void Project::saveLayout(Layout *layout) {
    saveLayoutBorder(layout);
    saveLayoutBlockdata(layout);

    // Update global data structures with current map data.
    updateLayout(layout);

    layout->editHistory.setClean();
}

void Project::updateLayout(Layout *layout) {
    if (!this->layoutIdsMaster.contains(layout->id)) {
        this->layoutIdsMaster.append(layout->id);
    }

    if (this->mapLayoutsMaster.contains(layout->id)) {
        this->mapLayoutsMaster[layout->id]->copyFrom(layout);
    }
    else {
        this->mapLayoutsMaster.insert(layout->id, layout->copy());
    }
}

void Project::saveAllDataStructures() {
    saveMapLayouts();
    saveMapGroups();
    saveRegionMapSections();
    saveHealLocations();
    saveWildMonData();
    saveConfig();
    this->hasUnsavedDataChanges = false;
}

void Project::saveConfig() {
    projectConfig.save();
    userConfig.save();
}

void Project::loadTilesetAssets(Tileset* tileset) {
    if (tileset->name.isNull()) {
        return;
    }
    readTilesetPaths(tileset);
    loadTilesetMetatileLabels(tileset);
    tileset->load();
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

Tileset *Project::createNewTileset(QString name, bool secondary, bool checkerboardFill) {
    const QString prefix = projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix);
    if (!name.startsWith(prefix)) {
        logError(QString("Tileset name '%1' doesn't begin with the prefix '%2'.").arg(name).arg(prefix));
        return nullptr;
    }

    auto tileset = new Tileset();
    tileset->name = name;
    tileset->is_secondary = secondary;

    // Create tileset directories
    const QString fullDirectoryPath = QString("%1/%2").arg(this->root).arg(tileset->getExpectedDir());
    QDir directory;
    if (!directory.mkpath(fullDirectoryPath)) {
        logError(QString("Failed to create directory '%1' for new tileset '%2'").arg(fullDirectoryPath).arg(tileset->name));
        delete tileset;
        return nullptr;
    }
    const QString palettesPath = fullDirectoryPath + "/palettes";
    if (!directory.mkpath(palettesPath)) {
        logError(QString("Failed to create palettes directory '%1' for new tileset '%2'").arg(palettesPath).arg(tileset->name));
        delete tileset;
        return nullptr;
    }

    tileset->tilesImagePath = fullDirectoryPath + "/tiles.png";
    tileset->metatiles_path = fullDirectoryPath + "/metatiles.bin";
    tileset->metatile_attrs_path = fullDirectoryPath + "/metatile_attributes.bin";

    // Set default tiles image
    QImage tilesImage(":/images/blank_tileset.png");
    tileset->loadTilesImage(&tilesImage);

    // Create default metatiles
    const int numMetatiles = tileset->is_secondary ? (Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary()) : Project::getNumMetatilesPrimary();
    const int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
    for (int i = 0; i < numMetatiles; ++i) {
        auto metatile = new Metatile();
        for(int j = 0; j < tilesPerMetatile; ++j){
            Tile tile = Tile();
            if (checkerboardFill) {
                // Create a checkerboard-style dummy tileset
                if (((i / 8) % 2) == 0)
                    tile.tileId = ((i % 2) == 0) ? 1 : 2;
                else
                    tile.tileId = ((i % 2) == 1) ? 1 : 2;

                if (tileset->is_secondary)
                    tile.tileId += Project::getNumTilesPrimary();
            }
            metatile->tiles.append(tile);
        }
        tileset->addMetatile(metatile);
    }

    // Create default palettes
    for(int i = 0; i < 16; ++i) {
        QList<QRgb> currentPal;
        for(int i = 0; i < 16;++i) {
            currentPal.append(qRgb(0,0,0));
        }
        tileset->palettes.append(currentPal);
        tileset->palettePreviews.append(currentPal);
        tileset->palettePaths.append(QString("%1/%2.pal").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')));
    }
    tileset->palettes[0][1] = qRgb(255,0,255);
    tileset->palettePreviews[0][1] = qRgb(255,0,255);

    // Update tileset label arrays
    QStringList *labelList = tileset->is_secondary ? &this->secondaryTilesetLabels : &this->primaryTilesetLabels;
    for (int i = 0; i < labelList->length(); i++) {
        if (labelList->at(i) > tileset->name) {
            labelList->insert(i, tileset->name);
            break;
        }
    }
    this->tilesetLabelsOrdered.append(tileset->name);

    // TODO: Ideally we wouldn't save new Tilesets immediately
    // Append to tileset specific files. Strip prefix from name to get base name for use in other symbols.
    name.remove(0, prefix.length());
    tileset->appendToHeaders(this->root, name, this->usingAsmTilesets);
    tileset->appendToGraphics(this->root, name, this->usingAsmTilesets);
    tileset->appendToMetatiles(this->root, name, this->usingAsmTilesets);

    tileset->save();

    this->tilesetCache.insert(tileset->name, tileset);

    emit tilesetCreated(tileset);
    return tileset;
}

QString Project::findMetatileLabelsTileset(QString label) {
    for (const QString &tilesetName : this->tilesetLabelsOrdered) {
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

    const QStringList regexList = {QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix))};
    QMap<QString, int> defines = parser.readCDefinesByRegex(metatileLabelsFilename, regexList);

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
    for (auto it = this->metatileLabelsMap[tileset->name].constBegin(); it != this->metatileLabelsMap[tileset->name].constEnd(); it++) {
        QString labelName = it.key();
        tileset->metatileLabels[it.value()] = labelName.replace(metatileLabelPrefix, "");
    }
}

Blockdata Project::readBlockdata(QString path, bool *ok) {
    Blockdata blockdata;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        for (int i = 0; (i + 1) < data.length(); i += 2) {
            uint16_t word = static_cast<uint16_t>((data[i] & 0xff) + ((data[i + 1] & 0xff) << 8));
            blockdata.append(word);
        }
        if (ok) *ok = true;
    } else {
        // Failed
        if (ok) *ok = false;
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
    this->extraEncounterGroups.clear();
    this->wildMonFields.clear();
    this->wildMonData.clear();
    this->encounterGroupLabels.clear();
    this->pokemonMinLevel = 0;
    this->pokemonMaxLevel = 100;
    this->maxEncounterRate = 2880/16;
    this->wildEncountersLoaded = false;
    if (!userConfig.useEncounterJson) {
        return true;
    }

    // Read max encounter rate. The games multiply the encounter rate value in the map data by 16, so our input limit is the max/16.
    const QString encounterRateFile = projectConfig.getFilePath(ProjectFilePath::wild_encounter);
    const QString maxEncounterRateName = projectConfig.getIdentifier(ProjectIdentifier::define_max_encounter_rate);

    fileWatcher.addPath(QString("%1/%2").arg(root).arg(encounterRateFile));
    auto defines = parser.readCDefinesByName(encounterRateFile, {maxEncounterRateName});
    if (defines.contains(maxEncounterRateName))
        this->maxEncounterRate = defines.value(maxEncounterRateName)/16;

    // Read min/max level
    const QString levelRangeFile = projectConfig.getFilePath(ProjectFilePath::constants_pokemon);
    const QString minLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_min_level);
    const QString maxLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_max_level);

    fileWatcher.addPath(QString("%1/%2").arg(root).arg(levelRangeFile));
    defines = parser.readCDefinesByName(levelRangeFile, {minLevelName, maxLevelName});
    if (defines.contains(minLevelName))
        this->pokemonMinLevel = defines.value(minLevelName);
    if (defines.contains(maxLevelName))
        this->pokemonMaxLevel = defines.value(maxLevelName);

    this->pokemonMinLevel = qMin(this->pokemonMinLevel, this->pokemonMaxLevel);
    this->pokemonMaxLevel = qMax(this->pokemonMinLevel, this->pokemonMaxLevel);

    // Read encounter data
    QString wildMonJsonFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_wild_encounters));
    fileWatcher.addPath(wildMonJsonFilepath);

    OrderedJson::object wildMonObj;
    if (!parser.tryParseOrderedJsonFile(&wildMonObj, wildMonJsonFilepath)) {
        // Failing to read wild encounters data is not a critical error, the encounter editor will just be disabled
        logWarn(QString("Failed to read wild encounters from %1").arg(wildMonJsonFilepath));
        return true;
    }

    // For each encounter type, count the number of times each encounter rate value occurs.
    // The most common value will be used as the default for new groups.
    QMap<QString, QMap<int, int>> encounterRateFrequencyMaps;

    // Parse "wild_encounter_groups". This is the main object array containing all the data in this file.
    for (OrderedJson mainArrayJson : wildMonObj["wild_encounter_groups"].array_items()) {
        OrderedJson::object mainArrayObject = mainArrayJson.object_items();

        // We're only interested in wild encounter data that's associated with maps ("for_maps" == true).
        // Any other wild encounter data (e.g. for Battle Pike / Battle Pyramid) will be ignored.
        // We'll record any data that's not for maps in extraEncounterGroups to be outputted when we save.
        if (!mainArrayObject["for_maps"].bool_value()) {
            this->extraEncounterGroups.push_back(mainArrayObject);
            continue;
        }

        // Parse the "fields" data. This is like the header for the wild encounters data.
        // Each element describes a type of wild encounter Porymap can expect to find, and we represent this data with an EncounterField.
        // They should contain a name ("type"), the number of encounter slots and the ratio at which they occur ("encounter_rates"),
        // and whether the encounters are divided into groups (like fishing rods).
        for (const OrderedJson &fieldJson : mainArrayObject["fields"].array_items()) {
            OrderedJson::object fieldObject = fieldJson.object_items();

            EncounterField encounterField;
            encounterField.name = fieldObject["type"].string_value();

            for (auto val : fieldObject["encounter_rates"].array_items()) {
                encounterField.encounterRates.append(val.int_value());
            }

            // Each element of the "groups" array is an object with the group name as the key (e.g. "old_rod")
            // and an array of slot numbers indicating which encounter slots in this encounter type belong to that group.
            for (auto groupPair : fieldObject["groups"].object_items()) {
                const QString groupName = groupPair.first;
                for (auto slotNum : groupPair.second.array_items()) {
                    encounterField.groups[groupName].append(slotNum.int_value());
                }
            }

            encounterRateFrequencyMaps.insert(encounterField.name, QMap<int, int>());
            this->wildMonFields.append(encounterField);
        }

        // Parse the "encounters" data. This is the meat of the wild encounters data.
        // Each element is an object that will tell us which map it's associated with,
        // its symbol name (which we will display in the Groups dropdown) and a list of
        // pokémon associated with any of the encounter types described by the data we parsed above.
        for (const auto &encounterJson : mainArrayObject["encounters"].array_items()) {
            OrderedJson::object encounterObj = encounterJson.object_items();

            WildPokemonHeader header;

            // Check for each possible encounter type.
            for (const EncounterField &monField : this->wildMonFields) {
                const QString field = monField.name;
                if (encounterObj[field].is_null()) {
                    // Encounter type isn't present
                    continue;
                }
                OrderedJson::object encounterFieldObj = encounterObj[field].object_items();

                WildMonInfo monInfo;
                monInfo.active = true;

                // Read encounter rate
                monInfo.encounterRate = encounterFieldObj["encounter_rate"].int_value();
                encounterRateFrequencyMaps[field][monInfo.encounterRate]++;

                // Read wild pokémon list
                for (auto monJson : encounterFieldObj["mons"].array_items()) {
                    OrderedJson::object monObj = monJson.object_items();

                    WildPokemon newMon;
                    newMon.minLevel = monObj["min_level"].int_value();
                    newMon.maxLevel = monObj["max_level"].int_value();
                    newMon.species = monObj["species"].string_value();
                    monInfo.wildPokemon.append(newMon);
                }

                // If the user supplied too few pokémon for this group then we fill in the rest with default values.
                for (int i = monInfo.wildPokemon.length(); i < monField.encounterRates.length(); i++) {
                    monInfo.wildPokemon.append(WildPokemon());
                }
                header.wildMons[field] = monInfo;
            }

            const QString mapConstant = encounterObj["map"].string_value();
            const QString baseLabel = encounterObj["base_label"].string_value();
            this->wildMonData[mapConstant].insert({baseLabel, header});
            this->encounterGroupLabels.append(baseLabel);
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

    this->wildEncountersLoaded = true;
    return true;
}

bool Project::readMapGroups() {
    this->mapConstantsToMapNames.clear();
    this->mapNamesToMapConstants.clear();
    this->mapNames.clear();
    this->groupNames.clear();
    this->groupNameToMapNames.clear();

    this->initTopLevelMapFields();

    const QString filepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::json_map_groups);
    fileWatcher.addPath(filepath);
    QJsonDocument mapGroupsDoc;
    if (!parser.tryParseJsonFile(&mapGroupsDoc, filepath)) {
        logError(QString("Failed to read map groups from %1").arg(filepath));
        return false;
    }

    QJsonObject mapGroupsObj = mapGroupsDoc.object();
    QJsonArray mapGroupOrder = mapGroupsObj["group_order"].toArray();

    const QString dynamicMapName = getDynamicMapName();
    const QString dynamicMapConstant = getDynamicMapDefineName();

    // Process the map group lists
    QStringList failedMapNames;
    for (int groupIndex = 0; groupIndex < mapGroupOrder.size(); groupIndex++) {
        const QString groupName = ParseUtil::jsonToQString(mapGroupOrder.at(groupIndex));
        const QJsonArray mapNamesJson = mapGroupsObj.value(groupName).toArray();
        this->groupNames.append(groupName);
        
        // Process the names in this map group
        for (int j = 0; j < mapNamesJson.size(); j++) {
            const QString mapName = ParseUtil::jsonToQString(mapNamesJson.at(j));
            if (mapName == dynamicMapName) {
                logWarn(QString("Ignoring map with reserved name '%1'.").arg(mapName));
                failedMapNames.append(mapName);
                continue;
            }
            if (this->mapNames.contains(mapName)) {
                logWarn(QString("Ignoring repeated map name '%1'.").arg(mapName));
                failedMapNames.append(mapName);
                continue;
            }

            // Load the map's json file so we can get its ID constant (and two other constants we use for the map list).
            QJsonDocument mapDoc;
            if (!readMapJson(mapName, &mapDoc)) {
                failedMapNames.append(mapName);
                continue; // Error message has already been logged
            }

            // Read and validate the map's ID from its JSON data.
            const QJsonObject mapObj = mapDoc.object();
            const QString mapConstant = ParseUtil::jsonToQString(mapObj["id"]);
            if (mapConstant.isEmpty()) {
                logWarn(QString("Map '%1' is missing an \"id\" value and will be ignored.").arg(mapName));
                failedMapNames.append(mapName);
                continue;
            }
            if (mapConstant == dynamicMapConstant) {
                logWarn(QString("Ignoring map with reserved \"id\" value '%1'.").arg(mapName));
                failedMapNames.append(mapName);
                continue;
            }
            const QString expectedPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
            if (!mapConstant.startsWith(expectedPrefix)) {
                logWarn(QString("Map '%1' has invalid \"id\" value '%2' and will be ignored. Value must begin with '%3'.").arg(mapName).arg(mapConstant).arg(expectedPrefix));
                failedMapNames.append(mapName);
                continue;
            }
            auto it = this->mapConstantsToMapNames.constFind(mapConstant);
            if (it != this->mapConstantsToMapNames.constEnd()) {
                logWarn(QString("Map '%1' has the same \"id\" value '%2' as map '%3' and will be ignored.").arg(mapName).arg(it.key()).arg(it.value()));
                failedMapNames.append(mapName);
                continue;
            }

            // Read layout ID for map list
            const QString layoutId = ParseUtil::jsonToQString(mapObj["layout"]);
            if (!this->layoutIds.contains(layoutId)) {
                // If a map has an unknown layout ID it won't be able to load it at all anyway, so skip it.
                // Skipping these will let us assume all the map layout IDs are valid, which simplies some handling elsewhere.
                logWarn(QString("Map '%1' has unknown \"layout\" value '%2' and will be ignored.").arg(mapName).arg(layoutId));
                failedMapNames.append(mapName);
                continue;
            }

            // Read MAPSEC name for map list
            const QString mapSectionName = ParseUtil::jsonToQString(mapObj["region_map_section"]);
            if (!this->mapSectionIdNames.contains(mapSectionName)) {
                // An unknown location is OK. Aside from that name not appearing in the dropdowns this shouldn't cause problems.
                // We'll log a warning, but allow this map to be displayed.
                logWarn(QString("Map '%1' has unknown \"region_map_section\" value '%2'.").arg(mapName).arg(mapSectionName));
            }

            // Success, save the constants to the project
            this->mapNames.append(mapName);
            this->groupNameToMapNames[groupName].append(mapName);
            this->mapConstantsToMapNames.insert(mapConstant, mapName);
            this->mapNamesToMapConstants.insert(mapName, mapConstant);
            this->mapNameToLayoutId.insert(mapName, layoutId);
            this->mapNameToMapSectionName.insert(mapName, mapSectionName);
        }
    }

    if (this->groupNames.isEmpty()) {
        logError(QString("Failed to find any map groups in %1").arg(filepath));
        return false;
    }
    if (this->mapNames.isEmpty()) {
        logError(QString("Failed to find any map names in %1").arg(filepath));
        return false;
    }

    if (!failedMapNames.isEmpty()) {
        // At least 1 map was excluded due to an error.
        // User should be alerted of this, rather than just silently logging the details.
        emit mapsExcluded(failedMapNames);
    }

    // Save special "Dynamic" constant
    this->mapConstantsToMapNames.insert(dynamicMapConstant, dynamicMapName);
    this->mapNamesToMapConstants.insert(dynamicMapName, dynamicMapConstant);
    this->mapNames.append(dynamicMapName);

    return true;
}

void Project::addNewMapGroup(const QString &groupName) {
    if (this->groupNames.contains(groupName))
        return;

    this->groupNames.append(groupName);
    this->groupNameToMapNames.insert(groupName, QStringList());
    this->hasUnsavedDataChanges = true;

    emit mapGroupAdded(groupName);
}

QString Project::mapNameToMapGroup(const QString &mapName) {
    for (auto it = this->groupNameToMapNames.constBegin(); it != this->groupNameToMapNames.constEnd(); it++) {
        const QStringList mapNames = it.value();
        if (mapNames.contains(mapName)) {
            return it.key();
        }
    }
    return QString();
}

// When we ask the user to provide a new identifier for something (like a map name or MAPSEC id)
// we use this to make sure that it doesn't collide with any known identifiers first.
// Porymap knows of many more identifiers than this, but for simplicity we only check the lists that users can add to via Porymap.
// In general this only matters to Porymap if the identifier will be added to the group it collides with,
// but name collisions are likely undesirable in the project.
bool Project::isIdentifierUnique(const QString &identifier) const {
    if (this->mapNames.contains(identifier))
        return false;
    if (this->mapConstantsToMapNames.contains(identifier))
        return false;
    if (this->groupNames.contains(identifier))
        return false;
    if (this->mapSectionIdNames.contains(identifier))
        return false;
    if (this->tilesetLabelsOrdered.contains(identifier))
        return false;
    if (this->layoutIds.contains(identifier))
        return false;
    for (const auto &layout : this->mapLayouts) {
        if (layout->name == identifier) {
            return false;
        }
    }
    if (identifier == getEmptyMapDefineName())
        return false;
    if (this->encounterGroupLabels.contains(identifier))
        return false;
    // Check event IDs
    for (const auto &map : this->mapCache) {
        auto events = map->getEvents();
        for (const auto &event : events) {
            QString idName = event->getIdName();
            if (!idName.isEmpty() && idName == identifier)
                return false;
        }
    }
    return true;
}

// For some arbitrary string, return true if it's both a valid identifier name and not one that's already in-use.
bool Project::isValidNewIdentifier(QString identifier) const {
    IdentifierValidator validator;
    return validator.isValid(identifier) && isIdentifierUnique(identifier);
}

// Assumes 'identifier' is a valid name. If 'identifier' is unique, returns 'identifier'.
// Otherwise returns the identifier with a numbered suffix added to make it unique.
QString Project::toUniqueIdentifier(const QString &identifier) const {
    int suffix = 2;
    QString uniqueIdentifier = identifier;
    while (!isIdentifierUnique(uniqueIdentifier)) {
        uniqueIdentifier = QString("%1_%2").arg(identifier).arg(suffix++);
    }
    return uniqueIdentifier;
}

void Project::initNewMapSettings() {
    this->newMapSettings.name = QString();
    this->newMapSettings.group = this->groupNames.at(0);
    this->newMapSettings.canFlyTo = false;

    this->newMapSettings.layout.folderName = this->newMapSettings.name;
    this->newMapSettings.layout.name = QString();
    this->newMapSettings.layout.id = Layout::layoutConstantFromName(this->newMapSettings.name);
    this->newMapSettings.layout.width = getDefaultMapDimension();
    this->newMapSettings.layout.height = getDefaultMapDimension();
    this->newMapSettings.layout.borderWidth = DEFAULT_BORDER_WIDTH;
    this->newMapSettings.layout.borderHeight = DEFAULT_BORDER_HEIGHT;
    this->newMapSettings.layout.primaryTilesetLabel = getDefaultPrimaryTilesetLabel();
    this->newMapSettings.layout.secondaryTilesetLabel = getDefaultSecondaryTilesetLabel();

    this->newMapSettings.header.setSong(this->defaultSong);
    this->newMapSettings.header.setLocation(this->mapSectionIdNames.value(0, "0"));
    this->newMapSettings.header.setRequiresFlash(false);
    this->newMapSettings.header.setWeather(this->weatherNames.value(0, "0"));
    this->newMapSettings.header.setType(this->mapTypes.value(0, "0"));
    this->newMapSettings.header.setBattleScene(this->mapBattleScenes.value(0, "0"));
    this->newMapSettings.header.setShowsLocationName(true);
    this->newMapSettings.header.setAllowsRunning(false);
    this->newMapSettings.header.setAllowsBiking(false);
    this->newMapSettings.header.setAllowsEscaping(false);
    this->newMapSettings.header.setFloorNumber(0);
}

void Project::initNewLayoutSettings() {
    this->newLayoutSettings.name = QString();
    this->newLayoutSettings.id = Layout::layoutConstantFromName(this->newLayoutSettings.name);
    this->newLayoutSettings.width = getDefaultMapDimension();
    this->newLayoutSettings.height = getDefaultMapDimension();
    this->newLayoutSettings.borderWidth = DEFAULT_BORDER_WIDTH;
    this->newLayoutSettings.borderHeight = DEFAULT_BORDER_HEIGHT;
    this->newLayoutSettings.primaryTilesetLabel = getDefaultPrimaryTilesetLabel();
    this->newLayoutSettings.secondaryTilesetLabel = getDefaultSecondaryTilesetLabel();
}

QString Project::getDefaultPrimaryTilesetLabel() const {
    QString defaultLabel = projectConfig.defaultPrimaryTileset;
    if (!this->primaryTilesetLabels.contains(defaultLabel)) {
        QString firstLabel = this->primaryTilesetLabels.first();
        logWarn(QString("Unable to find default primary tileset '%1', using '%2' instead.").arg(defaultLabel).arg(firstLabel));
        defaultLabel = firstLabel;
    }
    return defaultLabel;
}

QString Project::getDefaultSecondaryTilesetLabel() const {
    QString defaultLabel = projectConfig.defaultSecondaryTileset;
    if (!this->secondaryTilesetLabels.contains(defaultLabel)) {
        QString firstLabel = this->secondaryTilesetLabels.first();
        logWarn(QString("Unable to find default secondary tileset '%1', using '%2' instead.").arg(defaultLabel).arg(firstLabel));
        defaultLabel = firstLabel;
    }
    return defaultLabel;
}

void Project::appendTilesetLabel(const QString &label, const QString &isSecondaryStr) {
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
        filename = asm_filename; // For error reporting further down
    } else {
        this->usingAsmTilesets = false;
        const auto structs = parser.readCStructs(filename, "", Tileset::getHeaderMemberMap(this->usingAsmTilesets));
        for (auto i = structs.cbegin(); i != structs.cend(); i++){
            appendTilesetLabel(i.key(), i.value().value("isSecondary"));
        }
    }

    numericalModeSort(this->primaryTilesetLabels);
    numericalModeSort(this->secondaryTilesetLabels);

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
    const QString numTilesPerMetatileName = projectConfig.getIdentifier(ProjectIdentifier::define_tiles_per_metatile);
    const QStringList names = {
        numTilesPrimaryName,
        numTilesTotalName,
        numMetatilesPrimaryName,
        numPalsPrimaryName,
        numPalsTotalName,
        maxMapSizeName,
        numTilesPerMetatileName,
    };
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_fieldmap);
    fileWatcher.addPath(root + "/" + filename);
    const QMap<QString, int> defines = parser.readCDefinesByName(filename, names);

    auto loadDefine = [defines](const QString name, int * dest, int min, int max) {
        auto it = defines.find(name);
        if (it != defines.end()) {
            *dest = it.value();
            if (*dest < min) {
                logWarn(QString("Value for tileset property '%1' (%2) is below the minimum (%3). Defaulting to minimum.").arg(name).arg(*dest).arg(min));
                *dest = min;
            } else if (*dest > max) {
                logWarn(QString("Value for tileset property '%1' (%2) is above the maximum (%3). Defaulting to maximum.").arg(name).arg(*dest).arg(max));
                *dest = max;
            }
        } else {
            logWarn(QString("Value for tileset property '%1' not found. Using default (%2) instead.").arg(name).arg(*dest));
        }
    };
    loadDefine(numPalsTotalName,        &Project::num_pals_total, 2, INT_MAX); // In reality the max would be 16, but as far as Porymap is concerned it doesn't matter.
    loadDefine(numTilesTotalName,       &Project::num_tiles_total, 2, 1024); // 1024 is fixed because we store tile IDs in a 10-bit field.
    loadDefine(numPalsPrimaryName,      &Project::num_pals_primary, 1, Project::num_pals_total - 1);
    loadDefine(numTilesPrimaryName,     &Project::num_tiles_primary, 1, Project::num_tiles_total - 1);

    // This maximum is overly generous, because until we parse the appropriate masks from the project
    // we don't actually know what the maximum number of metatiles is.
    loadDefine(numMetatilesPrimaryName, &Project::num_metatiles_primary, 1, 0xFFFF - 1);

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

    it = defines.find(numTilesPerMetatileName);
    if (it != defines.end()) {
        // We can determine whether triple-layer metatiles are in-use by reading this constant.
        // If the constant is missing (or is using a value other than 8 or 12) the user must tell
        // us whether they're using triple-layer metatiles under Project Settings.
        static const int numTilesPerLayer = 4;
        int numTilesPerMetatile = it.value();
        if (numTilesPerMetatile == 2 * numTilesPerLayer) {
            projectConfig.tripleLayerMetatilesEnabled = false;
            this->disabledSettingsNames.insert(numTilesPerMetatileName);
        } else if (numTilesPerMetatile == 3 * numTilesPerLayer) {
            projectConfig.tripleLayerMetatilesEnabled = true;
            this->disabledSettingsNames.insert(numTilesPerMetatileName);
        }
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
    for (auto name : defineNames)
        this->disabledSettingsNames.insert(name);

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
        projectConfig.blockMetatileIdMask = blockMask;
    if (readBlockMask(collisionMaskName, &blockMask))
        projectConfig.blockCollisionMask = blockMask;
    if (readBlockMask(elevationMaskName, &blockMask))
        projectConfig.blockElevationMask = blockMask;

    // Read RSE metatile attribute masks
    auto it = defines.find(behaviorMaskName);
    if (it != defines.end())
        projectConfig.metatileBehaviorMask = static_cast<uint32_t>(it.value());
    it = defines.find(layerTypeMaskName);
    if (it != defines.end())
        projectConfig.metatileLayerTypeMask = static_cast<uint32_t>(it.value());

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
            projectConfig.metatileTerrainTypeMask = mask;
            this->disabledSettingsNames.insert(terrainTypeTableName);
        }
        // Read encounter type mask
        mask = attrTable.value(encounterTypeTableName).toUInt(&ok, 0);
        if (ok) {
            projectConfig.metatileEncounterTypeMask = mask;
            this->disabledSettingsNames.insert(encounterTypeTableName);
        }
        // If we haven't already parsed behavior and layer type then try those too
        if (!this->disabledSettingsNames.contains(behaviorMaskName)) {
            // Read behavior mask
            mask = attrTable.value(behaviorTableName).toUInt(&ok, 0);
            if (ok) {
                projectConfig.metatileBehaviorMask = mask;
                this->disabledSettingsNames.insert(behaviorTableName);
            }
        }
        if (!this->disabledSettingsNames.contains(layerTypeMaskName)) {
            // Read layer type mask
            mask = attrTable.value(layerTypeTableName).toUInt(&ok, 0);
            if (ok) {
                projectConfig.metatileLayerTypeMask = mask;
                this->disabledSettingsNames.insert(layerTypeTableName);
            }
        }
    }
    return true;
}

bool Project::readRegionMapSections() {
    this->mapSectionIdNames.clear();
    this->mapSectionIdNamesSaveOrder.clear();
    this->mapSectionDisplayNames.clear();
    this->regionMapEntries.clear();
    const QString defaultName = getEmptyMapsecName();
    const QString requiredPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix);

    QJsonDocument doc;
    const QString baseFilepath = projectConfig.getFilePath(ProjectFilePath::json_region_map_entries);
    const QString filepath = QString("%1/%2").arg(this->root).arg(baseFilepath);
    if (!parser.tryParseJsonFile(&doc, filepath)) {
        logError(QString("Failed to read region map sections from '%1'").arg(baseFilepath));
        return false;
    }
    fileWatcher.addPath(filepath);

    QJsonArray mapSections = doc.object()["map_sections"].toArray();
    for (int i = 0; i < mapSections.size(); i++) {
        QJsonObject mapSectionObj = mapSections.at(i).toObject();

        // For each map section, "id" is the only required field. This is the field we use to display the location names in the map list, and in various drop-downs.
        QString idField = "id";
        if (!mapSectionObj.contains(idField)) {
            const QString oldIdField = "map_section";
            if (mapSectionObj.contains(oldIdField)) {
                // User has the old name for this field. Parse using this name, then save with the new name.
                // This will presumably stop the user's project from compiling, but that's preferable to
                // ignoring everything here and then wiping the file's data when we save later.
                idField = oldIdField;
            } else {
                logWarn(QString("Ignoring data for map section %1 in '%2'. Missing required field \"%3\"").arg(i).arg(baseFilepath).arg(idField));
                continue;
            }
        }
        const QString idName = ParseUtil::jsonToQString(mapSectionObj[idField]);
        if (!idName.startsWith(requiredPrefix)) {
            logWarn(QString("Ignoring data for map section '%1' in '%2'. IDs must start with the prefix '%3'").arg(idName).arg(baseFilepath).arg(requiredPrefix));
            continue;
        }

        this->mapSectionIdNames.append(idName);
        this->mapSectionIdNamesSaveOrder.append(idName);

        if (mapSectionObj.contains("name"))
            this->mapSectionDisplayNames.insert(idName, ParseUtil::jsonToQString(mapSectionObj["name"]));

        // Map sections may have additional data indicating their position on the region map.
        // If they have this data, we can add them to the region map entry list.
        bool hasRegionMapData = true;
        static const QSet<QString> regionMapFieldNames = { "x", "y", "width", "height" };
        for (auto fieldName : regionMapFieldNames) {
            if (!mapSectionObj.contains(fieldName)) {
                hasRegionMapData = false;
                break;
            }
        }
        if (!hasRegionMapData)
            continue;

        MapSectionEntry entry;
        entry.x = ParseUtil::jsonToInt(mapSectionObj["x"]);
        entry.y = ParseUtil::jsonToInt(mapSectionObj["y"]);
        entry.width = ParseUtil::jsonToInt(mapSectionObj["width"]);
        entry.height = ParseUtil::jsonToInt(mapSectionObj["height"]);
        entry.valid = true;
        this->regionMapEntries[idName] = entry;
    }

    // Make sure the default name is present in the list.
    if (!this->mapSectionIdNames.contains(defaultName)) {
        this->mapSectionIdNames.append(defaultName);
    }
    numericalModeSort(this->mapSectionIdNames);

    return true;
}

QString Project::getEmptyMapsecName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_map_section_empty);
}

QString Project::getMapGroupPrefix() {
    // We could expose this to users, but it's never enforced so it probably won't affect anyone.
    return QStringLiteral("gMapGroup_");
}

// This function assumes a valid and unique name
void Project::addNewMapsec(const QString &idName) {
    if (this->mapSectionIdNamesSaveOrder.last() == getEmptyMapsecName()) {
        // If the default map section name (MAPSEC_NONE) is last in the list we'll keep it last in the list.
        this->mapSectionIdNamesSaveOrder.insert(this->mapSectionIdNames.length() - 1, idName);
    } else {
        this->mapSectionIdNamesSaveOrder.append(idName);
    }

    this->mapSectionIdNames.append(idName);
    numericalModeSort(this->mapSectionIdNames);

    this->hasUnsavedDataChanges = true;

    emit mapSectionAdded(idName);
    emit mapSectionIdNamesChanged(this->mapSectionIdNames);
}

void Project::removeMapsec(const QString &idName) {
    if (!this->mapSectionIdNames.contains(idName) || idName == getEmptyMapsecName())
        return;

    this->mapSectionIdNames.removeOne(idName);
    this->mapSectionIdNamesSaveOrder.removeOne(idName);
    this->hasUnsavedDataChanges = true;
    emit mapSectionIdNamesChanged(this->mapSectionIdNames);
}

void Project::setMapsecDisplayName(const QString &idName, const QString &displayName) {
    if (this->mapSectionDisplayNames.value(idName) == displayName)
        return;
    this->mapSectionDisplayNames[idName] = displayName;
    this->hasUnsavedDataChanges = true;
    emit mapSectionDisplayNameChanged(idName, displayName);
}

void Project::clearHealLocations() {
    for (auto &events : this->healLocations) {
        qDeleteAll(events);
    }
    this->healLocations.clear();
    this->healLocationSaveOrder.clear();
}

bool Project::readHealLocations() {
    clearHealLocations();

    QJsonDocument doc;
    const QString baseFilepath = projectConfig.getFilePath(ProjectFilePath::json_heal_locations);
    const QString filepath = QString("%1/%2").arg(this->root).arg(baseFilepath);
    if (!parser.tryParseJsonFile(&doc, filepath)) {
        logError(QString("Failed to read heal locations from '%1'").arg(baseFilepath));
        return false;
    }
    fileWatcher.addPath(filepath);

    QJsonArray healLocations = doc.object()["heal_locations"].toArray();
    for (int i = 0; i < healLocations.size(); i++) {
        QJsonObject healLocationObj = healLocations.at(i).toObject();
        static const QString mapField = QStringLiteral("map");
        if (!healLocationObj.contains(mapField)) {
            logWarn(QString("Ignoring data for heal location %1 in '%2'. Missing required field \"%3\"").arg(i).arg(baseFilepath).arg(mapField));
            continue;
        }

        auto event = new HealLocationEvent();
        event->loadFromJson(healLocationObj, this);
        this->healLocations[ParseUtil::jsonToQString(healLocationObj["map"])].append(event);
        this->healLocationSaveOrder.append(event->getIdName());
    }
    return true;
}

bool Project::readItemNames() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_items)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_items);
    fileWatcher.addPath(root + "/" + filename);
    itemNames = parser.readCDefineNames(filename, regexList);
    if (itemNames.isEmpty())
        logWarn(QString("Failed to read item constants from %1").arg(filename));
    return true;
}

bool Project::readFlagNames() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_flags)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_flags);
    fileWatcher.addPath(root + "/" + filename);
    flagNames = parser.readCDefineNames(filename, regexList);
    if (flagNames.isEmpty())
        logWarn(QString("Failed to read flag constants from %1").arg(filename));
    return true;
}

bool Project::readVarNames() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_vars)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_vars);
    fileWatcher.addPath(root + "/" + filename);
    varNames = parser.readCDefineNames(filename, regexList);
    if (varNames.isEmpty())
        logWarn(QString("Failed to read var constants from %1").arg(filename));
    return true;
}

bool Project::readMovementTypes() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_movement_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_event_movement);
    fileWatcher.addPath(root + "/" + filename);
    movementTypes = parser.readCDefineNames(filename, regexList);
    if (movementTypes.isEmpty())
        logWarn(QString("Failed to read movement type constants from %1").arg(filename));
    return true;
}

bool Project::readInitialFacingDirections() {
    QString filename = projectConfig.getFilePath(ProjectFilePath::initial_facing_table);
    fileWatcher.addPath(root + "/" + filename);
    facingDirections = parser.readNamedIndexCArray(filename, projectConfig.getIdentifier(ProjectIdentifier::symbol_facing_directions));
    if (facingDirections.isEmpty())
        logWarn(QString("Failed to read initial movement type facing directions from %1").arg(filename));
    return true;
}

bool Project::readMapTypes() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_map_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    fileWatcher.addPath(root + "/" + filename);
    mapTypes = parser.readCDefineNames(filename, regexList);
    if (mapTypes.isEmpty())
        logWarn(QString("Failed to read map type constants from %1").arg(filename));
    return true;
}

bool Project::readMapBattleScenes() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_battle_scenes)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    fileWatcher.addPath(root + "/" + filename);
    mapBattleScenes = parser.readCDefineNames(filename, regexList);
    if (mapBattleScenes.isEmpty())
        logWarn(QString("Failed to read map battle scene constants from %1").arg(filename));
    return true;
}

bool Project::readWeatherNames() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_weather)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    fileWatcher.addPath(root + "/" + filename);
    weatherNames = parser.readCDefineNames(filename, regexList);
    if (weatherNames.isEmpty())
        logWarn(QString("Failed to read weather constants from %1").arg(filename));
    return true;
}

bool Project::readCoordEventWeatherNames() {
    if (!projectConfig.eventWeatherTriggerEnabled)
        return true;

    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_coord_event_weather)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    fileWatcher.addPath(root + "/" + filename);
    coordEventWeatherNames = parser.readCDefineNames(filename, regexList);
    if (coordEventWeatherNames.isEmpty())
        logWarn(QString("Failed to read coord event weather constants from %1").arg(filename));
    return true;
}

bool Project::readSecretBaseIds() {
    if (!projectConfig.eventSecretBaseEnabled)
        return true;

    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_secret_bases)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_secret_bases);
    fileWatcher.addPath(root + "/" + filename);
    secretBaseIds = parser.readCDefineNames(filename, regexList);
    if (secretBaseIds.isEmpty())
        logWarn(QString("Failed to read secret base id constants from '%1'").arg(filename));
    return true;
}

bool Project::readBgEventFacingDirections() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_sign_facing_directions)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_event_bg);
    fileWatcher.addPath(root + "/" + filename);
    bgEventFacingDirections = parser.readCDefineNames(filename, regexList);
    if (bgEventFacingDirections.isEmpty())
        logWarn(QString("Failed to read bg event facing direction constants from %1").arg(filename));
    return true;
}

bool Project::readTrainerTypes() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_trainer_types)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_trainer_types);
    fileWatcher.addPath(root + "/" + filename);
    trainerTypes = parser.readCDefineNames(filename, regexList);
    if (trainerTypes.isEmpty())
        logWarn(QString("Failed to read trainer type constants from %1").arg(filename));
    return true;
}

bool Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();

    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_behaviors)};
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_behaviors);
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> defines = parser.readCDefinesByRegex(filename, regexList);
    if (defines.isEmpty()) {
        // Not having any metatile behavior names is ok (their values will be displayed instead).
        // If the user's metatiles can have nonzero values then warn them, as they likely want names.
        if (projectConfig.metatileBehaviorMask)
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
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_music)};
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_songs);
    fileWatcher.addPath(root + "/" + filename);
    this->songNames = parser.readCDefineNames(filename, regexList);
    if (this->songNames.isEmpty())
        logWarn(QString("Failed to read song names from %1.").arg(filename));

    // Song names don't have a very useful order (esp. if we include SE_* values), so sort them alphabetically.
    // The default song should be the first in the list, not the first alphabetically, so save that before sorting.
    this->defaultSong = this->songNames.value(0, "0");
    numericalModeSort(this->songNames);
    return true;
}

bool Project::readObjEventGfxConstants() {
    const QStringList regexList = {projectConfig.getIdentifier(ProjectIdentifier::regex_obj_event_gfx)};
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_events);
    fileWatcher.addPath(root + "/" + filename);
    this->gfxDefines = parser.readCDefinesByRegex(filename, regexList);
    if (this->gfxDefines.isEmpty())
        logWarn(QString("Failed to read object event graphics constants from %1.").arg(filename));
    return true;
}

bool Project::readMiscellaneousConstants() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_global);
    const QString maxObjectEventsName = projectConfig.getIdentifier(ProjectIdentifier::define_obj_event_count);
    fileWatcher.addPath(root + "/" + filename);
    QMap<QString, int> defines = parser.readCDefinesByName(filename, {maxObjectEventsName});

    this->maxObjectEvents = 64; // Default value
    auto it = defines.find(maxObjectEventsName);
    if (it != defines.end()) {
        if (it.value() > 0) {
            this->maxObjectEvents = it.value();
        } else {
            logWarn(QString("Value for '%1' is %2, must be greater than 0. Using default (%3) instead.")
                    .arg(maxObjectEventsName)
                    .arg(it.value())
                    .arg(this->maxObjectEvents));
        }
    }
    else {
        logWarn(QString("Value for '%1' not found. Using default (%2) instead.")
                .arg(maxObjectEventsName)
                .arg(this->maxObjectEvents));
    }

    return true;
}

bool Project::readEventScriptLabels() {
    globalScriptLabels.clear();
    for (const auto &filePath : getEventScriptsFilePaths())
        globalScriptLabels << ParseUtil::getGlobalScriptLabels(filePath);

    globalScriptLabels.sort(Qt::CaseInsensitive);
    globalScriptLabels.removeDuplicates();

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

    if (projectConfig.usePoryScript) {
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

void Project::loadEventPixmap(Event *event, bool forceLoad) {
    if (event && (event->getPixmap().isNull() || forceLoad))
        event->loadPixmap(this);
}

void Project::clearEventGraphics() {
    qDeleteAll(this->eventGraphicsMap);
    this->eventGraphicsMap.clear();
}

bool Project::readEventGraphics() {
    clearEventGraphics();

    const QString pointersFilepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_pointers);
    const QString gfxInfoFilepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx_info);
    const QString picTablesFilepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_pic_tables);
    const QString gfxFilepath = projectConfig.getFilePath(ProjectFilePath::data_obj_event_gfx);
    fileWatcher.addPaths({pointersFilepath, gfxInfoFilepath, picTablesFilepath, gfxFilepath});

    // Read the table mapping OBJ_EVENT_GFX constants to the names of pointers to data about their graphics.
    const QString pointersName = projectConfig.getIdentifier(ProjectIdentifier::symbol_obj_event_gfx_pointers);
    const QMap<QString, QString> pointerMap = parser.readNamedIndexCArray(pointersFilepath, pointersName);

    // The positions of each of the required members for the gfx info struct.
    // For backwards compatibility if the struct doesn't use initializers.
    static const QHash<int, QString> gfxInfoMemberMap = {
        {8, "inanimate"},
        {11, "oam"},
        {12, "subspriteTables"},
        {14, "images"},
    };
    // Read the structs containing data about each of the event sprites.
    auto gfxInfos = parser.readCStructs(gfxInfoFilepath, "", gfxInfoMemberMap);

    // We need data in both of these files to translate data from the structs above into the path for a .png file.
    const QMap<QString, QStringList> picTables = parser.readCArrayMulti(picTablesFilepath);
    const QMap<QString, QString> graphicIncbins = parser.readCIncbinMulti(gfxFilepath);

    for (auto i = this->gfxDefines.constBegin(); i != this->gfxDefines.constEnd(); i++) {
        const QString gfxName = i.key();

        // Strip the address-of operator to get the pointer's name. We'll use this name to get data about the event's sprite.
        // If we don't recognize the name, ignore it. The event will use a default sprite.
        QString info_label = pointerMap[gfxName].replace("&", "");
        if (!gfxInfos.contains(info_label))
            continue;
        const QHash<QString, QString> gfxInfoAttributes = gfxInfos[info_label];

        auto gfx = new EventGraphics;

        // We need the .png filepath for the event's sprite. This is buried behind a few levels of indirection.
        // The 'images' field gives us the name of the table containing the sprite's image data.
        // The entries in this table are expected to be in the format (PngSymbolName, ...).
        // We extract the symbol name of the .png's INCBIN'd data by looking at the first entry in this table.
        // Once we have the .png's symbol name we can get the actual filepath from its INCBIN.
        QString gfx_label = picTables[gfxInfoAttributes.value("images")].value(0);
        static const QRegularExpression re_parens("[\\(\\)]");
        gfx_label = gfx_label.section(re_parens, 1, 1);
        gfx->filepath = fixGraphicPath(graphicIncbins[gfx_label]);

        // Note: gfx has a 'spritesheet' field that will contain a QImage for the event's sprite.
        //       We don't create this QImage yet. Reading the image now is unnecessary overhead for startup.
        //       We'll read the image file when the event's sprite is first requested to be drawn.

        // The .png file is expected to be a spritesheet that can have multiple frames.
        // We only want to show one frame at a time, so we need to know the dimensions of each frame.
        // TODO: Describe different ways we read these. Use width/height?
        static const QRegularExpression re("\\S+_(\\d+)x(\\d+)");
        QRegularExpressionMatch dimensionMatch = re.match(gfxInfoAttributes.value("oam"));
        QRegularExpressionMatch oamTablesMatch = re.match(gfxInfoAttributes.value("subspriteTables"));
        if (oamTablesMatch.hasMatch()) {
            gfx->spriteWidth = oamTablesMatch.captured(1).toInt(nullptr, 0);
            gfx->spriteHeight = oamTablesMatch.captured(2).toInt(nullptr, 0);
        } else if (dimensionMatch.hasMatch()) {
            gfx->spriteWidth = dimensionMatch.captured(1).toInt(nullptr, 0);
            gfx->spriteHeight = dimensionMatch.captured(2).toInt(nullptr, 0);
        }

        // Inanimate events will only ever use the first frame of their spritesheet.
        gfx->inanimate = ParseUtil::gameStringToBool(gfxInfoAttributes.value("inanimate"));

        this->eventGraphicsMap.insert(gfxName, gfx);
    }
    return true;
}

QPixmap Project::getEventPixmap(const QString &gfxName, const QString &movementName) {
    struct FrameData {
        int index = 0;
        bool hFlip = false;
    };
    // TODO: Expose as a setting to users
    static const QMap<QString, FrameData> directionToFrameData = {
        {"DIR_SOUTH", { .index = 0, .hFlip = false }},
        {"DIR_NORTH", { .index = 1, .hFlip = false }},
        {"DIR_WEST",  { .index = 2, .hFlip = false }},
        {"DIR_EAST",  { .index = 2, .hFlip = true }}, // East-facing sprite is just the West-facing sprite mirrored
    };
    const QString direction = this->facingDirections.value(movementName, "DIR_SOUTH");
    auto frameData = directionToFrameData.value(direction);
    return getEventPixmap(gfxName, frameData.index, frameData.hFlip);
}

QPixmap Project::getEventPixmap(const QString &gfxName, int frame, bool hFlip) {
    EventGraphics* gfx = this->eventGraphicsMap.value(gfxName, nullptr);
    if (!gfx) {
        // Invalid gfx constant. If this is a number, try to use that instead.
        bool ok;
        int gfxNum = ParseUtil::gameStringToInt(gfxName, &ok);
        if (ok && gfxNum < this->gfxDefines.count()) {
            gfx = this->eventGraphicsMap.value(this->gfxDefines.key(gfxNum, "NULL"), nullptr);
        }
    }
    if (gfx && !gfx->loaded) {
        // This is the first request for this event's sprite. We'll attempt to load it now.
        if (!gfx->filepath.isEmpty()) {
            gfx->spritesheet = QImage(QString("%1/%2").arg(this->root).arg(gfx->filepath));
            if (gfx->spritesheet.isNull()) {
                logWarn(QString("Failed to open '%1' for event's sprite. Event will use a default sprite instead.").arg(gfx->filepath));
            } else {
                // If we were unable to find the dimensions of a frame within the spritesheet we'll use the full image dimensions.
                if (gfx->spriteWidth <= 0) {
                    gfx->spriteWidth = gfx->spritesheet.width();
                }
                if (gfx->spriteHeight <= 0) {
                    gfx->spriteHeight = gfx->spritesheet.height();
                }
            }
        }
        // Set this whether we were successful or not, we only need to try to load it once.
        gfx->loaded = true;
    }
    if (!gfx || gfx->spritesheet.isNull()) {
        // Either we didn't recognize the gfxName, or we were unable to load the sprite's image.
        return QPixmap();
    }

    QImage img;
    if (gfx->inanimate) {
        img = gfx->spritesheet.copy(0, 0, gfx->spriteWidth, gfx->spriteHeight);
    } else {
        int x = 0;
        int y = 0;

        // Get frame's position in spritesheet.
        // Assume horizontal layout. If position would exceed sheet width, try vertical layout.
        if ((frame + 1) * gfx->spriteWidth <= gfx->spritesheet.width()) {
            x = frame * gfx->spriteWidth;
        } else if ((frame + 1) * gfx->spriteHeight <= gfx->spritesheet.height()) {
            y = frame * gfx->spriteHeight;
        }

        img = gfx->spritesheet.copy(x, y, gfx->spriteWidth, gfx->spriteHeight);
        if (hFlip) {
            img = img.transformed(QTransform().scale(-1, 1));
        }
    }
    // Set first palette color fully transparent.
    img.setColor(0, qRgba(0, 0, 0, 0));
    QPixmap pixmap = QPixmap::fromImage(img);

    // TODO: Cache?
    return pixmap;
}

QPixmap Project::getEventPixmap(Event::Group) {
    // TODO
    return QPixmap();
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
    const QStringList regexList = {QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix))};
    const QString constantsFilename = projectConfig.getFilePath(ProjectFilePath::constants_species);
    fileWatcher.addPath(root + "/" + constantsFilename);
    QStringList speciesNames = parser.readCDefineNames(constantsFilename, regexList);
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

QPixmap Project::getSpeciesIcon(const QString &species) const {
    QPixmap pixmap;
    if (!QPixmapCache::find(species, &pixmap)) {
        // Prefer path from config. If not present, use the path parsed from project files
        QString path = projectConfig.getPokemonIconPath(species);
        if (path.isEmpty()) {
            path = this->speciesToIconPath.value(species);
        } else {
            path = Project::getExistingFilepath(path);
        }

        QImage img(path);
        if (img.isNull()) {
            // No icon for this species, use placeholder
            static const QPixmap placeholder = QPixmap(QStringLiteral(":images/pokemon_icon_placeholder.png"));
            pixmap = placeholder;
        } else {
            img.setColor(0, qRgba(0, 0, 0, 0));
            pixmap = QPixmap::fromImage(img).copy(0, 0, 32, 32);
            QPixmapCache::insert(species, pixmap);
        }
    }
    return pixmap;
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

int Project::getDefaultMapDimension()
{
    return Project::default_map_dimension;
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
        default_map_dimension = 20;
    } else if (max >= getMapDataSize(1, 1)) {
        // Below equation derived from max >= (x + 15) * (x + 14)
        // x^2 + 29x + (210 - max), then complete the square and simplify
        default_map_dimension = qFloor((qSqrt(4 * getMaxMapDataSize() + 1) - 29) / 2);
    } else {
        logError(QString("'%1' of %2 is too small to support a 1x1 map. Must be at least %3.")
                    .arg(projectConfig.getIdentifier(ProjectIdentifier::define_map_size))
                    .arg(max)
                    .arg(getMapDataSize(1, 1)));
        return false;
    }
    return true;
}

// Object events have their own limit specified by ProjectIdentifier::define_obj_event_count.
// The default value for this is 64. All events (object events included) are also limited by
// the data types of the event counters in the project. This would normally be u8, so the limit is 255.
// We let the users tell us this limit in case they change these data types.
int Project::getMaxEvents(Event::Group group) {
    if (group == Event::Group::Object)
        return qMin(this->maxObjectEvents, projectConfig.maxEventsPerGroup);
    return projectConfig.maxEventsPerGroup;
}

QString Project::getEmptyMapDefineName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_map_empty);
}

QString Project::getDynamicMapDefineName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_map_dynamic);
}

QString Project::getDynamicMapName() {
    return projectConfig.getIdentifier(ProjectIdentifier::symbol_dynamic_map_name);
}

QString Project::getEmptySpeciesName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_species_empty);
}

// If the provided filepath is an absolute path to an existing file, return filepath.
// If not, and the provided filepath is a relative path from the project dir to an existing file, return the relative path.
// Otherwise return empty string.
QString Project::getExistingFilepath(QString filepath) {
    if (filepath.isEmpty() || QFile::exists(filepath))
        return filepath;

    filepath = QDir::cleanPath(projectConfig.projectDir + QDir::separator() + filepath);
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
    uint32_t maxMask = Metatile::getMaxAttributesMask();
    projectConfig.metatileBehaviorMask &= maxMask;
    projectConfig.metatileTerrainTypeMask &= maxMask;
    projectConfig.metatileEncounterTypeMask &= maxMask;
    projectConfig.metatileLayerTypeMask &= maxMask;

    Block::setLayout();
    Metatile::setLayout(this);

    Project::num_metatiles_primary = qMin(qMax(Project::num_metatiles_primary, 1), Block::getMaxMetatileId() + 1);
    projectConfig.defaultMetatileId = qMin(projectConfig.defaultMetatileId, Block::getMaxMetatileId());
    projectConfig.defaultElevation = qMin(projectConfig.defaultElevation, Block::getMaxElevation());
    projectConfig.defaultCollision = qMin(projectConfig.defaultCollision, Block::getMaxCollision());
    projectConfig.collisionSheetHeight = qMin(qMax(projectConfig.collisionSheetHeight, 1), Block::getMaxElevation() + 1);
    projectConfig.collisionSheetWidth = qMin(qMax(projectConfig.collisionSheetWidth, 1), Block::getMaxCollision() + 1);
}

bool Project::hasUnsavedChanges() {
    if (this->hasUnsavedDataChanges)
        return true;

    // Check layouts for unsaved changes
    for (auto i = this->mapLayouts.constBegin(); i != this->mapLayouts.constEnd(); i++) {
        auto layout = i.value();
        if (layout && layout->hasUnsavedChanges())
            return true;
    }

    // Check loaded maps for unsaved changes
    for (auto i = this->mapCache.constBegin(); i != this->mapCache.constEnd(); i++) {
        auto map = i.value();
        if (map && map->hasUnsavedChanges())
            return true;
    }
    return false;
}

// TODO: This belongs in a more general utility file, once we have one.
// Sometimes we want to sort names alphabetically to make them easier to find in large combo box lists.
// QStringList::sort (as of writing) can only sort numbers in lexical order, which has an undesirable
// effect (e.g. MAPSEC_ROUTE_10 comes after MAPSEC_ROUTE_1, rather than MAPSEC_ROUTE_9).
// We can use QCollator to sort these lists with better handling for numbers.
void Project::numericalModeSort(QStringList &list) {
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(list.begin(), list.end(), collator);
}
