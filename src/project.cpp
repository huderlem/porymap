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
#include "utility.h"

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

Project::Project(QObject *parent) :
    QObject(parent)
{
    QObject::connect(&this->fileWatcher, &QFileSystemWatcher::fileChanged, this, &Project::recordFileChange);
}

Project::~Project()
{
    clearMaps();
    clearTilesetCache();
    clearMapLayouts();
    clearEventGraphics();
    clearHealLocations();
    QPixmapCache::clear();
}

void Project::setRoot(const QString &dir) {
    this->root = dir;
    FileDialog::setDirectory(dir);
    this->parser.setRoot(dir);
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

// Porymap projects have no standardized way for Porymap to determine whether they're compatible as of the latest breaking changes.
// We can use the project's git history (if it has one, and we're able to get it) to make a reasonable guess.
// We know the hashes of the commits in the base repos that contain breaking changes, so if we find one of these then the project
// should support at least up to that Porymap major version. If this fails for any reason it returns a version of -1.
int Project::getSupportedMajorVersion(QString *errorOut) {
    // This has relatively tight timeout windows (500ms for each process, compared to the default 30,000ms). This version check
    // is not important enough to significantly slow down project launch, we'd rather just timeout.
    const int timeoutLimit = 500;
    const int failureVersion = -1;
    QString gitName = "git";
    QString gitPath = QStandardPaths::findExecutable(gitName);
    if (gitPath.isEmpty()) {
        if (errorOut) *errorOut = QString("Unable to locate %1.").arg(gitName);
        return failureVersion;
    }

    QProcess process;
    process.setWorkingDirectory(this->root);
    process.setProgram(gitPath);
    process.setReadChannel(QProcess::StandardOutput);
    process.setStandardInputFile(QProcess::nullDevice()); // We won't have any writing to do.

    // First we need to know which (if any) known history this project belongs to.
    // We'll get the root commit, then compare it to the known root commits for the base project repos.
    static const QStringList args_getRootCommit = { "rev-list", "--max-parents=0", "HEAD" };
    process.setArguments(args_getRootCommit);
    process.start();
    if (!process.waitForFinished(timeoutLimit) || process.exitStatus() != QProcess::ExitStatus::NormalExit || process.exitCode() != 0) {
        if (errorOut) {
            *errorOut = QStringLiteral("Failed to identify commit history");
            if (process.error() != QProcess::UnknownError && !process.errorString().isEmpty()) {
                errorOut->append(QString(": %1").arg(process.errorString()));
            } else {
                process.setReadChannel(QProcess::StandardError);
                QString error = QString(process.readLine()).remove('\n');
                if (!error.isEmpty()) errorOut->append(QString(": %1").arg(error));
            }
        }
        return failureVersion;
    }
    const QString rootCommit = QString(process.readLine()).remove('\n');

    // The keys in this map are the hashes of the root commits for each of the 3 base repos.
    // The values are a list of pairs, where the first element is a major version number, and the
    // second element is the hash of the earliest commit that supports that major version.
    static const QMap<QString, QList<QPair<int, QString>>> historyMap = {
        // pokeemerald
        {"33b799c967fd63d04afe82eecc4892f3e45781b3", {
            {6, "0c32d840fae64c612b01ca8b004d1f95b7f798ba"},
            {5, "c76beed98990a57c84d3930190fd194abfedf7e8"},
            {4, "cb5b8da77b9ba6837fcc8c5163bedc5008b12c2c"},
            {3, "204c431993dad29661a9ff47326787cd0cf381e6"},
            {2, "cdae0c1444bed98e652c87dc3e3edcecacfef8be"},
            {1, ""}
        }},
        // pokefirered
        {"670fef77ac4d9116d5fdc28c0da40622919a062b", {
            {6, "f60d28ceb58cd6e7064e34c34e56d09b84420355"},
            {5, "52591dcee42933d64f60c59276fc13c3bb89c47b"},
            {4, "200c82e01a94dbe535e6ed8768d8afad4444d4d2"},
        }},
        // pokeruby
        {"1362b60f3467f0894d55e82f3294980b6373021d", {
            {6, "5eb76d89ce5c41feeba848abe10b8e52b1add84c"},
            {5, "d99cb43736dd1d4ee4820f838cb259d773d8bf25"},
            {4, "f302fcc134bf354c3655e3423be68fd7a99cb396"},
            {3, "b4f4d2c0f03462dcdf3492aad27890294600eb2e"},
            {2, "0e8ccfc4fd3544001f4c25fafd401f7558bdefba"},
            {1, ""}
        }},
    };
    if (!historyMap.contains(rootCommit)) {
        // Either this repo does not share history with one of the base repos, or we got some unexpected result.
        if (errorOut) *errorOut = QStringLiteral("Unrecognized commit history");
        return failureVersion;
    }

    // We now know which base repo that the user's repo shares history with.
    // Next we check to see if it contains the changes required to support particular major versions of Porymap.
    // We'll start with the most recent major version and work backwards.
    for (const auto &pair : historyMap.value(rootCommit)) {
        int versionNum = pair.first;
        QString commitHash = pair.second;
        if (commitHash.isEmpty()) {
            // An empty commit hash means 'consider any point in the history a supported version'
            return versionNum;
        }
        process.setArguments({ "merge-base", "--is-ancestor", commitHash, "HEAD" });
        process.start();
        if (!process.waitForFinished(timeoutLimit) || process.exitStatus() != QProcess::ExitStatus::NormalExit) {
            if (errorOut) {
                *errorOut = QStringLiteral("Failed to search commit history");
                if (process.error() != QProcess::UnknownError && !process.errorString().isEmpty()) {
                    errorOut->append(QString(": %1").arg(process.errorString()));
                } else {
                    process.setReadChannel(QProcess::StandardError);
                    QString error = QString(process.readLine()).remove('\n');
                    if (!error.isEmpty()) errorOut->append(QString(": %1").arg(error));
                }
            }
            return failureVersion;
        }
        if (process.exitCode() == 0) {
            // Identified a supported major version
            return versionNum;
        }
    }
    // We recognized the commit history, but it's too old for any version of Porymap to support.
    return 0;
}

bool Project::load() {
    this->parser.setUpdatesSplashScreen(true);
    resetFileCache();
    this->disabledSettingsNames.clear();
    bool success = readGlobalConstants()
                && readMapLayouts()
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
    this->parser.setUpdatesSplashScreen(false);
    return success;
}

void Project::resetFileCache() {
    this->parser.clearFileCache();
    this->failedFileWatchPaths.clear();

    const QSet<QString> filepaths = {
        // Whenever we load a tileset we'll need to parse some data from these files, so we cache them to avoid the overhead of opening the files.
        // We don't know yet whether the project uses C or asm tileset data, so try to cache both (we'll ignore errors from missing files).
        projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm),
        projectConfig.getFilePath(ProjectFilePath::tilesets_graphics_asm),
        projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles_asm),
        projectConfig.getFilePath(ProjectFilePath::tilesets_headers),
        projectConfig.getFilePath(ProjectFilePath::tilesets_graphics),
        projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles),
        // We need separate sets of constants from these files
        projectConfig.getFilePath(ProjectFilePath::constants_map_types),
        projectConfig.getFilePath(ProjectFilePath::global_fieldmap),
        projectConfig.getFilePath(ProjectFilePath::constants_weather),
    };
    for (const auto &path : filepaths) {
        if (this->parser.cacheFile(path)) {
            watchFile(path);
        }
    }
}

QString Project::getProjectTitle() const {
    if (!root.isNull()) {
        return root.section('/', -1);
    } else {
        return QString();
    }
}

void Project::clearMaps() {
    qDeleteAll(this->maps);
    this->maps.clear();
    this->loadedMapNames.clear();
}

void Project::clearTilesetCache() {
    qDeleteAll(this->tilesetCache);
    this->tilesetCache.clear();
}

Map* Project::loadMap(const QString &mapName) {
    if (mapName == getDynamicMapName()) {
        // Silently ignored, caller is expected to handle this if they want this to be an error.
        return nullptr;
    }

    // Some maps are ignored while opening the project because they have invalid or incomplete data.
    // We already logged a warning about this, but now that we're trying to load the map it's an error.
    auto it = this->erroredMaps.constFind(mapName);
    if (it != this->erroredMaps.constEnd()) {
        logError(it.value());
        return nullptr;
    }

    Map* map = this->maps.value(mapName);
    if (!map) {
        logError(QString("Unknown map name '%1'.").arg(mapName));
        return nullptr;
    }

    if (isLoadedMap(mapName))
        return map;

    if (!loadMapData(map))
        return nullptr;

    // Load map layout
    if (map->isPersistedToFile() && !map->hasUnsavedChanges()) {
        if (!loadLayout(map->layoutId()))
            return nullptr;
    }

    this->loadedMapNames.insert(mapName);
    emit mapLoaded(map);

    return map;
}

Layout *Project::loadLayout(const QString &layoutId) {
    Layout *layout = this->mapLayouts.value(layoutId);
    if (!layout) {
        logError(QString("Unknown layout ID '%1'.").arg(layoutId));
        return nullptr;
    }

    if (isLoadedLayout(layoutId))
        return layout;

    // Force these to run even if one fails
    bool loadedTilesets = loadLayoutTilesets(layout);
    bool loadedBlockdata = layout->loadBlockdata(this->root);
    bool loadedBorder = layout->loadBorder(this->root);
    if (!loadedTilesets || !loadedBlockdata || !loadedBorder) {
        // Error should already be logged.
        return nullptr;
    }

    this->loadedLayoutIds.insert(layoutId);
    return layout;
}

QSet<QString> Project::getTopLevelMapFields() const {
    QSet<QString> fields = {
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
    if (projectConfig.mapAllowFlagsEnabled) {
        fields.insert("allow_cycling");
        fields.insert("allow_escaping");
        fields.insert("allow_running");
    }
    if (projectConfig.floorNumberEnabled) {
        fields.insert("floor_number");
    }
    return fields;
}

QJsonDocument Project::readMapJson(const QString &mapName, QString *error) {
    // Note: We are explicitly not adding mapFilepath to the fileWatcher here.
    //       All map.json files are read at launch, and adding them all to the filewatcher
    //       can easily exceed the 256 file limit that exists on some platforms.
    const QString mapFilepath = Map::getJsonFilepath(mapName);
    QJsonDocument doc;
    if (!parser.tryParseJsonFile(&doc, mapFilepath, error)) {
        if (error) {
            error->prepend(QString("Failed to read map data from '%1': ").arg(mapFilepath));
        }
    }
    return doc;
}

bool Project::loadMapEvent(Map *map, QJsonObject json, Event::Type defaultType) {
    QString typeString = ParseUtil::jsonToQString(json.take("type"));
    Event::Type type = typeString.isEmpty() ? defaultType : Event::typeFromJsonKey(typeString);
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

    QString error;
    QJsonDocument mapDoc = readMapJson(map->name(), &error);
    if (!error.isEmpty()) {
        logError(error);
        return false;
    }

    QJsonObject mapObj = mapDoc.object();
    QString name = ParseUtil::jsonToQString(mapObj.take("name"));
    if (name != map->name()) {
        logWarn(QString("Mismatched name '%1' for map '%2' will be overwritten.").arg(name).arg(map->name()));
    }

    // We should already know the map constant ID from the initial project launch, but we'll ensure it's correct here anyway.
    map->setConstantName(ParseUtil::jsonToQString(mapObj.take("id")));
    this->mapConstantsToMapNames.insert(map->constantName(), map->name());

    const QString layoutId = ParseUtil::jsonToQString(mapObj.take("layout"));
    Layout* layout = this->mapLayouts.value(layoutId);
    if (!layout) {
        // We've already verified layout IDs on project launch and ignored maps with invalid IDs, so this shouldn't happen.
        logError(QString("Cannot load map with unknown layout ID '%1'").arg(layoutId));
        return false;
    }
    map->setLayout(layout);

    map->header()->setSong(ParseUtil::jsonToQString(mapObj.take("music")));
    map->header()->setLocation(ParseUtil::jsonToQString(mapObj.take("region_map_section")));
    map->header()->setRequiresFlash(ParseUtil::jsonToBool(mapObj.take("requires_flash")));
    map->header()->setWeather(ParseUtil::jsonToQString(mapObj.take("weather")));
    map->header()->setType(ParseUtil::jsonToQString(mapObj.take("map_type")));
    map->header()->setShowsLocationName(ParseUtil::jsonToBool(mapObj.take("show_map_name")));
    map->header()->setBattleScene(ParseUtil::jsonToQString(mapObj.take("battle_scene")));

    if (projectConfig.mapAllowFlagsEnabled) {
        map->header()->setAllowsBiking(ParseUtil::jsonToBool(mapObj.take("allow_cycling")));
        map->header()->setAllowsEscaping(ParseUtil::jsonToBool(mapObj.take("allow_escaping")));
        map->header()->setAllowsRunning(ParseUtil::jsonToBool(mapObj.take("allow_running")));
    }
    if (projectConfig.floorNumberEnabled) {
        map->header()->setFloorNumber(ParseUtil::jsonToInt(mapObj.take("floor_number")));
    }
    map->setSharedEventsMap(ParseUtil::jsonToQString(mapObj.take("shared_events_map")));
    map->setSharedScriptsMap(ParseUtil::jsonToQString(mapObj.take("shared_scripts_map")));

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
        const QJsonArray eventsJsonArr = mapObj.take(eventGroupKey).toArray();
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
    QJsonArray connectionsArr = mapObj.take("connections").toArray();
    if (!connectionsArr.isEmpty()) {
        for (int i = 0; i < connectionsArr.size(); i++) {
            QJsonObject connectionObj = connectionsArr[i].toObject();
            const QString direction = ParseUtil::jsonToQString(connectionObj.take("direction"));
            const int offset = ParseUtil::jsonToInt(connectionObj.take("offset"));
            const QString mapConstant = ParseUtil::jsonToQString(connectionObj.take("map"));
            auto connection = new MapConnection(this->mapConstantsToMapNames.value(mapConstant, mapConstant), direction, offset);
            connection->setCustomData(connectionObj);
            map->loadConnection(connection);
        }
    }
    map->setCustomAttributes(mapObj);

    return true;
}

Map *Project::createNewMap(const Project::NewMapSettings &settings, const Map* toDuplicate) {
    Map *map = toDuplicate ? new Map(*toDuplicate) : new Map;
    map->setName(settings.name);
    map->setHeader(settings.header);
    map->setNeedsHealLocation(settings.canFlyTo);

    // Generate a unique MAP constant.
    map->setConstantName(toUniqueIdentifier(map->expectedConstantName()));

    if (!this->groupNames.contains(settings.group)) {
        // Adding map to a map group that doesn't exist yet.
        if (isValidNewIdentifier(settings.group)) {
            addNewMapGroup(settings.group);
        } else {
            logError(QString("Cannot create new map with invalid map group name '%1'.").arg(settings.group));
            delete map;
            return nullptr;
        }
    }

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
        if (!loadLayout(settings.layout.id)) {
            // Layout failed to load. For now we can just record the ID.
            map->setLayoutId(settings.layout.id);
        }
    }
    map->setLayout(layout);

    // Try to record the MAPSEC name in case this is a new name.
    addNewMapsec(map->header()->location());

    this->groupNameToMapNames[settings.group].append(map->name());
    this->mapConstantsToMapNames.insert(map->constantName(), map->name());
    this->alphabeticalMapNames.append(map->name());
    Util::numericalModeSort(this->alphabeticalMapNames);

    map->setIsPersistedToFile(false);
    this->maps.insert(map->name(), map);

    emit mapCreated(map, settings.group);

    return map;
}

Layout *Project::createNewLayout(const Layout::Settings &settings, const Layout *toDuplicate) {
    if (this->mapLayouts.contains(settings.id))
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
    layout->newFolderPath = folderPath;
    layout->border_path = folderPath + "/border.bin";
    layout->blockdata_path = folderPath + "/map.bin";

    if (layout->blockdata.isEmpty()) {
        // Fill layout using default fill settings
        setNewLayoutBlockdata(layout);
    }
    if (layout->border.isEmpty()) {
        // Fill border using default fill settings
        setNewLayoutBorder(layout);
    }

    // No need for a full load, we already have all the blockdata.
    if (!loadLayoutTilesets(layout)) {
        delete layout;
        return nullptr;
    }

    this->mapLayouts.insert(layout->id, layout);
    this->orderedLayoutIds.append(layout->id);
    this->loadedLayoutIds.insert(layout->id);
    this->alphabeticalLayoutIds.append(layout->id);
    Util::numericalModeSort(this->alphabeticalLayoutIds);

    emit layoutCreated(layout);

    return layout;
}

void Project::clearMapLayouts() {
    qDeleteAll(this->mapLayouts);
    this->mapLayouts.clear();
    qDeleteAll(this->mapLayoutsMaster);
    this->mapLayoutsMaster.clear();
    this->alphabeticalLayoutIds.clear();
    this->orderedLayoutIds.clear();
    this->orderedLayoutIdsMaster.clear();
    this->loadedLayoutIds.clear();
    this->customLayoutsData = QJsonObject();
    this->failedLayoutsData.clear();
}

bool Project::readMapLayouts() {
    clearMapLayouts();

    const QString layoutsFilepath = projectConfig.getFilePath(ProjectFilePath::json_layouts);
    watchFile(layoutsFilepath);
    QJsonDocument layoutsDoc;
    QString error;
    if (!parser.tryParseJsonFile(&layoutsDoc, layoutsFilepath, &error)) {
        logError(QString("Failed to read map layouts from '%1': %2").arg(layoutsFilepath).arg(error));
        return false;
    }

    QJsonObject layoutsObj = layoutsDoc.object();

    this->layoutsLabel = ParseUtil::jsonToQString(layoutsObj.take("layouts_table_label"));
    if (this->layoutsLabel.isEmpty()) {
        this->layoutsLabel = "gMapLayouts";
        logWarn(QString("'layouts_table_label' value is missing from %1. Defaulting to %2")
                 .arg(layoutsFilepath)
                 .arg(layoutsLabel));
    }

    QJsonArray layouts = layoutsObj.take("layouts").toArray();
    for (int i = 0; i < layouts.size(); i++) {
        QJsonObject layoutObj = layouts[i].toObject();
        if (layoutObj.isEmpty())
            continue;

        QScopedPointer<Layout> layout(new Layout());
        layout->id = ParseUtil::jsonToQString(layoutObj.take("id"));
        if (layout->id.isEmpty()) {
            // Use name to identify it in the warning, if available.
            QString name = ParseUtil::jsonToQString(layoutObj["name"]);
            if (name.isEmpty()) name = QString("Layout %1 (unnamed)").arg(i);
            logWarn(QString("Missing 'id' value for %1 in %2").arg(name).arg(layoutsFilepath));
            this->failedLayoutsData.append(layouts[i].toObject());
            continue;
        }
        if (this->mapLayouts.contains(layout->id)) {
            logWarn(QString("Duplicate layout entry for %1 in %2 will be ignored.").arg(layout->id).arg(layoutsFilepath));
            this->failedLayoutsData.append(layouts[i].toObject());
            continue;
        }
        layout->name = ParseUtil::jsonToQString(layoutObj.take("name"));
        if (layout->name.isEmpty()) {
            logWarn(QString("Missing 'name' value for %1 in %2").arg(layout->id).arg(layoutsFilepath));
            this->failedLayoutsData.append(layouts[i].toObject());
            continue;
        }

        layout->width = ParseUtil::jsonToInt(layoutObj.take("width"));
        layout->height = ParseUtil::jsonToInt(layoutObj.take("height"));
        if (projectConfig.useCustomBorderSize) {
            layout->border_width = ParseUtil::jsonToInt(layoutObj.take("border_width"));
            layout->border_height = ParseUtil::jsonToInt(layoutObj.take("border_height"));
        } else {
            layout->border_width = DEFAULT_BORDER_WIDTH;
            layout->border_height = DEFAULT_BORDER_HEIGHT;
        }
        layout->tileset_primary_label = ParseUtil::jsonToQString(layoutObj.take("primary_tileset"));
        layout->tileset_secondary_label = ParseUtil::jsonToQString(layoutObj.take("secondary_tileset"));
        layout->border_path = ParseUtil::jsonToQString(layoutObj.take("border_filepath"));
        layout->blockdata_path = ParseUtil::jsonToQString(layoutObj.take("blockdata_filepath"));

        layout->customData = layoutObj;

        this->mapLayouts.insert(layout->id, layout->copy());
        this->mapLayoutsMaster.insert(layout->id, layout->copy());
        this->orderedLayoutIds.append(layout->id);
        this->orderedLayoutIdsMaster.append(layout->id);
    }

    if (this->mapLayouts.isEmpty()) {
        logError(QString("Failed to read any map layouts from '%1'. At least one map layout is required.").arg(layoutsFilepath));
        return false;
    }

    this->alphabeticalLayoutIds = this->orderedLayoutIds;
    Util::numericalModeSort(this->alphabeticalLayoutIds);

    this->customLayoutsData = layoutsObj;

    return true;
}

bool Project::saveMapLayouts() {
    QString layoutsFilepath = root + "/" + projectConfig.getFilePath(ProjectFilePath::json_layouts);
    QFile layoutsFile(layoutsFilepath);
    if (!layoutsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(layoutsFilepath).arg(layoutsFile.errorString()));
        return false;
    }

    OrderedJson::object layoutsObj;
    layoutsObj["layouts_table_label"] = this->layoutsLabel;

    OrderedJson::array layoutsArr;
    for (const QString &layoutId : this->orderedLayoutIdsMaster) {
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
        OrderedJson::append(&layoutObj, layout->customData);
        layoutsArr.push_back(layoutObj);
    }
    // Append any layouts that were hidden because we failed to load them at launch.
    // We do a little extra work to keep the field order the same as successfully-loaded layouts.
    for (QJsonObject failedData : this->failedLayoutsData) {
        OrderedJson::object layoutObj;
        static const QStringList expectedFields = {
            "id", "name", "width", "height", "border_width", "border_height",
            "primary_tileset", "secondary_tileset", "border_filepath", "blockdata_filepath"
        };
        for (const auto &field : expectedFields) {
            if (failedData.contains(field)) {
                layoutObj[field] = OrderedJson::fromQJsonValue(failedData.take(field));
            }
        }
        OrderedJson::append(&layoutObj, failedData);
        layoutsArr.push_back(layoutObj);
    }
    layoutsObj["layouts"] = layoutsArr;
    OrderedJson::append(&layoutsObj, this->customLayoutsData);

    ignoreWatchedFileTemporarily(layoutsFilepath);

    OrderedJson layoutJson(layoutsObj);
    OrderedJsonDoc jsonDoc(&layoutJson);
    jsonDoc.dump(&layoutsFile);
    layoutsFile.close();
    return true;
}

bool Project::watchFile(const QString &filename) {
    QString filepath = filename.startsWith(this->root) ? filename : QString("%1/%2").arg(this->root).arg(filename);
    if (!this->fileWatcher.addPath(filepath) && !this->fileWatcher.files().contains(filepath)) {
        // We failed to watch the file, and this wasn't a file we were already watching.
        // Log a warning, but only if A. we actually care that we failed, because 'monitor files' is enabled,
        // B. we haven't logged a warning for this file yet, and C. we would have otherwise been able to watch it, because the file exists.
        if (porymapConfig.monitorFiles && !this->failedFileWatchPaths.contains(filepath) && QFileInfo::exists(filepath)) {
            this->failedFileWatchPaths.insert(filepath);
            logWarn(QString("Failed to add '%1' to file watcher. Currently watching %2 files.")
                            .arg(Util::stripPrefix(filepath, this->root))
                            .arg(this->fileWatcher.files().length()));
        }
        return false;
    }
    return true;
}

bool Project::watchFiles(const QStringList &filenames) {
    bool success = true;
    for (const auto &filename : filenames) {
        if (!watchFile(filename)) success = false;
    }
    return success;
}

bool Project::stopFileWatch(const QString &filename) {
    QString filepath = filename.startsWith(this->root) ? filename : QString("%1/%2").arg(this->root).arg(filename);
    return this->fileWatcher.removePath(filepath);
}

void Project::ignoreWatchedFileTemporarily(const QString &filepath) {
    // Ignore any file-change events for this filepath for the next 5 seconds.
    this->modifiedFileTimestamps.insert(filepath, QDateTime::currentMSecsSinceEpoch() + 5000);
}

void Project::ignoreWatchedFilesTemporarily(const QStringList &filepaths) {
    for (const auto &filepath : filepaths) {
        ignoreWatchedFileTemporarily(filepath);
    }
}

void Project::recordFileChange(const QString &filepath) {
    // --From the Qt manual--
    // Note: As a safety measure, many applications save an open file by writing a new file and then deleting the old one.
    //       In your slot function, you can check watcher.files().contains(path).
    //       If it returns false, check whether the file still exists and then call addPath() to continue watching it.
    if (!this->fileWatcher.files().contains(filepath) && QFileInfo::exists(filepath)) {
        this->fileWatcher.addPath(filepath);
    }

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

bool Project::saveMapGroups() {
    QString mapGroupsFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_map_groups));
    QFile mapGroupsFile(mapGroupsFilepath);
    if (!mapGroupsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(mapGroupsFilepath).arg(mapGroupsFile.errorString()));
        return false;
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
            if (this->maps.value(mapName) && !this->maps.value(mapName)->isPersistedToFile()) {
                // This is a new map that hasn't been saved yet, don't add it to the global map groups list yet.
                continue;
            }
            groupArr.push_back(mapName);
        }
        mapGroupsObj[groupName] = groupArr;
    }
    OrderedJson::append(&mapGroupsObj, this->customMapGroupsData);

    ignoreWatchedFileTemporarily(mapGroupsFilepath);

    OrderedJson mapGroupJson(mapGroupsObj);
    OrderedJsonDoc jsonDoc(&mapGroupJson);
    jsonDoc.dump(&mapGroupsFile);
    mapGroupsFile.close();
    return true;
}

bool Project::saveRegionMapSections() {
    const QString filepath = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(ProjectFilePath::json_region_map_entries));
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(filepath).arg(file.errorString()));
        return false;
    }

    OrderedJson::array mapSectionArray;
    for (const auto &idName : this->mapSectionIdNamesSaveOrder) {
        const LocationData location = this->locationData.value(idName);

        OrderedJson::object mapSectionObj;
        mapSectionObj["id"] = idName;

        if (!location.displayName.isEmpty()) {
            mapSectionObj["name"] = location.displayName;
        }

        if (location.map.valid) {
            mapSectionObj["x"] = location.map.x;
            mapSectionObj["y"] = location.map.y;
            mapSectionObj["width"] = location.map.width;
            mapSectionObj["height"] = location.map.height;
        }
        OrderedJson::append(&mapSectionObj, location.custom);

        mapSectionArray.append(mapSectionObj);
    }

    OrderedJson::object object;
    object["map_sections"] = mapSectionArray;
    OrderedJson::append(&object, this->customMapSectionsData);

    ignoreWatchedFileTemporarily(filepath);
    OrderedJson json(object);
    OrderedJsonDoc jsonDoc(&json);
    jsonDoc.dump(&file);
    file.close();
    return true;
}

bool Project::saveWildMonData() {
    if (!this->wildEncountersLoaded) return true;

    QString wildEncountersJsonFilepath = QString("%1/%2").arg(root).arg(projectConfig.getFilePath(ProjectFilePath::json_wild_encounters));
    QFile wildEncountersFile(wildEncountersJsonFilepath);
    if (!wildEncountersFile.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(wildEncountersJsonFilepath).arg(wildEncountersFile.errorString()));
        return false;
    }

    OrderedJson::object wildEncountersObject;
    OrderedJson::array wildEncounterGroups;

    OrderedJson::object monHeadersObject;
    monHeadersObject["label"] = this->wildMonTableName;
    monHeadersObject["for_maps"] = true;

    OrderedJson::array fieldsInfoArray;
    for (EncounterField fieldInfo : this->wildMonFields) {
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

        OrderedJson::append(&fieldObject, fieldInfo.customData);
        fieldsInfoArray.append(fieldObject);
    }
    monHeadersObject["fields"] = fieldsInfoArray;

    OrderedJson::array encountersArray;
    for (auto keyPair : this->wildMonData) {
        QString key = keyPair.first;
        for (auto grouplLabelPair : this->wildMonData[key]) {
            QString groupLabel = grouplLabelPair.first;
            OrderedJson::object encounterObject;
            encounterObject["map"] = key;
            encounterObject["base_label"] = groupLabel;

            WildPokemonHeader encounterHeader = this->wildMonData[key][groupLabel];
            for (auto fieldNamePair : encounterHeader.wildMons) {
                QString fieldName = fieldNamePair.first;
                OrderedJson::object monInfoObject;
                WildMonInfo monInfo = encounterHeader.wildMons[fieldName];
                monInfoObject["encounter_rate"] = monInfo.encounterRate;
                OrderedJson::array monArray;
                for (WildPokemon wildMon : monInfo.wildPokemon) {
                    OrderedJson::object monEntry;
                    monEntry["min_level"] = wildMon.minLevel;
                    monEntry["max_level"] = wildMon.maxLevel;
                    monEntry["species"] = wildMon.species;
                    OrderedJson::append(&monEntry, wildMon.customData);
                    monArray.push_back(monEntry);
                }
                monInfoObject["mons"] = monArray;
                OrderedJson::append(&monInfoObject, monInfo.customData);

                encounterObject[fieldName] = monInfoObject;
                OrderedJson::append(&encounterObject, encounterHeader.customData);
            }
            encountersArray.push_back(encounterObject);
        }
    }
    monHeadersObject["encounters"] = encountersArray;
    OrderedJson::append(&monHeadersObject, this->customWildMonGroupData);

    wildEncounterGroups.push_back(monHeadersObject);
    OrderedJson::append(&wildEncounterGroups, this->extraEncounterGroups);

    wildEncountersObject["wild_encounter_groups"] = wildEncounterGroups;
    OrderedJson::append(&wildEncountersObject, this->customWildMonData);

    ignoreWatchedFileTemporarily(wildEncountersJsonFilepath);
    OrderedJson encounterJson(wildEncountersObject);
    OrderedJsonDoc jsonDoc(&encounterJson);
    jsonDoc.dump(&wildEncountersFile);
    wildEncountersFile.close();
    return true;
}

// For a map with a constant of 'MAP_FOO', returns a unique 'HEAL_LOCATION_FOO'.
// Because of how event ID names are checked it doesn't guarantee that the name
// won't be in-use by some map that hasn't been loaded yet.
QString Project::getNewHealLocationName(const Map* map) const {
    return !map ? QString() : toUniqueIdentifier(projectConfig.getIdentifier(ProjectIdentifier::define_heal_locations_prefix) + Util::toDefineCase(map->name()));
}

bool Project::saveHealLocations() {
    const QString filepath = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(ProjectFilePath::json_heal_locations));
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(filepath).arg(file.errorString()));
        return false;
    }

    // Build the JSON data for output.
    QMap<QString, QList<OrderedJson::object>> idNameToJson;
    for (const auto &events : this->healLocations) {
        for (const auto &event : events) {
            idNameToJson[event->getIdName()].append(event->buildEventJson(this));
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
    for (const auto &objects : idNameToJson) {
        for (const auto &object : objects) {
            eventJsonArr.push_back(object);
        }
    }

    OrderedJson::object object;
    object["heal_locations"] = eventJsonArr;
    OrderedJson::append(&object, this->customHealLocationsData);

    ignoreWatchedFileTemporarily(filepath);
    OrderedJson json(object);
    OrderedJsonDoc jsonDoc(&json);
    jsonDoc.dump(&file);
    file.close();
    return true;
}

bool Project::saveTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    bool success = saveTilesetMetatileLabels(primaryTileset, secondaryTileset);
    if (primaryTileset && !primaryTileset->save())
        success = false;
    if (secondaryTileset && !secondaryTileset->save())
        success = false;
    return success;
}

void Project::updateTilesetMetatileLabels(Tileset *tileset) {
    if (!tileset) return;

    // Erase old labels, then repopulate with new labels
    const QString prefix = tileset->getMetatileLabelPrefix();
    this->metatileLabelsMap[tileset->name].clear();
    for (auto i = tileset->metatileLabels.constBegin(); i != tileset->metatileLabels.constEnd(); i++) {
        uint16_t metatileId = i.key();
        QString label = i.value();
        if (!label.isEmpty()) {
            this->metatileLabelsMap[tileset->name][prefix + label] = metatileId;
        }
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

bool Project::saveTilesetMetatileLabels(Tileset *primaryTileset, Tileset *secondaryTileset) {
    // Skip writing the file if there are no labels in both the new and old sets
    if ((!primaryTileset || (metatileLabelsMap[primaryTileset->name].size() == 0 && primaryTileset->metatileLabels.size() == 0))
     && (!secondaryTileset || (metatileLabelsMap[secondaryTileset->name].size() == 0 && secondaryTileset->metatileLabels.size() == 0)))
        return true;

    updateTilesetMetatileLabels(primaryTileset);
    updateTilesetMetatileLabels(secondaryTileset);

    // Recreate metatile labels file
    const QString guardName = "GUARD_METATILE_LABELS_H";
    QString outputText = QString("#ifndef %1\n#define %1\n").arg(guardName);

    for (auto i = this->metatileLabelsMap.constBegin(); i != this->metatileLabelsMap.constEnd(); i++) {
        const QString tilesetName = i.key();
        const QMap<QString, uint16_t> tilesetMetatileLabels = i.value();
        if (tilesetMetatileLabels.isEmpty())
            continue;
        outputText += QString("\n// %1\n%2").arg(tilesetName).arg(buildMetatileLabelsText(tilesetMetatileLabels));
    }

    if (unusedMetatileLabels.size() != 0) {
        // Append any defines originally read from the file that aren't associated with any tileset.
        outputText += QString("\n// Other\n");
        outputText += buildMetatileLabelsText(unusedMetatileLabels);
    }

    outputText += QString("\n#endif // %1\n").arg(guardName);

    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_labels);
    ignoreWatchedFileTemporarily(root + "/" + filename);
    return saveTextFile(root + "/" + filename, outputText);
}

bool Project::loadLayoutTilesets(Layout *layout) {
    // Note: Do not replace invalid tileset labels with the default tileset labels here.
    //       Changing the tilesets like this can require us to load tilesets unnecessarily
    //       in order to avoid strange behavior (e.g. tile/metatile usage counts changing).
    if (layout->tileset_primary_label.isEmpty()) {
        logError(QString("Failed to load %1: missing primary tileset label.").arg(layout->name));
        return false;
    }
    if (layout->tileset_secondary_label.isEmpty()) {
        logError(QString("Failed to load %1: missing secondary tileset label.").arg(layout->name));
        return false;
    }

    layout->tileset_primary = getTileset(layout->tileset_primary_label);
    layout->tileset_secondary = getTileset(layout->tileset_secondary_label);
    return layout->tileset_primary && layout->tileset_secondary;
}

Tileset* Project::loadTileset(QString label, Tileset *tileset) {
    auto memberMap = Tileset::getHeaderMemberMap(this->usingAsmTilesets);
    if (this->usingAsmTilesets) {
        // Read asm tileset header. Backwards compatibility
        const QString path = projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm);
        const QStringList values = parser.getLabelValues(parser.parseAsm(path), label);
        if (values.isEmpty()) {
            logError(QString("Failed to find header data in '%1' for tileset '%2'.").arg(path).arg(label));
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
        const QString path = projectConfig.getFilePath(ProjectFilePath::tilesets_headers);
        auto structs = parser.readCStructs(path, label, memberMap);
        if (!structs.contains(label)) {
            logError(QString("Failed to find header data in '%1' for tileset '%2'.").arg(path).arg(label));
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

    if (!loadTilesetAssets(tileset)) {
        // Error should already be logged.
        delete tileset;
        return nullptr;
    }

    tilesetCache.insert(label, tileset);
    return tileset;
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

bool Project::saveAll() {
    bool success = true;
    for (auto map : this->maps) {
        if (!saveMap(map, true)) // Avoid double-saving the layouts
            success = false;
    }
    for (auto layout : this->mapLayouts) {
        if (!saveLayout(layout))
            success = false;
    }
    if (!saveGlobalData()) success = false;
    return success;
}

bool Project::saveMap(Map *map, bool skipLayout) {
    if (!map || !isLoadedMap(map->name())) return true;

    // Create/Modify a few collateral files for brand new maps.
    const QString folderPath = projectConfig.getFilePath(ProjectFilePath::data_map_folders) + map->name();
    const QString fullPath = QString("%1/%2").arg(this->root).arg(folderPath);
    if (!map->isPersistedToFile()) {
        if (!QDir::root().mkpath(fullPath)) {
            logError(QString("Failed to create directory for new map: '%1'").arg(fullPath));
            return false;
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
    QString mapFilepath = map->getJsonFilepath();
    QFile mapFile(mapFilepath);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: %2").arg(mapFilepath).arg(mapFile.errorString()));
        return false;
    }

    OrderedJson::object mapObj;
    // Header values.
    mapObj["id"] = map->constantName();
    mapObj["name"] = map->name();
    mapObj["layout"] = map->layoutId();
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
            connectionObj["map"] = getMapConstant(connection->targetMapName(), connection->targetMapName());
            connectionObj["offset"] = connection->offset();
            connectionObj["direction"] = connection->direction();
            OrderedJson::append(&connectionObj, connection->customData());
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
    OrderedJson::append(&mapObj, map->customAttributes());

    ignoreWatchedFileTemporarily(mapFilepath);

    OrderedJson mapJson(mapObj);
    OrderedJsonDoc jsonDoc(&mapJson);
    jsonDoc.dump(&mapFile);
    mapFile.close();

    // Try to record the MAPSEC name in case this is a new name.
    addNewMapsec(map->header()->location());
    map->setClean();

    if (!skipLayout && !saveLayout(map->layout()))
        return false;
    return true;
}

bool Project::saveLayout(Layout *layout) {
    if (!layout || !isLoadedLayout(layout->id))
        return true;

    if (!layout->save(this->root))
        return false;

    // Update global data structures with current map data.
    if (!this->orderedLayoutIdsMaster.contains(layout->id)) {
        this->orderedLayoutIdsMaster.append(layout->id);
    }

    if (this->mapLayoutsMaster.contains(layout->id)) {
        this->mapLayoutsMaster[layout->id]->copyFrom(layout);
    } else {
        this->mapLayoutsMaster.insert(layout->id, layout->copy());
    }
    return true;
}

bool Project::saveGlobalData() {
    bool success = true;
    if (!saveMapLayouts()) success = false;
    if (!saveMapGroups()) success = false;
    if (!saveRegionMapSections()) success = false;
    if (!saveHealLocations()) success = false;
    if (!saveWildMonData()) success = false;
    if (!saveConfig()) success = false;
    if (!success)
        return false;

    this->hasUnsavedDataChanges = false;
    return true;
}

bool Project::saveConfig() {
    bool success = true;
    if (!projectConfig.save()) success = false;
    if (!userConfig.save()) success = false;
    return success;
}

bool Project::loadTilesetAssets(Tileset* tileset) {
    readTilesetPaths(tileset);
    loadTilesetMetatileLabels(tileset);
    return tileset->load();
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
    int i = 0;
    for (; i < labelList->length(); i++) {
        if (labelList->at(i) > tileset->name) {
            break;
        }
    }
    labelList->insert(i, tileset->name);
    this->tilesetLabelsOrdered.append(tileset->name);

    // Append to tileset specific files.
    // TODO: Ideally we wouldn't save new Tilesets immediately
    QString headersFilepath = this->root + "/";
    QString graphicsFilepath = this->root + "/";
    QString metatilesFilepath = this->root + "/";
    if (this->usingAsmTilesets) {
        headersFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_headers_asm));
        graphicsFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_graphics_asm));
        metatilesFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles_asm));
    } else {
        headersFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_headers));
        graphicsFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_graphics));
        metatilesFilepath.append(projectConfig.getFilePath(ProjectFilePath::tilesets_metatiles));
    }
    ignoreWatchedFilesTemporarily({headersFilepath, graphicsFilepath, metatilesFilepath});
    name.remove(0, prefix.length()); // Strip prefix from name to get base name for use in other symbols.
    tileset->appendToHeaders(headersFilepath, name, this->usingAsmTilesets);
    tileset->appendToGraphics(graphicsFilepath, name, this->usingAsmTilesets);
    tileset->appendToMetatiles(metatilesFilepath, name, this->usingAsmTilesets);

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
    watchFile(metatileLabelsFilename);

    const QSet<QString> regexList = {QString("\\b%1").arg(projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix))};
    const auto defines = parser.readCDefinesByRegex(metatileLabelsFilename, regexList);
    for (auto i = defines.constBegin(); i != defines.constEnd(); i++) {
        QString label = i.key();
        uint32_t metatileId = i.value();
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
    if (!tileset || tileset->name.isEmpty()) return;

    QString metatileLabelPrefix = tileset->getMetatileLabelPrefix();

    // Reverse map for faster lookup by metatile id
    for (auto it = this->metatileLabelsMap[tileset->name].constBegin(); it != this->metatileLabelsMap[tileset->name].constEnd(); it++) {
        QString labelName = it.key();
        tileset->metatileLabels[it.value()] = labelName.replace(metatileLabelPrefix, "");
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

bool Project::saveTextFile(const QString &path, const QString &text) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open '%1' for writing: ").arg(path) + file.errorString());
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

bool Project::appendTextFile(const QString &path, const QString &text) {
    QFile file(path);
    if (!file.open(QIODevice::Append)) {
        logError(QString("Could not open '%1' for appending: ").arg(path) + file.errorString());
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

bool Project::readWildMonData() {
    this->extraEncounterGroups.clear();
    this->wildMonFields.clear();
    this->wildMonData.clear();
    this->wildMonTableName.clear();
    this->encounterGroupLabels.clear();
    this->pokemonMinLevel = 0;
    this->pokemonMaxLevel = 100;
    this->maxEncounterRate = 2880/16;
    this->wildEncountersLoaded = false;
    this->customWildMonData = OrderedJson::object();
    this->customWildMonGroupData = OrderedJson::object();
    if (!userConfig.useEncounterJson) {
        return true;
    }

    // Read max encounter rate. The games multiply the encounter rate value in the map data by 16, so our input limit is the max/16.
    const QString encounterRateFile = projectConfig.getFilePath(ProjectFilePath::wild_encounter);
    const QString maxEncounterRateName = projectConfig.getIdentifier(ProjectIdentifier::define_max_encounter_rate);

    watchFile(encounterRateFile);
    auto defines = parser.readCDefinesByName(encounterRateFile, {maxEncounterRateName});
    if (defines.contains(maxEncounterRateName))
        this->maxEncounterRate = defines.value(maxEncounterRateName)/16;

    // Read min/max level
    const QString levelRangeFile = projectConfig.getFilePath(ProjectFilePath::constants_pokemon);
    const QString minLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_min_level);
    const QString maxLevelName = projectConfig.getIdentifier(ProjectIdentifier::define_max_level);
    watchFile(levelRangeFile);
    defines = parser.readCDefinesByName(levelRangeFile, {minLevelName, maxLevelName});
    if (defines.contains(minLevelName))
        this->pokemonMinLevel = defines.value(minLevelName);
    if (defines.contains(maxLevelName))
        this->pokemonMaxLevel = defines.value(maxLevelName);

    this->pokemonMinLevel = qMin(this->pokemonMinLevel, this->pokemonMaxLevel);
    this->pokemonMaxLevel = qMax(this->pokemonMinLevel, this->pokemonMaxLevel);

    // Read encounter data
    const QString wildMonJsonFilepath = projectConfig.getFilePath(ProjectFilePath::json_wild_encounters);
    watchFile(wildMonJsonFilepath);

    OrderedJson::object wildMonObj;
    QString error;
    if (!parser.tryParseOrderedJsonFile(&wildMonObj, wildMonJsonFilepath, &error)) {
        // Failing to read wild encounters data is not a critical error, the encounter editor will just be disabled
        logWarn(QString("Failed to read wild encounters from '%1': %2").arg(wildMonJsonFilepath).arg(error));
        return true;
    }

    // For each encounter type, count the number of times each encounter rate value occurs.
    // The most common value will be used as the default for new groups.
    QMap<QString, QMap<int, int>> encounterRateFrequencyMaps;

    // Parse "wild_encounter_groups". This is the main object array containing all the data in this file.
    OrderedJson::array mainArray = wildMonObj.take("wild_encounter_groups").array_items();
    for (const OrderedJson &mainArrayJson : mainArray) {
        OrderedJson::object mainArrayObject = mainArrayJson.object_items();

        // We're only interested in wild encounter data that's associated with maps ("for_maps" == true).
        // Any other wild encounter data (e.g. for Battle Pike / Battle Pyramid) will be ignored.
        // We'll record any data that's not for maps in extraEncounterGroups to be outputted when we save.
        if (!mainArrayObject["for_maps"].bool_value()) {
            this->extraEncounterGroups.push_back(mainArrayObject);
            continue;
        } else {
            // Note: We don't call 'take' above, we don't want to strip data from extraEncounterGroups.
            //       We do want to strip it from the main group, because it shouldn't be treated as custom data.
            mainArrayObject.erase("for_maps");
        }

        // If multiple "for_maps" data sets are found they will be collapsed into a single set.
        QString label = mainArrayObject.take("label").string_value();
        if (this->wildMonTableName.isEmpty()) {
            this->wildMonTableName = label;
        } else {
            logWarn(QString("Wild encounters table '%1' will be combined with '%2'. Only one table with \"for_maps\" set to 'true' is expected.")
                                .arg(label)
                                .arg(this->wildMonTableName));
        }

        // Parse the "fields" data. This is like the header for the wild encounters data.
        // Each element describes a type of wild encounter Porymap can expect to find, and we represent this data with an EncounterField.
        // They should contain a name ("type"), the number of encounter slots and the ratio at which they occur ("encounter_rates"),
        // and whether the encounters are divided into groups (like fishing rods).
        OrderedJson::array fieldsArray = mainArrayObject.take("fields").array_items();
        for (const OrderedJson &fieldJson : fieldsArray) {
            OrderedJson::object fieldObject = fieldJson.object_items();

            EncounterField encounterField;
            encounterField.name = fieldObject.take("type").string_value();

            OrderedJson::array encounterRatesArray = fieldObject.take("encounter_rates").array_items();
            for (const auto &val : encounterRatesArray) {
                encounterField.encounterRates.append(val.int_value());
            }

            // Each element of the "groups" array is an object with the group name as the key (e.g. "old_rod")
            // and an array of slot numbers indicating which encounter slots in this encounter type belong to that group.
            OrderedJson::object groups = fieldObject.take("groups").object_items();
            for (auto groupPair : groups) {
                const QString groupName = groupPair.first;
                for (auto slotNum : groupPair.second.array_items()) {
                    encounterField.groups[groupName].append(slotNum.int_value());
                }
            }
            encounterField.customData = fieldObject;

            encounterRateFrequencyMaps.insert(encounterField.name, QMap<int, int>());
            this->wildMonFields.append(encounterField);
        }

        // Parse the "encounters" data. This is the meat of the wild encounters data.
        // Each element is an object that will tell us which map it's associated with,
        // its symbol name (which we will display in the Groups dropdown) and a list of
        // pokémon associated with any of the encounter types described by the data we parsed above.
        OrderedJson::array encountersArray = mainArrayObject.take("encounters").array_items();
        for (const auto &encounterJson : encountersArray) {
            OrderedJson::object encounterObj = encounterJson.object_items();

            WildPokemonHeader header;

            // Check for each possible encounter type.
            for (const EncounterField &monField : this->wildMonFields) {
                const QString field = monField.name;
                if (!encounterObj.contains(field)) {
                    // Encounter type isn't present
                    continue;
                }
                OrderedJson::object encounterFieldObj = encounterObj.take(field).object_items();

                WildMonInfo monInfo;
                monInfo.active = true;

                // Read encounter rate
                monInfo.encounterRate = encounterFieldObj.take("encounter_rate").int_value();
                encounterRateFrequencyMaps[field][monInfo.encounterRate]++;

                // Read wild pokémon list
                OrderedJson::array monsArray = encounterFieldObj.take("mons").array_items();
                for (const auto &monJson : monsArray) {
                    OrderedJson::object monObj = monJson.object_items();

                    WildPokemon newMon;
                    newMon.minLevel = monObj.take("min_level").int_value();
                    newMon.maxLevel = monObj.take("max_level").int_value();
                    newMon.species = monObj.take("species").string_value();
                    newMon.customData = monObj;
                    monInfo.wildPokemon.append(newMon);
                }
                monInfo.customData = encounterFieldObj;

                // If the user supplied too few pokémon for this group then we fill in the rest with default values.
                for (int i = monInfo.wildPokemon.length(); i < monField.encounterRates.length(); i++) {
                    monInfo.wildPokemon.append(WildPokemon());
                }
                header.wildMons[field] = monInfo;
            }
            const QString mapConstant = encounterObj.take("map").string_value();
            const QString baseLabel = encounterObj.take("base_label").string_value();
            header.customData = encounterObj;
            this->wildMonData[mapConstant].insert({baseLabel, header});
            this->encounterGroupLabels.append(baseLabel);
        }
        this->customWildMonGroupData = mainArrayObject;
    }
    this->customWildMonData = wildMonObj;

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
    clearMaps();
    this->mapConstantsToMapNames.clear();
    this->alphabeticalMapNames.clear();
    this->groupNames.clear();
    this->groupNameToMapNames.clear();
    this->customMapGroupsData = QJsonObject();

    const QString filepath = projectConfig.getFilePath(ProjectFilePath::json_map_groups);
    watchFile(filepath);
    QJsonDocument mapGroupsDoc;
    QString error;
    if (!parser.tryParseJsonFile(&mapGroupsDoc, filepath, &error)) {
        logError(QString("Failed to read map groups from '%1': %2").arg(filepath).arg(error));
        return false;
    }

    QJsonObject mapGroupsObj = mapGroupsDoc.object();
    QJsonArray mapGroupOrder = mapGroupsObj.take("group_order").toArray();

    const QString dynamicMapName = getDynamicMapName();
    const QString dynamicMapConstant = getDynamicMapDefineName();

    // Process the map group lists
    for (int groupIndex = 0; groupIndex < mapGroupOrder.size(); groupIndex++) {
        const QString groupName = ParseUtil::jsonToQString(mapGroupOrder.at(groupIndex));
        if (this->groupNames.contains(groupName)) {
            logWarn(QString("Ignoring repeated map group name '%1'.").arg(groupName));
            continue;
        }

        const QJsonArray mapNamesJson = mapGroupsObj.take(groupName).toArray();
        this->groupNames.append(groupName);
        
        // Process the names in this map group
        for (int j = 0; j < mapNamesJson.size(); j++) {
            const QString mapName = ParseUtil::jsonToQString(mapNamesJson.at(j));
            if (mapName.isEmpty()) {
                logWarn(QString("Ignoring empty map %1 in map group '%2'.").arg(j).arg(groupName).arg(mapName));
                continue;
            }
            // We explicitly hide "Dynamic" from the map list, so this entry will be deleted from the file if the user changes the map list order.
            if (mapName == dynamicMapName) {
                logWarn(QString("Ignoring map %1 in map group '%2': Cannot use reserved map name '%3'.").arg(j).arg(groupName).arg(mapName));
                continue;
            }

            // Excepting the disallowed map names above, we want to preserve the user's map list data,
            // so this list should accept all names we find whether they have valid data or not.
            this->groupNameToMapNames[groupName].append(mapName);

            // We log a warning for this, but a repeated name in the map list otherwise functions fine.
            // All repeated entries will refer to the same map data.
            if (this->maps.contains(mapName)) {
                logWarn(QString("Map %1 in map group '%2' has repeated map name '%3'.").arg(j).arg(groupName).arg(mapName));
                continue;
            }

            // Load the map's json file so we can get its ID constant (and two other constants we use for the map list).
            // If we fail to get the ID for any reason, we flag the map as 'errored'. It can still appear in the map list,
            // but we won't be able to translate the map name to a map constant, so the map name can't appear elsewhere.
            QString mapJsonError;
            QJsonDocument mapDoc = readMapJson(mapName, &mapJsonError);
            if (!mapJsonError.isEmpty()) {
                this->erroredMaps.insert(mapName, mapJsonError);
                logWarn(mapJsonError);
                continue;
            }

            // Read and validate the map's ID from its JSON data.
            const QJsonObject mapObj = mapDoc.object();
            const QString mapConstant = ParseUtil::jsonToQString(mapObj["id"]);
            if (mapConstant.isEmpty()) {
                QString message = QString("Map '%1' is invalid: Missing \"id\" value.").arg(mapName);
                this->erroredMaps.insert(mapName, message);
                logWarn(message);
                continue;
            }
            if (mapConstant == dynamicMapConstant) {
                QString message = QString("Map '%1' is invalid: Cannot use reserved name '%2' for \"id\" value.").arg(mapName).arg(mapConstant);
                this->erroredMaps.insert(mapName, message);
                logWarn(message);
                continue;
            }
            auto it = this->mapConstantsToMapNames.constFind(mapConstant);
            if (it != this->mapConstantsToMapNames.constEnd()) {
                QString message =  QString("Map '%1' is invalid: Cannot use the same \"id\" value '%2' as map '%3'.")
                                                .arg(mapName)
                                                .arg(it.key())
                                                .arg(it.value());
                this->erroredMaps.insert(mapName, message);
                logWarn(message);
                continue;
            }

            // Success, create the Map object
            auto map = new Map;
            map->setName(mapName);
            map->setConstantName(mapConstant);

            this->maps.insert(mapName, map);
            this->alphabeticalMapNames.append(mapName);
            this->mapConstantsToMapNames.insert(mapConstant, mapName);

            // Read layout ID for map list
            const QString layoutId = ParseUtil::jsonToQString(mapObj["layout"]);
            map->setLayoutId(layoutId);
            map->setLayout(this->mapLayouts.value(layoutId)); // This may set layout to nullptr. Don't report anything until user tries to load this map.

            // Read MAPSEC name for map list
            map->header()->setLocation(ParseUtil::jsonToQString(mapObj["region_map_section"]));
        }
    }

    // Note: Not successfully reading any maps or map groups is ok. We only require at least 1 map layout.

    // Save special "Dynamic" constant
    this->mapConstantsToMapNames.insert(dynamicMapConstant, dynamicMapName);
    this->alphabeticalMapNames.append(dynamicMapName);
    Util::numericalModeSort(this->alphabeticalMapNames);

    // Chuck the "connections_include_order" field, this is only for matching.
    if (!projectConfig.preserveMatchingOnlyData) {
        mapGroupsObj.remove("connections_include_order");
    }

    // Preserve any remaining fields for when we save.
    this->customMapGroupsData = mapGroupsObj;

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

QString Project::mapNameToMapGroup(const QString &mapName) const {
    for (auto it = this->groupNameToMapNames.constBegin(); it != this->groupNameToMapNames.constEnd(); it++) {
        if (it.value().contains(mapName)) {
            return it.key();
        }
    }
    return QString();
}

QString Project::getMapConstant(const QString &mapName, const QString &defaultValue) const {
    if (mapName == getDynamicMapName()) return getDynamicMapDefineName();

    Map* map = this->maps.value(mapName);
    return map ? map->constantName() : defaultValue;
}

QString Project::getMapLayoutId(const QString &mapName, const QString &defaultValue) const {
    Map* map = this->maps.value(mapName);
    return map ? map->layoutId() : defaultValue;
}

QString Project::getMapLocation(const QString &mapName, const QString &defaultValue) const {
    Map* map = this->maps.value(mapName);
    return map ? map->header()->location() : defaultValue;
}

QString Project::getLayoutName(const QString &layoutId) const {
    Layout* layout = this->mapLayouts.value(layoutId);
    return layout ? layout->name : QString();
}

QStringList Project::getLayoutNames() const {
    QStringList names;
    for (const auto &layoutId : this->alphabeticalLayoutIds) {
        names.append(getLayoutName(layoutId));
    }
    return names;
}

QStringList Project::getMapNamesByGroup() const {
    QStringList names;
    for (const auto &groupName : this->groupNames) {
        for (const auto &groupMapNames : this->groupNameToMapNames.value(groupName)) {
            names.append(groupMapNames);
        }
    }
    return names;
}

bool Project::isUnsavedMap(const QString &mapName) const {
    Map* map = this->maps.value(mapName);
    return map ? map->hasUnsavedChanges() : false;
}

bool Project::isUnsavedLayout(const QString &layoutId) const {
    Layout* layout = this->mapLayouts.value(layoutId);
    return layout ? layout->hasUnsavedChanges() : false;
}

// Determining which map a secret base ID refers to relies on assumptions about its name.
// The default format is for a secret base ID of 'SECRET_BASE_FOO_#' to refer to a map with the constant
// 'MAP_SECRET_BASE_FOO', so we strip the `_#` suffix and add the default map prefix 'MAP_'. If this fails,
// we'll try every combination of that prefix, suffix, and 'SECRET_BASE_FOO'.
QString Project::secretBaseIdToMapName(const QString &secretBaseId) const {
    const QString mapPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
    QString baseIdNoSuffix = secretBaseId.left(secretBaseId.lastIndexOf("_"));

    auto it = this->mapConstantsToMapNames.constFind(mapPrefix + baseIdNoSuffix);
    if (it != this->mapConstantsToMapNames.constEnd()) {
        // Found a result of the format 'SECRET_BASE_FOO_#' -> 'MAP_SECRET_BASE_FOO'.
        return it.value();
    }
    it = this->mapConstantsToMapNames.constFind(baseIdNoSuffix);
    if (it != this->mapConstantsToMapNames.constEnd()) {
        // Found a result of the format 'SECRET_BASE_FOO_#' -> 'SECRET_BASE_FOO'.
        return it.value();
    }
    it = this->mapConstantsToMapNames.constFind(mapPrefix + secretBaseId);
    if (it != this->mapConstantsToMapNames.constEnd()) {
        // Found a result of the format 'SECRET_BASE_FOO_#' -> 'MAP_SECRET_BASE_FOO_#'.
        return it.value();
    }
    it = this->mapConstantsToMapNames.constFind(secretBaseId);
    if (it != this->mapConstantsToMapNames.constEnd()) {
        // 'SECRET_BASE_FOO_#' is already a map constant.
        return it.value();
    }
    return QString();
}

// When we ask the user to provide a new identifier for something (like a map name or MAPSEC id)
// we use this to make sure that it doesn't collide with any known identifiers first.
// Porymap knows of many more identifiers than this, but for simplicity we only check the lists that users can add to via Porymap.
// In general this only matters to Porymap if the identifier will be added to the group it collides with,
// but name collisions are likely undesirable in the project.
bool Project::isIdentifierUnique(const QString &identifier) const {
    if (this->maps.contains(identifier) || this->erroredMaps.contains(identifier))
        return false;
    if (this->mapConstantsToMapNames.contains(identifier))
        return false;
    if (this->groupNames.contains(identifier))
        return false;
    if (this->mapSectionIdNames.contains(identifier))
        return false;
    if (this->tilesetLabelsOrdered.contains(identifier))
        return false;
    if (this->mapLayouts.contains(identifier))
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
    for (const auto &mapName : this->loadedMapNames) {
        for (const auto &event : this->maps.value(mapName)->getEvents()) {
            QString idName = event->getIdName();
            if (!idName.isEmpty() && idName == identifier)
                return false;
        }
    }
    return true;
}

// For some arbitrary string, return true if it's both a valid identifier name and not one that's already in-use.
bool Project::isValidNewIdentifier(const QString &identifier) const {
    static const IdentifierValidator validator;
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
    this->newMapSettings.group = this->groupNames.value(0);
    this->newMapSettings.canFlyTo = false;

    this->newMapSettings.layout.folderName = this->newMapSettings.name;
    this->newMapSettings.layout.name = QString();
    this->newMapSettings.layout.id = Layout::layoutConstantFromName(this->newMapSettings.name);
    this->newMapSettings.layout.width = this->defaultMapSize.width();
    this->newMapSettings.layout.height = this->defaultMapSize.height();
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
    this->newLayoutSettings.width = this->defaultMapSize.width();
    this->newLayoutSettings.height = this->defaultMapSize.height();
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
        QString text = parser.loadTextFile(asm_filename);
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

    Util::numericalModeSort(this->primaryTilesetLabels);
    Util::numericalModeSort(this->secondaryTilesetLabels);

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
    const QString mapOffsetWidthName = projectConfig.getIdentifier(ProjectIdentifier::define_map_offset_width);
    const QString mapOffsetHeightName = projectConfig.getIdentifier(ProjectIdentifier::define_map_offset_height);

    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_fieldmap);
    watchFile(filename);
    const auto defines = parser.readCDefinesByName(filename, { numTilesPrimaryName,
                                                               numTilesTotalName,
                                                               numMetatilesPrimaryName,
                                                               numPalsPrimaryName,
                                                               numPalsTotalName,
                                                               maxMapSizeName,
                                                               numTilesPerMetatileName,
                                                               mapOffsetWidthName,
                                                               mapOffsetHeightName,
    });

    auto loadDefine = [defines](const QString name, int * dest, int min, int max) {
        auto it = defines.find(name);
        if (it != defines.end()) {
            *dest = it.value();
            if (*dest < min) {
                logWarn(QString("Value for '%1' (%2) is below the minimum (%3). Defaulting to minimum.").arg(name).arg(*dest).arg(min));
                *dest = min;
            } else if (*dest > max) {
                logWarn(QString("Value for '%1' (%2) is above the maximum (%3). Defaulting to maximum.").arg(name).arg(*dest).arg(max));
                *dest = max;
            }
        } else {
            logWarn(QString("Value for '%1' not found. Using default (%2) instead.").arg(name).arg(*dest));
        }
    };
    loadDefine(numPalsTotalName,        &Project::num_pals_total, 2, INT_MAX); // In reality the max would be 16, but as far as Porymap is concerned it doesn't matter.
    loadDefine(numTilesTotalName,       &Project::num_tiles_total, 2, 1024); // 1024 is fixed because we store tile IDs in a 10-bit field.
    loadDefine(numPalsPrimaryName,      &Project::num_pals_primary, 1, Project::num_pals_total - 1);
    loadDefine(numTilesPrimaryName,     &Project::num_tiles_primary, 1, Project::num_tiles_total - 1);

    // This maximum is overly generous, because until we parse the appropriate masks from the project
    // we don't actually know what the maximum number of metatiles is.
    loadDefine(numMetatilesPrimaryName, &Project::num_metatiles_primary, 1, 0xFFFF - 1);

    int w = 15, h = 14; // Default values of MAP_OFFSET_W, MAP_OFFSET_H
    loadDefine(mapOffsetWidthName, &w, 0, INT_MAX);
    loadDefine(mapOffsetHeightName, &h, 0, INT_MAX);
    this->mapSizeAddition = QSize(w, h);

    this->maxMapDataSize = 10240; // Default value of MAX_MAP_DATA_SIZE
    this->defaultMapSize = projectConfig.defaultMapSize;
    auto it = defines.find(maxMapSizeName);
    if (it != defines.end()) {
        int min = getMapDataSize(1, 1);
        if (it.value() >= min) {
            this->maxMapDataSize = it.value();
            if (getMapDataSize(this->defaultMapSize.width(), this->defaultMapSize.height()) > this->maxMapDataSize) {
                // The specified map size is too small to use the default map dimensions.
                // Calculate the largest square map size that we can use instead.
                int dimension = qFloor((qSqrt(4 * this->maxMapDataSize + 1) - (w + h)) / 2);
                logWarn(QString("Value for '%1' (%2) is too small to support the default %3x%4 map. Default changed to %5x%5.")
                    .arg(maxMapSizeName)
                    .arg(it.value())
                    .arg(this->defaultMapSize.width())
                    .arg(this->defaultMapSize.height())
                    .arg(dimension));
                this->defaultMapSize = QSize(dimension, dimension);
            }
        } else {
            logWarn(QString("Value for '%1' (%2) is too small to support a 1x1 map. Must be at least %3. Using default (%4) instead.")
                    .arg(maxMapSizeName)
                    .arg(it.value())
                    .arg(min)
                    .arg(this->maxMapDataSize));
        }
    }
    else {
        logWarn(QString("Value for '%1' not found. Using default (%2) instead.")
                .arg(maxMapSizeName)
                .arg(this->maxMapDataSize));
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
    this->encounterTypeToName.clear();
    this->terrainTypeToName.clear();

    const QString metatileIdMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_metatile);
    const QString collisionMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_collision);
    const QString elevationMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_elevation);
    const QString behaviorMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_behavior);
    const QString layerTypeMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_layer);

    const QString globalFieldmap = projectConfig.getFilePath(ProjectFilePath::global_fieldmap); // File already being watched
    const auto defines = parser.readCDefinesByName(globalFieldmap, { metatileIdMaskName,
                                                                     collisionMaskName,
                                                                     elevationMaskName,
                                                                     behaviorMaskName,
                                                                     layerTypeMaskName,
    });

    // These mask values are accessible via the settings editor for users who don't have these defines.
    // If users do have the defines we disable them in the settings editor and direct them to their project files.
    // Record the names we read so we know later which settings to disable.
    const QStringList defineNames = defines.keys();
    for (auto name : defineNames)
        this->disabledSettingsNames.insert(name);

    // Read Block masks
    auto readBlockMask = [defines](const QString name, uint16_t *value) {
        auto it = defines.constFind(name);
        if (it == defines.constEnd())
            return false;
        *value = static_cast<uint16_t>(it.value());
        if (*value != it.value()){
            logWarn(QString("Value for %1 truncated from '%2' to '%3'")
                .arg(name)
                .arg(Util::toHexString(it.value()))
                .arg(Util::toHexString(*value)));
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
        watchFile(srcFieldmap);

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

    // Read #defines for encounter and terrain types to populate in the Tileset Editor dropdowns (if necessary)
    QString error;
    if (projectConfig.metatileEncounterTypeMask) {
        const auto defines = parser.readCDefinesByRegex(globalFieldmap, {projectConfig.getIdentifier(ProjectIdentifier::regex_encounter_types)}, &error);
        if (!error.isEmpty()) {
            logWarn(QString("Failed to read encounter type constants from '%1': %2").arg(globalFieldmap).arg(error));
            error = QString();
        } else {
            for (auto i = defines.constBegin(); i != defines.constEnd(); i++) {
                this->encounterTypeToName.insert(static_cast<uint32_t>(i.value()), i.key());
            }
        }
    }
    if (projectConfig.metatileTerrainTypeMask) {
        const auto defines = parser.readCDefinesByRegex(globalFieldmap, {projectConfig.getIdentifier(ProjectIdentifier::regex_terrain_types)}, &error);
        if (!error.isEmpty()) {
            logWarn(QString("Failed to read terrain type constants from '%1': %2").arg(globalFieldmap).arg(error));
            error = QString();
        } else {
            for (auto i = defines.constBegin(); i != defines.constEnd(); i++) {
                this->terrainTypeToName.insert(static_cast<uint32_t>(i.value()), i.key());
            }
        }
    }

    return true;
}

bool Project::readRegionMapSections() {
    this->locationData.clear();
    this->mapSectionIdNames.clear();
    this->mapSectionIdNamesSaveOrder.clear();
    this->customMapSectionsData = QJsonObject();

    const QString defaultName = getEmptyMapsecName();
    const QString requiredPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix);

    QJsonDocument doc;
    const QString filepath = projectConfig.getFilePath(ProjectFilePath::json_region_map_entries);
    QString error;
    if (!parser.tryParseJsonFile(&doc, filepath, &error)) {
        logError(QString("Failed to read region map sections from '%1': %2").arg(filepath).arg(error));
        return false;
    }
    watchFile(filepath);

    QJsonObject mapSectionsGlobalObj = doc.object();
    QJsonArray mapSections = mapSectionsGlobalObj.take("map_sections").toArray();
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
                logWarn(QString("Ignoring data for map section %1 in '%2'. Missing required field \"%3\"").arg(i).arg(filepath).arg(idField));
                continue;
            }
        }
        const QString idName = ParseUtil::jsonToQString(mapSectionObj.take(idField));
        if (!idName.startsWith(requiredPrefix)) {
            logWarn(QString("Ignoring data for map section '%1' in '%2'. IDs must start with the prefix '%3'").arg(idName).arg(filepath).arg(requiredPrefix));
            continue;
        }

        this->mapSectionIdNames.append(idName);
        this->mapSectionIdNamesSaveOrder.append(idName);

        LocationData location;
        if (mapSectionObj.contains("name")) {
            location.displayName = ParseUtil::jsonToQString(mapSectionObj.take("name"));
        }

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
        if (hasRegionMapData) {
            location.map.x = ParseUtil::jsonToInt(mapSectionObj.take("x"));
            location.map.y = ParseUtil::jsonToInt(mapSectionObj.take("y"));
            location.map.width = ParseUtil::jsonToInt(mapSectionObj.take("width"));
            location.map.height = ParseUtil::jsonToInt(mapSectionObj.take("height"));
            location.map.valid = true;
        }

        // Chuck the "name_clone" field, this is only for matching.
        if (!projectConfig.preserveMatchingOnlyData) {
            mapSectionObj.remove("name_clone");
        }

        // Preserve any remaining fields for when we save.
        location.custom = mapSectionObj;

        this->locationData.insert(idName, location);
    }
    this->customMapSectionsData = mapSectionsGlobalObj;

    // Make sure the default name is present in the list.
    if (!this->mapSectionIdNames.contains(defaultName)) {
        this->mapSectionIdNames.append(defaultName);
    }
    Util::numericalModeSort(this->mapSectionIdNames);

    return true;
}

void Project::setRegionMapEntries(const QHash<QString, MapSectionEntry> &entries) {
    for (auto it = entries.constBegin(); it != entries.constEnd(); it++) {
        this->locationData[it.key()].map = it.value();
    }
}

QHash<QString, MapSectionEntry> Project::getRegionMapEntries() const {
    QHash<QString, MapSectionEntry> entries;
    for (auto it = this->locationData.constBegin(); it != this->locationData.constEnd(); it++) {
        entries[it.key()] = it.value().map;
    }
    return entries;
}

QString Project::getEmptyMapsecName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_map_section_empty);
}

QString Project::getMapGroupPrefix() {
    // We could expose this to users, but it's never enforced so it probably won't affect anyone.
    return QStringLiteral("gMapGroup_");
}

bool Project::addNewMapsec(const QString &idName, const QString &displayName) {
    if (this->mapSectionIdNames.contains(idName)) {
        // Already added
        return false;
    }

    IdentifierValidator validator(projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix));
    if (!validator.isValid(idName)) {
        logWarn(QString("Cannot add new MAPSEC with invalid name '%1'").arg(idName));
        return false;
    }

    if (!this->mapSectionIdNamesSaveOrder.isEmpty() && this->mapSectionIdNamesSaveOrder.last() == getEmptyMapsecName()) {
        // If the default map section name (MAPSEC_NONE) is last in the list we'll keep it last in the list.
        this->mapSectionIdNamesSaveOrder.insert(this->mapSectionIdNames.length() - 1, idName);
    } else {
        this->mapSectionIdNamesSaveOrder.append(idName);
    }

    this->mapSectionIdNames.append(idName);
    Util::numericalModeSort(this->mapSectionIdNames);

    this->hasUnsavedDataChanges = true;

    emit mapSectionAdded(idName);
    emit mapSectionIdNamesChanged(this->mapSectionIdNames);
    if (!displayName.isEmpty()) setMapsecDisplayName(idName, displayName);
    return true;
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
    if (getMapsecDisplayName(idName) == displayName)
        return;
    this->locationData[idName].displayName = displayName;
    this->hasUnsavedDataChanges = true;
    emit mapSectionDisplayNameChanged(idName, displayName);
}

void Project::clearHealLocations() {
    for (auto &events : this->healLocations) {
        qDeleteAll(events);
    }
    this->healLocations.clear();
    this->healLocationSaveOrder.clear();
    this->customHealLocationsData = QJsonObject();
}

bool Project::readHealLocations() {
    clearHealLocations();

    QJsonDocument doc;
    const QString filepath = projectConfig.getFilePath(ProjectFilePath::json_heal_locations);
    QString error;
    if (!parser.tryParseJsonFile(&doc, filepath, &error)) {
        logError(QString("Failed to read heal locations from '%1': %2").arg(filepath).arg(error));
        return false;
    }
    watchFile(filepath);

    QJsonObject healLocationsObj = doc.object();
    QJsonArray healLocations = healLocationsObj.take("heal_locations").toArray();
    for (int i = 0; i < healLocations.size(); i++) {
        QJsonObject healLocationObj = healLocations.at(i).toObject();
        static const QString mapField = QStringLiteral("map");
        if (!healLocationObj.contains(mapField)) {
            logWarn(QString("Ignoring data for heal location %1 in '%2'. Missing required field \"%3\"").arg(i).arg(filepath).arg(mapField));
            continue;
        }

        auto event = new HealLocationEvent();
        event->loadFromJson(healLocationObj, this);
        this->healLocations[event->getHostMapName()].append(event);
        this->healLocationSaveOrder.append(event->getIdName());
    }
    this->customHealLocationsData = healLocationsObj;
    return true;
}

bool Project::readItemNames() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_items);
    watchFile(filename);
    QString error;
    this->itemNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_items)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read item constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readFlagNames() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_flags);
    watchFile(filename);
    QString error;
    this->flagNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_flags)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read flag constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readVarNames() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_vars);
    watchFile(filename);
    QString error;
    this->varNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_vars)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read var constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readMovementTypes() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_event_movement);
    watchFile(filename);
    QString error;
    this->movementTypes = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_movement_types)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read movement type constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readInitialFacingDirections() {
    QString filename = projectConfig.getFilePath(ProjectFilePath::initial_facing_table);
    watchFile(filename);
    QString error;
    this->facingDirections = parser.readNamedIndexCArray(filename, projectConfig.getIdentifier(ProjectIdentifier::symbol_facing_directions), &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read initial movement type facing directions from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readMapTypes() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    // File already being watched
    QString error;
    this->mapTypes = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_map_types)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read map type constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readMapBattleScenes() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_map_types);
    // File already being watched
    QString error;
    this->mapBattleScenes = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_battle_scenes)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read map battle scene constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readWeatherNames() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    watchFile(filename);
    QString error;
    this->weatherNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_weather)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read weather constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readCoordEventWeatherNames() {
    if (!projectConfig.eventWeatherTriggerEnabled)
        return true;

    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_weather);
    watchFile(filename);
    QString error;
    this->coordEventWeatherNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_coord_event_weather)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read coord event weather constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readSecretBaseIds() {
    if (!projectConfig.eventSecretBaseEnabled)
        return true;

    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_secret_bases);
    watchFile(filename);
    QString error;
    this->secretBaseIds = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_secret_bases)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read secret base id constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readBgEventFacingDirections() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_event_bg);
    watchFile(filename);
    QString error;
    this->bgEventFacingDirections = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_sign_facing_directions)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read bg event facing direction constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readTrainerTypes() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_trainer_types);
    watchFile(filename);
    QString error;
    this->trainerTypes = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_trainer_types)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read trainer type constants from '%1': %2").arg(filename).arg(error));
    return true;
}

bool Project::readMetatileBehaviors() {
    this->metatileBehaviorMap.clear();
    this->metatileBehaviorMapInverse.clear();

    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_metatile_behaviors);
    watchFile(filename);
    QString error;
    const auto defines = parser.readCDefinesByRegex(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_behaviors)}, &error);
    if (defines.isEmpty() && projectConfig.metatileBehaviorMask) {
        // Not having any metatile behavior names is ok (their values will be displayed instead)
        // but if the user's metatiles can have nonzero values then warn them, as they likely want names.
        QString warning = QString("Failed to read metatile behaviors from '%1'").arg(filename);
        if (!error.isEmpty()) warning += QString(": %1").arg(error);
        logWarn(warning);
    }

    for (auto i = defines.cbegin(), end = defines.cend(); i != end; i++) {
        uint32_t value = static_cast<uint32_t>(i.value());
        this->metatileBehaviorMap.insert(i.key(), value);
        this->metatileBehaviorMapInverse.insert(value, i.key());
    }

    return true;
}

bool Project::readSongNames() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_songs);
    watchFile(filename);
    QString error;
    this->songNames = parser.readCDefineNames(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_music)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read song names from '%1': %2").arg(filename).arg(error));

    // Song names don't have a very useful order (esp. if we include SE_* values), so sort them alphabetically.
    // The default song should be the first in the list, not the first alphabetically, so save that before sorting.
    this->defaultSong = this->songNames.value(0, "0");
    Util::numericalModeSort(this->songNames);
    return true;
}

bool Project::readObjEventGfxConstants() {
    QString filename = projectConfig.getFilePath(ProjectFilePath::constants_obj_events);
    watchFile(filename);
    QString error;
    const auto defines = parser.readCDefinesByRegex(filename, {projectConfig.getIdentifier(ProjectIdentifier::regex_obj_event_gfx)}, &error);
    if (!error.isEmpty())
        logWarn(QString("Failed to read object event graphics constants from '%1': %2").arg(filename).arg(error));

    this->gfxDefines.clear();
    for (auto it = defines.constBegin(); it != defines.constEnd(); it++)
        this->gfxDefines.insert(it.key(), it.value());

    return true;
}

bool Project::readMiscellaneousConstants() {
    const QString filename = projectConfig.getFilePath(ProjectFilePath::constants_global);
    const QString maxObjectEventsName = projectConfig.getIdentifier(ProjectIdentifier::define_obj_event_count);
    watchFile(filename);
    const auto defines = parser.readCDefinesByName(filename, {maxObjectEventsName});

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

bool Project::readGlobalConstants() {
    this->parser.resetCDefines();
    for (const auto &path : projectConfig.globalConstantsFilepaths) {
        QString error;
        this->parser.loadGlobalCDefinesFromFile(path, &error);
        if (!error.isEmpty()) {
            logWarn(QString("Failed to read global constants file '%1': %2").arg(path).arg(error));
        }
    }
    this->parser.loadGlobalCDefines(projectConfig.globalConstants);
    return true;
}

bool Project::readEventScriptLabels() {
    this->globalScriptLabels.clear();

    if (porymapConfig.loadAllEventScripts) {
        for (const auto &filePath : getEventScriptsFilepaths())
            this->globalScriptLabels << ParseUtil::getGlobalScriptLabels(filePath);

       this->globalScriptLabels.sort(Qt::CaseInsensitive);
       this->globalScriptLabels.removeDuplicates();
    }
    emit eventScriptLabelsRead();
    return true;
}

void Project::insertGlobalScriptLabels(QStringList &scriptLabels) const {
    if (this->globalScriptLabels.isEmpty())
        return;
    scriptLabels.append(this->globalScriptLabels);
    scriptLabels.sort();
    scriptLabels.removeDuplicates();
}

QString Project::fixPalettePath(const QString &path) const {
    return Util::replaceExtension(path, QStringLiteral("pal"));
}

QString Project::fixGraphicPath(const QString &path) const {
    return Util::replaceExtension(path, QStringLiteral("png"));
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

QStringList Project::getEventScriptsFilepaths() const {
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
    watchFiles({pointersFilepath, gfxInfoFilepath, picTablesFilepath, gfxFilepath});

    // Read the table mapping OBJ_EVENT_GFX constants to the names of pointers to data about their graphics.
    const QString pointersName = projectConfig.getIdentifier(ProjectIdentifier::symbol_obj_event_gfx_pointers);
    const QMap<QString, QString> pointerMap = parser.readNamedIndexCArray(pointersFilepath, pointersName);

    // The positions of each of the required members for the gfx info struct.
    // For backwards compatibility if the struct doesn't use initializers.
    static const QHash<int, QString> gfxInfoMemberMap = {
        {4, "width"},
        {5, "height"},
        {8, "inanimate"},
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
        QString info_label = pointerMap.value(gfxName);
        info_label.replace("&", "");
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
        // The true dimensions are buried in the subsprite data, so we try to infer the dimensions from the name of the 'subspriteTables' symbol.
        // If we are unable to do this, we can read the dimensions from the width and height fields.
        // This is much more straightforward, but the numbers are not necessarily accurate (one vanilla event sprite,
        // the Town Map in FRLG, has width/height values that differ from its true dimensions).
        static const QRegularExpression re_dimensions("\\S+_(\\d+)x(\\d+)");
        const QRegularExpressionMatch dimensionsMatch = re_dimensions.match(gfxInfoAttributes.value("subspriteTables"));
        if (dimensionsMatch.hasMatch()) {
            gfx->spriteWidth = dimensionsMatch.captured(1).toInt(nullptr, 0);
            gfx->spriteHeight = dimensionsMatch.captured(2).toInt(nullptr, 0);
        } else if (gfxInfoAttributes.contains("width") && gfxInfoAttributes.contains("height")) {
            gfx->spriteWidth = gfxInfoAttributes.value("width").toInt(nullptr, 0);
            gfx->spriteHeight = gfxInfoAttributes.value("height").toInt(nullptr, 0);
        }
        // If we fail to get sprite dimensions then they should remain -1, and the sprite will use the full spritesheet as its image.
        if (gfx->spriteWidth <= 0 || gfx->spriteHeight <= 0) {
            gfx->spriteWidth = -1;
            gfx->spriteHeight = -1;
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
    QPixmap pixmap;
    const QString cacheKey = QString("EVENT#%1#%2#%3").arg(gfxName).arg(frame).arg(hFlip ? "1" : "0");
    if (QPixmapCache::find(cacheKey, &pixmap)) {
        return pixmap;
    }

    EventGraphics* gfx = this->eventGraphicsMap.value(gfxName, nullptr);
    if (!gfx) {
        // Invalid gfx constant. If this is a number, try to use that instead.
        bool ok;
        int gfxNum = ParseUtil::gameStringToInt(gfxName, &ok);
        if (ok) gfx = this->eventGraphicsMap.value(this->gfxDefines.key(gfxNum, "NULL"), nullptr);
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
    if (!gfx || gfx->spritesheet.width() == 0 || gfx->spritesheet.height() == 0) {
        // Either we didn't recognize the gfxName, or we were unable to load the sprite's image.
        return QPixmap();
    }

    QImage img;
    if (gfx->inanimate) {
        img = gfx->spritesheet.copy(0, 0, gfx->spriteWidth, gfx->spriteHeight);
    } else {
        int x = frame * gfx->spriteWidth;
        int y = ((frame * gfx->spriteWidth) / gfx->spritesheet.width()) * gfx->spriteHeight;

        img = gfx->spritesheet.copy(x % gfx->spritesheet.width(),
                                    y % gfx->spritesheet.height(),
                                    gfx->spriteWidth,
                                    gfx->spriteHeight);
        if (hFlip) {
            img = img.transformed(QTransform().scale(-1, 1));
        }
    }
    // Set first palette color fully transparent.
    img.setColor(0, qRgba(0, 0, 0, 0));

    pixmap = QPixmap::fromImage(img);
    QPixmapCache::insert(cacheKey, pixmap);
    return pixmap;
}

QPixmap Project::getEventPixmap(Event::Group group) {
    if (group == Event::Group::None)
        return QPixmap();

    QPixmap pixmap;
    const QString cacheKey = QString("EVENT#%1").arg(Event::groupToString(group));
    if (QPixmapCache::find(cacheKey, &pixmap)) {
        return pixmap;
    }

    const int defaultWidth = 16;
    const int defaultHeight = 16;
    static const QPixmap defaultIcons = QPixmap(":/images/Entities_16x16.png");
    QPixmap defaultIcon = QPixmap(defaultIcons.copy(static_cast<int>(group) * defaultWidth, 0, defaultWidth, defaultHeight));

    // Custom event icons may be provided by the user.
    QString customIconPath = projectConfig.getEventIconPath(group);
    if (customIconPath.isEmpty()) {
        // No custom icon specified, use the default icon.
        pixmap = defaultIcon;
    } else {
        // Try to load custom icon
        QString validPath = Project::getExistingFilepath(customIconPath);
        if (!validPath.isEmpty()) customIconPath = validPath; // Otherwise allow it to fail with the original path
        pixmap = QPixmap(customIconPath);
        if (pixmap.isNull()) {
            pixmap = defaultIcon;
            logWarn(QString("Failed to load custom event icon '%1', using default icon.").arg(customIconPath));
        }
    }
    QPixmapCache::insert(cacheKey, pixmap);
    return pixmap;
}

bool Project::readSpeciesIconPaths() {
    this->speciesToIconPath.clear();
    this->speciesNames.clear();

    // Read map of species constants to icon names
    const QString srcfilename = projectConfig.getFilePath(ProjectFilePath::pokemon_icon_table);
    watchFile(srcfilename);
    const QString tableName = projectConfig.getIdentifier(ProjectIdentifier::symbol_pokemon_icon_table);
    const QMap<QString, QString> monIconNames = parser.readNamedIndexCArray(srcfilename, tableName);

    // Read species constants. If this fails we can get them from the icon table (but we shouldn't rely on it).
    const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix);
    const QString constantsFilename = projectConfig.getFilePath(ProjectFilePath::constants_species);
    watchFile(constantsFilename);
    this->speciesNames = parser.readCDefineNames(constantsFilename, {QString("\\b%1").arg(speciesPrefix)});
    if (this->speciesNames.isEmpty()) {
        this->speciesNames = monIconNames.keys();
    }
    this->speciesNames.sort();

    // If we successfully found the species icon table we can use this data to get the filepath for each species icon.
    // For any species not in the table, or if we failed to find the table at all, we will have to predict where the icon file is.
    // That can require checking a lot of files (especially for projects with many species), so to save time on startup we only
    // do this on request in Project::getDefaultSpeciesIconPath.
    if (!monIconNames.isEmpty()) {
        const QString iconGraphicsFile = projectConfig.getFilePath(ProjectFilePath::data_pokemon_gfx);
        watchFile(iconGraphicsFile);
        QMap<QString, QString> iconNameToFilepath = parser.readCIncbinMulti(iconGraphicsFile);
        
        for (auto i = monIconNames.constBegin(); i != monIconNames.constEnd(); i++) {
            QString path;
            QString species = i.key();
            QString iconName = i.value();
            if (iconNameToFilepath.contains(iconName)) {
                path = fixGraphicPath(iconNameToFilepath.value(iconName));
            } else {
                // We have an icon name for this species, but we haven't found its filepath.
                // Try to find the icon file using the full icon name, and the icon name if we assume it has a prefix.
                // Ex: For 'gMonIcon_QuestionMark' search for files by permuting through directories using 'question_mark' and 'g_mon_icon_question_mark.
                static const QRegularExpression re_caseChange("([a-z])([A-Z0-9])");
                QStringList dirNames;
                if (iconName.contains("_")) {
                    QString iconNameNoPrefix = iconName.mid(iconName.indexOf("_") + 1);
                    dirNames.append(iconNameNoPrefix.replace(re_caseChange, "\\1_\\2").toLower());
                }
                QString iconNameWithPrefix = iconName; // Leave iconName unchanged by .replace
                dirNames.append(iconNameWithPrefix.replace(re_caseChange, "\\1_\\2").toLower());
                path = iconNameToFilepath[iconName] = findSpeciesIconPath(dirNames);
            }
            if (!path.isEmpty()) {
                this->speciesToIconPath.insert(species, QString("%1/%2").arg(this->root).arg(path));
            }
        }
    }
    return true;
}

QString Project::getDefaultSpeciesIconPath(const QString &species) {
    if (this->speciesToIconPath.contains(species)) {
        // We already know the icon path for this species (either because we read it from the project, or we found it already).
        return this->speciesToIconPath.value(species);
    }
    if (!this->speciesNames.contains(species)) {
        // Don't bother searching for a path if we don't recognize the species name.
        return QString();
    }

    // Ex: For 'SPECIES_FOO_BAR_BAZ' search for files by permuting through directories using 'foo_bar_baz'.
    const QString speciesPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix);
    const QString path = findSpeciesIconPath({species.mid(speciesPrefix.length()).toLower()});
    this->speciesToIconPath.insert(species, path);

    // We failed to find a default icon path, this species will use a placeholder icon.
    // If the user has no custom icon path for this species, tell them they can provide one.
    if (path.isEmpty() && projectConfig.getPokemonIconPath(species).isEmpty()) {
        logWarn(QString("Failed to find Pokémon icon for '%1'. The filepath can be specified under 'Options->Project Settings'").arg(species));
    }
    return path;
}

// The name permuting in here is overkill, but it's making up for some of the fragility in the way we find pokémon icon paths.
// For pokeemerald-expansion in particular this function is solely responsible for finding pokémon icons, because they have no icon table.
QString Project::findSpeciesIconPath(const QStringList &names) const {
    QStringList possibleDirNames = names;

    // Permute paths with underscores.
    // Ex: For a base name of 'foo_bar_baz', try 'foo_bar/baz', 'foo/bar_baz', 'foobarbaz', 'foo_bar', and 'foo'.
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

    const QString basePath = QString("%1/%2").arg(this->root).arg(projectConfig.getFilePath(ProjectFilePath::pokemon_gfx));
    for (const auto &dir : possibleDirNames) {
        if (dir.isEmpty()) continue;

        const QString path = QString("%1%2/icon.png").arg(basePath).arg(dir);
        if (QFile::exists(path))
            return path;
    }
    return QString();
}

QPixmap Project::getSpeciesIcon(const QString &species) {
    QPixmap pixmap;
    if (!QPixmapCache::find(species, &pixmap)) {
        // Prefer path from config. If not present, use the path parsed from project files
        QString path = Project::getExistingFilepath(projectConfig.getPokemonIconPath(species));
        if (path.isEmpty()) {
            path = getDefaultSpeciesIconPath(species);
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

int Project::getMapDataSize(int width, int height) const {
    return (width + this->mapSizeAddition.width())
         * (height + this->mapSizeAddition.height());
}

int Project::getMaxMapWidth() const {
    return (getMaxMapDataSize() / (1 + this->mapSizeAddition.height())) - this->mapSizeAddition.width();
}

int Project::getMaxMapHeight() const {
    return (getMaxMapDataSize() / (1 + this->mapSizeAddition.width())) - this->mapSizeAddition.height();
}

bool Project::mapDimensionsValid(int width, int height) const {
    return getMapDataSize(width, height) <= getMaxMapDataSize();
}

// Object events have their own limit specified by ProjectIdentifier::define_obj_event_count.
// The default value for this is 64. All events (object events included) are also limited by
// the data types of the event counters in the project. This would normally be u8, so the limit is 255.
// We let the users tell us this limit in case they change these data types.
int Project::getMaxEvents(Event::Group group) const {
    if (group == Event::Group::Object)
        return qMin(this->maxObjectEvents, projectConfig.maxEventsPerGroup);
    return projectConfig.maxEventsPerGroup;
}

QString Project::getEmptyMapDefineName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_empty);
}

QString Project::getDynamicMapDefineName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_map_dynamic);
}

QString Project::getDynamicMapName() {
    return projectConfig.getIdentifier(ProjectIdentifier::symbol_dynamic_map_name);
}

QString Project::getEmptySpeciesName() {
    return projectConfig.getIdentifier(ProjectIdentifier::define_species_prefix) + projectConfig.getIdentifier(ProjectIdentifier::define_species_empty);
}

// Get the distance in metatiles (rounded up) that the player is able to see in each direction in-game.
// For the default view distance (i.e. assuming the player is centered in a 240x160 pixel GBA screen) this is 7x5 metatiles.
QMargins Project::getMetatileViewDistance() {
    QMargins viewDistance = projectConfig.playerViewDistance;
    viewDistance.setTop(qCeil(viewDistance.top() / 16.0));
    viewDistance.setBottom(qCeil(viewDistance.bottom() / 16.0));
    viewDistance.setLeft(qCeil(viewDistance.left() / 16.0));
    viewDistance.setRight(qCeil(viewDistance.right() / 16.0));
    return viewDistance;
}

// If the provided filepath is an absolute path to an existing file, return filepath.
// If not, and the provided filepath is a relative path from the project dir to an existing file, return the relative path.
// Otherwise return empty string.
QString Project::getExistingFilepath(QString filepath) {
    if (filepath.isEmpty() || QFile::exists(filepath))
        return filepath;

    filepath = QDir::cleanPath(projectConfig.projectDir() + QDir::separator() + filepath);
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
    projectConfig.collisionSheetSize.setHeight(qMin(qMax(projectConfig.collisionSheetSize.height(), 1), Block::getMaxElevation() + 1));
    projectConfig.collisionSheetSize.setWidth(qMin(qMax(projectConfig.collisionSheetSize.width(), 1), Block::getMaxCollision() + 1));
}

bool Project::hasUnsavedChanges() {
    if (this->hasUnsavedDataChanges)
        return true;

    // Check layouts for unsaved changes
    for (const auto &layout : this->mapLayouts) {
        if (layout->hasUnsavedChanges())
            return true;
    }

    // Check maps for unsaved changes
    for (const auto &map : this->maps) {
        if (map->hasUnsavedChanges())
            return true;
    }
    return false;
}
