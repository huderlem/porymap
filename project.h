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
    QMap<int, QString> mapAttributesTable;
    QMap<int, QString> mapAttributesTableMaster;
    QMap<QString, QMap<QString, QString>> mapAttributes;
    QMap<QString, QMap<QString, QString>> mapAttributesMaster;
    QStringList *itemNames = NULL;
    QStringList *flagNames = NULL;
    QStringList *varNames = NULL;
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
    void readMapAttributesTable();
    void readAllMapAttributes();
    void readMapAttributes(Map*);
    void readMapsWithConnections();
    void getTilesets(Map*);
    void loadTilesetAssets(Tileset*);

    QString getBlockdataPath(Map*);
    void saveBlockdata(Map*);
    void saveMapBorder(Map*);
    void writeBlockdata(QString, Blockdata*);
    void saveAllMaps();
    void saveMap(Map*);
    void saveAllDataStructures();
    void saveAllMapAttributes();
    void saveMapGroupsTable();
    void saveMapConstantsHeader();

    QList<QStringList>* parseAsm(QString text);
    QStringList getSongNames();
    QStringList getLocations();
    QStringList getVisibilities();
    QStringList getWeathers();
    QStringList getMapTypes();
    QStringList getBattleScenes();
    void readItemNames();
    void readFlagNames();
    void readVarNames();

    void loadEventPixmaps(QList<Event*> objects);
    QMap<QString, int> getEventObjGfxConstants();
    QString fixGraphicPath(QString path);

    void readMapEvents(Map *map);
    void loadMapConnections(Map *map);

    void loadMapBorder(Map *map);
    QString getMapBorderPath(Map *map);

    void saveMapEvents(Map *map);

    QStringList readCArray(QString text, QString label);
    QString readCIncbin(QString text, QString label);
    QMap<QString, int> readCDefines(QString text, QStringList prefixes);
private:
    QString getMapAttributesTableFilepath();
    QString getMapAssetsFilepath();
    void saveMapHeader(Map*);
    void saveMapConnections(Map*);
    void updateMapsWithConnections(Map*);
    void saveMapsWithConnections();
    void saveMapAttributesTable();
    void updateMapAttributes(Map* map);
    void readCDefinesSorted(QString, QStringList, QStringList*);
    void readCDefinesSorted(QString, QStringList, QStringList*, QString, int);

    void setNewMapHeader(Map* map, int mapIndex);
    void setNewMapAttributes(Map* map);
    void setNewMapBlockdata(Map* map);
    void setNewMapBorder(Map *map);
    void setNewMapEvents(Map *map);
    void setNewMapConnections(Map *map);
};

#endif // PROJECT_H
