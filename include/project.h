#pragma once
#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"
#include "heallocation.h"
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
    QStringList mapNames;
    QStringList groupNames;
    QMap<QString, QStringList> groupNameToMapNames;
    QList<HealLocation> healLocations;
    QMap<QString, int> healLocationNameToValue;
    QMap<QString, QString> mapConstantsToMapNames;
    QMap<QString, QString> mapNamesToMapConstants;
    QMap<QString, QString> mapNameToLayoutId;
    QMap<QString, QString> mapNameToMapSectionName;
    QString layoutsLabel;
    QStringList layoutIds;
    QStringList layoutIdsMaster;
    QMap<QString, Layout*> mapLayouts;
    QMap<QString, Layout*> mapLayoutsMaster;
    QMap<QString, EventGraphics*> eventGraphicsMap;
    QMap<QString, int> gfxDefines;
    QString defaultSong;
    QStringList songNames;
    QStringList itemNames;
    QStringList flagNames;
    QStringList varNames;
    QStringList movementTypes;
    QStringList mapTypes;
    QStringList mapBattleScenes;
    QStringList weatherNames;
    QStringList coordEventWeatherNames;
    QStringList secretBaseIds;
    QStringList bgEventFacingDirections;
    QStringList trainerTypes;
    QStringList globalScriptLabels;
    QStringList mapSectionIdNames;
    QMap<QString, MapSectionEntry> regionMapEntries;
    QMap<QString, QMap<QString, uint16_t>> metatileLabelsMap;
    QMap<QString, uint16_t> unusedMetatileLabels;
    QMap<QString, uint32_t> metatileBehaviorMap;
    QMap<uint32_t, QString> metatileBehaviorMapInverse;
    QMap<QString, QString> facingDirections;
    ParseUtil parser;
    QFileSystemWatcher fileWatcher;
    QMap<QString, qint64> modifiedFileTimestamps;
    bool usingAsmTilesets;
    QSet<QString> disabledSettingsNames;
    QSet<QString> topLevelMapFields;
    int pokemonMinLevel;
    int pokemonMaxLevel;
    int maxEncounterRate;
    bool wildEncountersLoaded;
    bool saveEmptyMapsec;

    void set_root(QString);

    void clearMapCache();
    void clearTilesetCache();
    void clearMapLayouts();
    void clearEventGraphics();

    struct DataQualifiers
    {
        bool isStatic;
        bool isConst;
    };
    DataQualifiers getDataQualifiers(QString, QString);
    DataQualifiers healLocationDataQualifiers;
    QString healLocationsTableName;

    bool sanityCheck();
    bool load();

    QMap<QString, Map*> mapCache;
    Map* loadMap(QString);
    Map* getMap(QString);

    QMap<QString, Tileset*> tilesetCache;
    Tileset* loadTileset(QString, Tileset *tileset = nullptr);
    Tileset* getTileset(QString, bool forceLoad = false);
    QStringList primaryTilesetLabels;
    QStringList secondaryTilesetLabels;
    QStringList tilesetLabelsOrdered;

    Blockdata readBlockdata(QString);
    bool loadBlockdata(Layout *);
    bool loadLayoutBorder(Layout *);

    void saveTextFile(QString path, QString text);
    void appendTextFile(QString path, QString text);
    void deleteFile(QString path);

    bool readMapGroups();
    void addNewMapGroup(const QString &groupName);

    struct NewMapSettings {
        QString name;
        QString group;
        bool canFlyTo;
        Layout::Settings layout;
        MapHeader header;
    };
    NewMapSettings newMapSettings;
    Layout::Settings newLayoutSettings;

    QString getNewMapName() const;
    QString getNewLayoutName() const;
    void initNewMapSettings();
    void initNewLayoutSettings();

    Map *createNewMap(const Project::NewMapSettings &mapSettings, const Map* toDuplicate = nullptr);
    Layout *createNewLayout(const Layout::Settings &layoutSettings, const Layout* toDuplicate = nullptr);
    Tileset *createNewTileset(const QString &friendlyName, bool secondary, bool checkerboardFill);
    bool isIdentifierUnique(const QString &identifier) const;
    QString getProjectTitle();

    bool readWildMonData();
    tsl::ordered_map<QString, tsl::ordered_map<QString, WildPokemonHeader>> wildMonData;

    QVector<EncounterField> wildMonFields;
    QVector<QString> encounterGroupLabels;
    QVector<poryjson::Json::object> extraEncounterGroups;

    bool readSpeciesIconPaths();
    QMap<QString, QString> speciesToIconPath;

    void addNewMapsec(const QString &name);
    void removeMapsec(const QString &name);

    bool hasUnsavedChanges();
    bool hasUnsavedDataChanges = false;

    void initTopLevelMapFields();
    bool readMapJson(const QString &mapName, QJsonDocument * out);
    bool loadMapData(Map*);
    bool readMapLayouts();
    Layout *loadLayout(QString layoutId);
    bool loadLayout(Layout *);
    bool loadMapLayout(Map*);
    bool loadLayoutTilesets(Layout *);
    void loadTilesetAssets(Tileset*);
    void loadTilesetMetatileLabels(Tileset*);
    void readTilesetPaths(Tileset* tileset);

    void saveLayout(Layout *);
    void saveLayoutBlockdata(Layout *);
    void saveLayoutBorder(Layout *);
    void writeBlockdata(QString, const Blockdata &);
    void saveAllMaps();
    void saveMap(Map *);
    void saveAllDataStructures();
    void saveConfig();
    void saveMapLayouts();
    void saveMapGroups();
    void saveRegionMapSections();
    void saveWildMonData();
    void saveHealLocations(Map*);
    void saveTilesets(Tileset*, Tileset*);
    void saveTilesetMetatileLabels(Tileset*, Tileset*);
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
    bool readHealLocationConstants();
    bool readHealLocations();
    bool readMiscellaneousConstants();
    bool readEventScriptLabels();
    bool readObjEventGfxConstants();
    bool readSongNames();
    bool readEventGraphics();
    bool readFieldmapProperties();
    bool readFieldmapMasks();
    QMap<QString, QMap<QString, QString>> readObjEventGfxInfo();

    void setEventPixmap(Event *event, bool forceLoad = false);

    QString fixPalettePath(QString path);
    QString fixGraphicPath(QString path);

    static QString getScriptFileExtension(bool usePoryScript);
    QString getScriptDefaultString(bool usePoryScript, QString mapName) const;
    QStringList getEventScriptsFilePaths() const;

    QString getDefaultPrimaryTilesetLabel() const;
    QString getDefaultSecondaryTilesetLabel() const;
    void updateTilesetMetatileLabels(Tileset *tileset);
    QString buildMetatileLabelsText(const QMap<QString, uint16_t> defines);
    QString findMetatileLabelsTileset(QString label);

    static QString getExistingFilepath(QString filepath);
    void applyParsedLimits();

    static QString getEmptyMapDefineName();
    static QString getDynamicMapDefineName();
    static QString getDynamicMapName();
    static int getNumTilesPrimary();
    static int getNumTilesTotal();
    static int getNumMetatilesPrimary();
    static int getNumMetatilesTotal();
    static int getNumPalettesPrimary();
    static int getNumPalettesTotal();
    static int getMaxMapDataSize();
    static int getDefaultMapDimension();
    static int getMaxMapWidth();
    static int getMaxMapHeight();
    static int getMapDataSize(int width, int height);
    static bool mapDimensionsValid(int width, int height);
    bool calculateDefaultMapSize();
    static int getMaxObjectEvents();
    static QString getEmptyMapsecName();

private:
    void updateLayout(Layout *);

    void setNewLayoutBlockdata(Layout *layout);
    void setNewLayoutBorder(Layout *layout);

    void saveHealLocationsData(Map *map);
    void saveHealLocationsConstants();

    void ignoreWatchedFileTemporarily(QString filepath);

    static int num_tiles_primary;
    static int num_tiles_total;
    static int num_metatiles_primary;
    static int num_pals_primary;
    static int num_pals_total;
    static int max_map_data_size;
    static int default_map_dimension;
    static int max_object_events;

signals:
    void fileChanged(QString filepath);
    void mapLoaded(Map *map);
    void mapCreated(Map *newMap, const QString &groupName);
    void layoutCreated(Layout *newLayout);
    void tilesetCreated(Tileset *newTileset);
    void mapGroupAdded(const QString &groupName);
    void mapSectionAdded(const QString &idName);
    void mapSectionIdNamesChanged(const QStringList &idNames);
};

#endif // PROJECT_H
