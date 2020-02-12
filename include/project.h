#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"
#include "heallocation.h"
#include "event.h"
#include "wildmoninfo.h"
#include "parseutil.h"

#include <QStringList>
#include <QList>
#include <QVector>
#include <QPair>
#include <QStandardItem>
#include <QVariant>

static QString NONE_MAP_CONSTANT = "MAP_NONE";
static QString NONE_MAP_NAME = "None";

class Project
{
public:
    Project();
    QString root;
    QStringList *groupNames = nullptr;
    QMap<QString, int> *map_groups;
    QList<QStringList> groupedMapNames;
    QStringList *mapNames = nullptr;
    QMap<QString, QVariant> miscConstants;
    QList<HealLocation> flyableMaps;
    QMap<QString, QString>* mapConstantsToMapNames;
    QMap<QString, QString>* mapNamesToMapConstants;
    QList<QString> mapLayoutsTable;
    QList<QString> mapLayoutsTableMaster;
    QString layoutsLabel;
    QMap<QString, MapLayout*> mapLayouts;
    QMap<QString, MapLayout*> mapLayoutsMaster;
    QMap<QString, QString> *mapSecToMapHoverName;
    QMap<QString, int> mapSectionNameToValue;
    QMap<int, QString> mapSectionValueToName;
    QStringList *itemNames = nullptr;
    QStringList *flagNames = nullptr;
    QStringList *varNames = nullptr;
    QStringList *movementTypes = nullptr;
    QStringList *mapTypes = nullptr;
    QStringList *mapBattleScenes = nullptr;
    QStringList *weatherNames = nullptr;
    QStringList *coordEventWeatherNames = nullptr;
    QStringList *secretBaseIds = nullptr;
    QStringList *bgEventFacingDirections = nullptr;
    QMap<QString, int> metatileBehaviorMap;
    QMap<int, QString> metatileBehaviorMapInverse;
    QMap<QString, QString> facingDirections;
    ParseUtil parser;

    void set_root(QString);

    struct DataQualifiers
    {
        bool isStatic;
        bool isConst;
    };
    DataQualifiers getDataQualifiers(QString, QString);
    QMap<QString, DataQualifiers> dataQualifiers;

    QMap<QString, Map*> *map_cache;
    Map* loadMap(QString);
    Map* getMap(QString);

    QMap<QString, Tileset*> *tileset_cache = nullptr;
    Tileset* loadTileset(QString, Tileset *tileset = nullptr);
    Tileset* getTileset(QString, bool forceLoad = false);
    QMap<QString, QStringList> tilesetLabels;

    Blockdata* readBlockdata(QString);
    bool loadBlockdata(Map*);

    void saveTextFile(QString path, QString text);
    void appendTextFile(QString path, QString text);
    void deleteFile(QString path);

    bool readMapGroups();
    Map* addNewMapToGroup(QString mapName, int groupNum);
    Map* addNewMapToGroup(QString, int, Map*, bool);
    QString getNewMapName();
    QString getProjectTitle();

    QString readMapLayoutId(QString map_name);
    QString readMapLocation(QString map_name);

    bool readWildMonData();
    QMap<QString, QMap<QString, WildPokemonHeader>> wildMonData;
    QVector<EncounterField> wildMonFields;
    QVector<QString> encounterGroupLabels;
    QMap<QString, QJsonObject> extraEncounterGroups;

    bool readSpeciesIconPaths();
    QMap<QString, QString> speciesToIconPath;

    QMap<QString, bool> getTopLevelMapFields();
    bool loadMapData(Map*);
    bool readMapLayouts();
    bool loadMapLayout(Map*);
    bool loadMapTilesets(Map*);
    void loadTilesetAssets(Tileset*);
    void loadTilesetTiles(Tileset*, QImage);
    void loadTilesetMetatiles(Tileset*);
    void loadTilesetMetatileLabels(Tileset*);

    void saveLayoutBlockdata(Map*);
    void saveLayoutBorder(Map*);
    void writeBlockdata(QString, Blockdata*);
    void saveAllMaps();
    void saveMap(Map*);
    void saveAllDataStructures();
    void saveMapLayouts();
    void saveMapGroups();
    void saveWildMonData();
    void saveMapConstantsHeader();
    void saveHealLocationStruct(Map*);
    void saveTilesets(Tileset*, Tileset*);
    void saveTilesetMetatileLabels(Tileset*, Tileset*);
    void saveTilesetMetatileAttributes(Tileset*);
    void saveTilesetMetatiles(Tileset*);
    void saveTilesetTilesImage(Tileset*);
    void saveTilesetPalettes(Tileset*, bool);

    QStringList getSongNames();
    QStringList getVisibilities();
    QMap<QString, QStringList> getTilesetLabels();
    bool readTilesetProperties();
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
    bool readMetatileBehaviors();
    bool readHealLocations();
    bool readMiscellaneousConstants();

    void loadEventPixmaps(QList<Event*> objects);
    QMap<QString, int> getEventObjGfxConstants();
    QString fixPalettePath(QString path);
    QString fixGraphicPath(QString path);

    QString getScriptFileExtension(bool usePoryScript);
    QString getScriptDefaultString(bool usePoryScript, QString mapName);

    bool loadMapBorder(Map *map);

    void saveMapHealEvents(Map *map);

    static int getNumTilesPrimary();
    static int getNumTilesTotal();
    static int getNumMetatilesPrimary();
    static int getNumMetatilesTotal();
    static int getNumPalettesPrimary();
    static int getNumPalettesTotal();

private:
    void updateMapLayout(Map*);

    void setNewMapHeader(Map* map, int mapIndex);
    void setNewMapLayout(Map* map);
    void setNewMapBlockdata(Map* map);
    void setNewMapBorder(Map *map);
    void setNewMapEvents(Map *map);
    void setNewMapConnections(Map *map);

    static int num_tiles_primary;
    static int num_tiles_total;
    static int num_metatiles_primary;
    static int num_metatiles_total;
    static int num_pals_primary;
    static int num_pals_total;
};

#endif // PROJECT_H
