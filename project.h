#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"

#include <QStringList>
#include <QList>
#include <QStandardItem>

class Project
{
public:
    Project();
    QString root;
    QStringList *groupNames = NULL;
    QMap<QString, int> *map_groups;
    QList<QStringList> groupedMapNames;
    QStringList *mapNames = NULL;
    QMap<QString, QString>* mapConstantsToMapNames;
    QMap<QString, QString>* mapNamesToMapConstants;
    QList<QString> mapLayoutsTable;
    QList<QString> mapLayoutsTableMaster;
    QMap<QString, MapLayout*> mapLayouts;
    QMap<QString, MapLayout*> mapLayoutsMaster;
    QStringList *regionMapSections = NULL;
    QStringList *itemNames = NULL;
    QStringList *flagNames = NULL;
    QStringList *varNames = NULL;
    QStringList *movementTypes = NULL;
    QStringList *mapTypes = NULL;
    QStringList *mapBattleScenes = NULL;
    QStringList *weatherNames = NULL;
    QStringList *coordEventWeatherNames = NULL;
    QStringList *secretBaseIds = NULL;
    QStringList *bgEventFacingDirections = NULL;
    QStringList mapsWithConnections;

    QMap<QString, Map*> *map_cache;
    Map* loadMap(QString);
    Map* getMap(QString);

    QMap<QString, Tileset*> *tileset_cache = NULL;
    Tileset* loadTileset(QString);
    Tileset* getTileset(QString);

    Blockdata* readBlockdata(QString);
    void loadBlockdata(Map*);

    QString readTextFile(QString path);
    void saveTextFile(QString path, QString text);
    void appendTextFile(QString path, QString text);
    void deleteFile(QString path);

    void readMapGroups();
    Map* addNewMapToGroup(QString mapName, int groupNum);
    QString getNewMapName();
    QString getProjectTitle();

    QList<QStringList>* getLabelMacros(QList<QStringList>*, QString);
    QStringList* getLabelValues(QList<QStringList>*, QString);
    void readMapHeader(Map*);
    void readMapLayoutsTable();
    void readAllMapLayouts();
    QStringList* readLayoutValues(QString layoutName);
    void readMapLayout(Map*);
    void readMapsWithConnections();
    void loadMapTilesets(Map*);
    void loadTilesetAssets(Tileset*);

    void saveBlockdata(Map*);
    void saveMapBorder(Map*);
    void writeBlockdata(QString, Blockdata*);
    void saveAllMaps();
    void saveMap(Map*);
    void saveAllDataStructures();
    void saveAllMapLayouts();
    void saveMapGroupsTable();
    void saveMapConstantsHeader();

    QList<QStringList>* parseAsm(QString text);
    QStringList getSongNames();
    QStringList getVisibilities();
    QMap<QString, QStringList> getTilesets();
    void readRegionMapSections();
    void readItemNames();
    void readFlagNames();
    void readVarNames();
    void readMovementTypes();
    void readMapTypes();
    void readMapBattleScenes();
    void readWeatherNames();
    void readCoordEventWeatherNames();
    void readSecretBaseIds();
    void readBgEventFacingDirections();

    void loadEventPixmaps(QList<Event*> objects);
    QMap<QString, int> getEventObjGfxConstants();
    QString fixGraphicPath(QString path);

    void readMapEvents(Map *map);
    void loadMapConnections(Map *map);

    void loadMapBorder(Map *map);

    void saveMapEvents(Map *map);

    QStringList readCArray(QString text, QString label);
    QString readCIncbin(QString text, QString label);
    QMap<QString, int> readCDefines(QString text, QStringList prefixes);
private:
    QString getMapLayoutsTableFilepath();
    QString getMapLayoutFilepath(QString);
    void saveMapHeader(Map*);
    void saveMapConnections(Map*);
    void updateMapsWithConnections(Map*);
    void saveMapsWithConnections();
    void saveMapLayoutsTable();
    void updateMapLayout(Map*);
    void readCDefinesSorted(QString, QStringList, QStringList*);
    void readCDefinesSorted(QString, QStringList, QStringList*, QString, int);

    void setNewMapHeader(Map* map, int mapIndex);
    void setNewMapLayout(Map* map);
    void setNewMapBlockdata(Map* map);
    void setNewMapBorder(Map *map);
    void setNewMapEvents(Map *map);
    void setNewMapConnections(Map *map);
};

#endif // PROJECT_H
