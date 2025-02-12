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
    QStringList mapNames;
    QStringList groupNames;
    QMap<QString, QStringList> groupNameToMapNames;
    QStringList healLocationSaveOrder;
    QMap<QString, QList<HealLocationEvent*>> healLocations;
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
    QStringList mapSectionIdNamesSaveOrder;
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

    void set_root(QString);

    void clearMapCache();
    void clearTilesetCache();
    void clearMapLayouts();
    void clearEventGraphics();
    void clearHealLocations();

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

    Blockdata readBlockdata(QString, bool *ok = nullptr);
    bool loadBlockdata(Layout *);
    bool loadLayoutBorder(Layout *);

    void saveTextFile(QString path, QString text);
    void appendTextFile(QString path, QString text);
    void deleteFile(QString path);

    bool readMapGroups();
    void addNewMapGroup(const QString &groupName);
    QString mapNameToMapGroup(const QString &mapName);

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
    bool isValidNewIdentifier(QString identifier) const;
    QString toUniqueIdentifier(const QString &identifier) const;
    QString getProjectTitle() const;
    QString getNewHealLocationName(const Map* map) const;

    bool readWildMonData();
    tsl::ordered_map<QString, tsl::ordered_map<QString, WildPokemonHeader>> wildMonData;

    QVector<EncounterField> wildMonFields;
    QVector<QString> encounterGroupLabels;
    QVector<poryjson::Json::object> extraEncounterGroups;

    bool readSpeciesIconPaths();
    QPixmap getSpeciesIcon(const QString &species) const;
    QMap<QString, QString> speciesToIconPath;

    void addNewMapsec(const QString &idName);
    void removeMapsec(const QString &idName);
    QString getMapsecDisplayName(const QString &idName) const { return this->mapSectionDisplayNames.value(idName); }
    void setMapsecDisplayName(const QString &idName, const QString &displayName);

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
    void saveHealLocations();
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
    static QString getEmptySpeciesName();
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
    static QString getMapGroupPrefix();

    static void numericalModeSort(QStringList &list);

private:
    QMap<QString, QString> mapSectionDisplayNames;

    void updateLayout(Layout *);

    void setNewLayoutBlockdata(Layout *layout);
    void setNewLayoutBorder(Layout *layout);

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
    void mapSectionDisplayNameChanged(const QString &idName, const QString &displayName);
    void mapSectionIdNamesChanged(const QStringList &idNames);
    void mapsExcluded(const QStringList &excludedMapNames);
};

#endif // PROJECT_H
