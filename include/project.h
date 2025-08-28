#pragma once
#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"
#include "wildmoninfo.h"
#include "parseutil.h"
#include "orderedjson.h"
#include "regionmap.h"

#include <QStringList>
#include <QList>
#include <QVector>
#include <QPair>
#include <QStandardItem>
#include <QVariant>
#include <QFileSystemWatcher>

class Project : public QObject
{
    Q_OBJECT
public:
    Project(QObject *parent = nullptr);
    ~Project();

    Project(const Project &) = delete;
    Project & operator = (const Project &) = delete;

    inline QWidget *parentWidget() const { return static_cast<QWidget *>(parent()); }

public:
    QString root;
    QStringList groupNames;
    QMap<QString, QStringList> groupNameToMapNames;
    QStringList healLocationSaveOrder;
    QMap<QString, QList<HealLocationEvent*>> healLocations;
    QMap<QString, QString> mapConstantsToMapNames;
    QMap<QString, int> gfxDefines;
    QString defaultSong;
    QStringList songNames;
    QStringList itemNames;
    QStringList flagNames;
    QStringList varNames;
    QStringList speciesNames;
    QStringList movementTypes;
    QStringList mapTypes;
    QStringList mapBattleScenes;
    QStringList weatherNames;
    QStringList coordEventWeatherNames;
    QStringList secretBaseIds;
    QStringList bgEventFacingDirections;
    QStringList trainerTypes;
    QStringList globalScriptLabels;
    QMap<uint32_t, QString> encounterTypeToName;
    QMap<uint32_t, QString> terrainTypeToName;
    QMap<QString, QMap<QString, uint16_t>> metatileLabelsMap;
    QMap<QString, uint16_t> unusedMetatileLabels;
    QMap<QString, uint32_t> metatileBehaviorMap;
    QMap<uint32_t, QString> metatileBehaviorMapInverse;
    ParseUtil parser;
    QSet<QString> modifiedFiles;
    bool usingAsmTilesets;
    QSet<QString> disabledSettingsNames;
    int pokemonMinLevel;
    int pokemonMaxLevel;
    int maxEncounterRate;
    bool wildEncountersLoaded;

    void setRoot(const QString&);

    const QStringList& mapNames() const { return this->alphabeticalMapNames; }
    QStringList getMapNamesByGroup() const;
    bool isKnownMap(const QString &mapName) const { return this->maps.contains(mapName); }
    bool isErroredMap(const QString &mapName) const { return this->erroredMaps.contains(mapName); }
    bool isLoadedMap(const QString &mapName) const { return this->loadedMapNames.contains(mapName); }
    bool isUnsavedMap(const QString &mapName) const;

    // Note: This does not guarantee the map is loaded.
    Map* getMap(const QString &mapName) { return this->maps.value(mapName); }
    Map* loadMap(const QString &mapName);

    const QStringList& layoutIds() const { return this->alphabeticalLayoutIds; }
    const QStringList& layoutIdsOrdered() const { return this->orderedLayoutIds; }
    bool isKnownLayout(const QString &layoutId) const { return this->mapLayouts.contains(layoutId); }
    bool isLoadedLayout(const QString &layoutId) const { return this->loadedLayoutIds.contains(layoutId); }
    bool isUnsavedLayout(const QString &layoutId) const;
    QString getLayoutName(const QString &layoutId) const;
    QStringList getLayoutNames() const;

    // Note: This does not guarantee the layout is loaded.
    Layout* getLayout(const QString &layoutId) const { return this->mapLayouts.value(layoutId); }
    Layout* loadLayout(const QString &layoutId);

    const QStringList& locationNames() const { return this->mapSectionIdNames; }
    const QStringList& locationNamesOrdered() const { return this->mapSectionIdNamesSaveOrder; }

    QString getLocationName(int locationValue) const { return this->mapSectionIdNamesSaveOrder.value(locationValue, getEmptyMapsecName()); }
    int getLocationValue(const QString &locationName) const { return this->mapSectionIdNamesSaveOrder.indexOf(locationName); }

    void clearMaps();
    void clearTilesetCache();
    void clearMapLayouts();
    void clearEventGraphics();
    void clearHealLocations();

    bool sanityCheck();
    int getSupportedMajorVersion(QString *errorOut = nullptr);
    bool load();

    QMap<QString, Tileset*> tilesetCache;
    Tileset* getTileset(const QString&, bool forceLoad = false);
    QStringList primaryTilesetLabels;
    QStringList secondaryTilesetLabels;
    QStringList tilesetLabelsOrdered;
    QSet<QString> getPairedTilesetLabels(const Tileset *tileset) const;
    QSet<QString> getTilesetLayoutIds(const Tileset *priamryTileset, const Tileset *secondaryTileset) const;

    bool readMapGroups();
    void addNewMapGroup(const QString &groupName);
    QString mapNameToMapGroup(const QString &mapName) const;
    QString getMapConstant(const QString &mapName, const QString &defaultValue = QString()) const;
    QString getMapLayoutId(const QString &mapName, const QString &defaultValue = QString()) const;
    QString getMapLocation(const QString &mapName, const QString &defaultValue = QString()) const;
    QString secretBaseIdToMapName(const QString &secretBaseId) const;

    struct NewMapSettings {
        QString name;
        QString group;
        bool canFlyTo;
        Layout::Settings layout;
        MapHeader header;
    };
    NewMapSettings newMapSettings;
    Layout::Settings newLayoutSettings;

    void initNewMapSettings();
    void initNewLayoutSettings();

    Map *createNewMap(const Project::NewMapSettings &mapSettings, const Map* toDuplicate = nullptr);
    Layout *createNewLayout(const Layout::Settings &layoutSettings, const Layout* toDuplicate = nullptr);
    Tileset *createNewTileset(QString name, bool secondary, bool checkerboardFill);
    bool isIdentifierUnique(const QString &identifier) const;
    bool isValidNewIdentifier(const QString &identifier) const;
    QString toUniqueIdentifier(const QString &identifier) const;
    QString getProjectTitle() const;
    QString getNewHealLocationName(const Map* map) const;

    bool readWildMonData();
    OrderedMap<QString, OrderedMap<QString, WildPokemonHeader>> wildMonData;

    QString wildMonTableName;
    QVector<EncounterField> wildMonFields;
    QVector<QString> encounterGroupLabels;

    bool readSpeciesIconPaths();
    QString getDefaultSpeciesIconPath(const QString &species);
    QPixmap getSpeciesIcon(const QString &species);

    bool addNewMapsec(const QString &idName, const QString &displayName = QString());
    void removeMapsec(const QString &idName);
    QString getMapsecDisplayName(const QString &idName) const { return this->locationData.value(idName).displayName; }
    void setMapsecDisplayName(const QString &idName, const QString &displayName);

    bool hasUnsavedChanges();
    bool hasUnsavedDataChanges = false;

    bool loadMapEvent(Map *map, QJsonObject json, Event::Type defaultType = Event::Type::None);
    bool loadMapData(Map*);
    bool readMapLayouts();
    bool loadLayoutTilesets(Layout *);
    bool loadTilesetAssets(Tileset*);
    void loadTilesetMetatileLabels(Tileset*);
    void readTilesetPaths(Tileset* tileset);

    bool saveAll();
    bool saveGlobalData();
    bool saveConfig();
    bool saveLayout(Layout *layout);
    bool saveMap(Map *map, bool skipLayout = false);
    bool saveTextFile(const QString &path, const QString &text);
    bool saveRegionMapSections();
    bool saveTilesets(Tileset*, Tileset*);
    bool saveTilesetMetatileLabels(Tileset*, Tileset*);

    void appendTilesetLabel(const QString &label, const QString &isSecondaryStr);
    bool readTilesetLabels();
    bool readTilesetMetatileLabels();
    bool readRegionMapSections();
    bool readItemNames();
    bool readFlagNames();
    bool readVarNames();
    bool readMovementTypes();
    bool readInitialFacingDirections();
    bool readMapTypes();
    bool readMapBattleScenes();
    bool readWeatherNames();
    bool readCoordEventWeatherNames();
    bool readSecretBaseIds();
    bool readBgEventFacingDirections();
    bool readTrainerTypes();
    bool readMetatileBehaviors();
    bool readHealLocations();
    bool readMiscellaneousConstants();
    bool readEventScriptLabels();
    bool readObjEventGfxConstants();
    bool readSongNames();
    bool readEventGraphics();
    bool readFieldmapProperties();
    bool readFieldmapMasks();
    bool readGlobalConstants();
    QMap<QString, QMap<QString, QString>> readObjEventGfxInfo();

    QPixmap getEventPixmap(const QString &gfxName, const QString &movementName);
    QPixmap getEventPixmap(const QString &gfxName, int frame, bool hFlip);
    QPixmap getEventPixmap(Event::Group group);
    void loadEventPixmap(Event *event, bool forceLoad = false);

    QString fixPalettePath(const QString &path) const;
    QString fixGraphicPath(const QString &path) const;

    static QString getScriptFileExtension(bool usePoryScript);
    QString getScriptDefaultString(bool usePoryScript, QString mapName) const;

    QStringList getAllEventScriptsFilepaths() const;
    QStringList getMapScriptsFilepaths() const;
    QStringList getCommonEventScriptsFilepaths() const;
    QStringList findScriptsFiles(const QString &searchDir, const QStringList &fileNames = {"*"}) const;
    void insertGlobalScriptLabels(QStringList &scriptLabels) const;

    QString getDefaultPrimaryTilesetLabel() const;
    QString getDefaultSecondaryTilesetLabel() const;
    void updateTilesetMetatileLabels(Tileset *tileset);
    QString buildMetatileLabelsText(const QMap<QString, uint16_t> defines);
    QString findMetatileLabelsTileset(QString label);

    bool watchFile(const QString &filename);
    bool watchFiles(const QStringList &filenames);
    bool stopFileWatch(const QString &filename);

    static QString getExistingFilepath(QString filepath);
    void applyParsedLimits();
    
    void setRegionMapEntries(const QHash<QString, MapSectionEntry> &entries);
    QHash<QString, MapSectionEntry> getRegionMapEntries() const;

    QSet<QString> getTopLevelMapFields() const;

    int getMapDataSize(int width, int height) const;
    int getMaxMapDataSize() const { return this->maxMapDataSize; }
    int getMaxMapWidth() const;
    int getMaxMapHeight() const;
    bool mapDimensionsValid(int width, int height) const;
    bool calculateDefaultMapSize();
    QSize getDefaultMapSize() const { return this->defaultMapSize; }
    QSize getMapSizeAddition() const { return this->mapSizeAddition; }

    int getMaxEvents(Event::Group group) const;

    static QString getEmptyMapDefineName();
    static QString getDynamicMapDefineName();
    static QString getDynamicMapName();
    static QString getEmptySpeciesName();
    static QMargins getPixelViewDistance();
    static QMargins getMetatileViewDistance();
    static int getNumTilesPrimary() { return num_tiles_primary; }
    static int getNumTilesTotal() { return num_tiles_total; }
    static int getNumTilesSecondary() { return getNumTilesTotal() - getNumTilesPrimary(); }
    static int getNumMetatilesPrimary() { return num_metatiles_primary; }
    static int getNumMetatilesTotal() { return Block::getMaxMetatileId() + 1; }
    static int getNumMetatilesSecondary() { return getNumMetatilesTotal() - getNumMetatilesPrimary(); }
    static int getNumPalettesPrimary(){ return num_pals_primary; }
    static int getNumPalettesTotal() { return num_pals_total; }
    static int getNumPalettesSecondary() { return getNumPalettesTotal() - getNumPalettesPrimary(); }
    static QString getEmptyMapsecName();
    static QString getMapGroupPrefix();

private:
    QPointer<QFileSystemWatcher> fileWatcher;
    QMap<QString, qint64> modifiedFileTimestamps;
    QMap<QString, QString> facingDirections;
    QHash<QString, QString> speciesToIconPath;
    QHash<QString, Map*> maps;
    QHash<QString, QString> erroredMaps;
    QStringList alphabeticalMapNames;
    QString layoutsLabel;
    QStringList alphabeticalLayoutIds;
    QStringList orderedLayoutIds;
    QStringList orderedLayoutIdsMaster;
    QHash<QString, Layout*> mapLayouts;
    QHash<QString, Layout*> mapLayoutsMaster;
    QStringList mapSectionIdNamesSaveOrder;
    QStringList mapSectionIdNames;

    // Fields for preserving top-level JSON data that Porymap isn't expecting.
    QJsonObject customLayoutsData;
    QJsonObject customMapSectionsData;
    QJsonObject customMapGroupsData;
    QJsonObject customHealLocationsData;
    OrderedJson::object customWildMonData;
    OrderedJson::object customWildMonGroupData;
    OrderedJson::array extraEncounterGroups;

    // Maps/layouts represented in these sets have been fully loaded from the project.
    // If a valid map name / layout id is not in these sets, a Map / Layout object exists
    // for it in Project::maps / Project::mapLayouts, but it has been minimally populated
    // (i.e. for a map layout it only has the data read from layouts.json, none of its assets
    // have been loaded, and for a map it only has the data needed to identify it in the map
    // list, none of the rest of its data in map.json).
    QSet<QString> loadedMapNames;
    QSet<QString> loadedLayoutIds;

    // Data for layouts that failed to load at launch.
    // We can't display these layouts to the user, but we want to preserve the data when they save.
    QList<QJsonObject> failedLayoutsData;

    QSet<QString> failedFileWatchPaths;

    const QRegularExpression re_gbapalExtension;
    const QRegularExpression re_bppExtension;

    struct EventGraphics
    {
        QString filepath;
        bool loaded = false;
        QImage spritesheet;
        int spriteWidth = -1;
        int spriteHeight = -1;
        bool inanimate = false;
    };
    QMap<QString, EventGraphics*> eventGraphicsMap;

    // The extra data that can be associated with each MAPSEC name.
    struct LocationData
    {
        MapSectionEntry map;
        QString displayName;
        QJsonObject custom;
    };
    QHash<QString, LocationData> locationData;

    QJsonDocument readMapJson(const QString &mapName, QString *error = nullptr);

    void setNewLayoutBlockdata(Layout *layout);
    void setNewLayoutBorder(Layout *layout);

    void ignoreWatchedFileTemporarily(const QString &filepath);
    void ignoreWatchedFilesTemporarily(const QStringList &filepaths);
    void recordFileChange(const QString &filepath);
    void resetFileCache();
    void resetFileWatcher();
    void logFileWatchStatus();
    void cacheTileset(const QString &label, Tileset *tileset);

    bool saveMapLayouts();
    bool saveMapGroups();
    bool saveWildMonData();
    bool saveHealLocations();
    bool appendTextFile(const QString &path, const QString &text);

    QString findSpeciesIconPath(const QStringList &names) const;

    int maxObjectEvents;
    int maxMapDataSize;
    QSize defaultMapSize;
    QSize mapSizeAddition;

    // TODO: These really shouldn't be static, they're specific to a single project.
    //       We're making an assumption here that we only have one project open at a single time
    //       (which is true, but then if that's the case we should have some global Project instance instead)
    static int num_tiles_primary;
    static int num_tiles_total;
    static int num_metatiles_primary;
    static int num_pals_primary;
    static int num_pals_total;

signals:
    void fileChanged(const QString &filepath);
    void mapLoaded(Map *map);
    void mapCreated(Map *newMap, const QString &groupName);
    void layoutCreated(Layout *newLayout);
    void tilesetCreated(Tileset *newTileset);
    void mapGroupAdded(const QString &groupName);
    void mapSectionAdded(const QString &idName);
    void mapSectionDisplayNameChanged(const QString &idName, const QString &displayName);
    void mapSectionIdNamesChanged(const QStringList &idNames);
    void eventScriptLabelsRead();
};

#endif // PROJECT_H
