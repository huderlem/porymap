#ifndef PROJECT_H
#define PROJECT_H

#include "map.h"
#include "blockdata.h"

#include <QStringList>
#include <QList>

class Project
{
public:
    Project();
    QString root;
    QStringList *groupNames = NULL;
    QList<QStringList*> *groupedMapNames = NULL;
    QStringList *mapNames = NULL;

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

    void readMapGroups();
    QString getProjectTitle();

    QList<QStringList>* getLabelMacros(QList<QStringList>*, QString);
    QStringList* getLabelValues(QList<QStringList>*, QString);
    void readMapHeader(Map*);
    void readMapAttributes(Map*);
    void getTilesets(Map*);
    void loadTilesetAssets(Tileset*);

    QString getBlockdataPath(Map*);
    void saveBlockdata(Map*);
    void writeBlockdata(QString, Blockdata*);
    void saveAllMaps();
    void saveMap(Map*);
    void saveMapHeader(Map*);

    QList<QStringList>* parse(QString text);
    QStringList getSongNames();
    QString getSongName(int);
    QStringList getLocations();
    QStringList getVisibilities();
    QStringList getWeathers();
    QStringList getMapTypes();
    QStringList getBattleScenes();

    void loadObjectPixmaps(QList<Event*> objects);
    QMap<QString, int> getMapObjGfxConstants();
    QString fixGraphicPath(QString path);

    void readMapEvents(Map *map);
    void loadMapConnections(Map *map);

    void loadMapBorder(Map *map);
    QString getMapBorderPath(Map *map);

    void saveMapEvents(Map *map);

    QStringList readCArray(QString text, QString label);
    QString readCIncbin(QString text, QString label);
    QMap<QString, int> readCDefines(QString text, QStringList prefixes);
};

#endif // PROJECT_H
