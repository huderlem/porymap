#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QObject>
#include <QByteArrayList>
#include <QSize>

enum MapSortOrder {
    Group   =  0,
    Area    =  1,
    Layout  =  2,
};

class KeyValueConfigBase : public QObject
{
public:
    void save();
    void load();
    virtual ~KeyValueConfigBase();
protected:
    virtual QString getConfigFilepath() = 0;
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
    virtual void onNewConfigFileCreated() = 0;
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig() {
        this->recentProject = "";
        this->recentMap = "";
        this->mapSortOrder = MapSortOrder::Group;
        this->prettyCursors = true;
        this->collisionOpacity = 50;
        this->metatilesZoom = 30;
        this->showPlayerView = false;
        this->showCursorTile = true;
        this->regionMapDimensions = QSize(32, 20);
    }
    void setRecentProject(QString project);
    void setRecentMap(QString map);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    void setGeometry(QByteArray, QByteArray, QByteArray, QByteArray, QByteArray);
    void setCollisionOpacity(int opacity);
    void setMetatilesZoom(int zoom);
    void setShowPlayerView(bool enabled);
    void setShowCursorTile(bool enabled);
    void setRegionMapDimensions(int width, int height);
    QString getRecentProject();
    QString getRecentMap();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
    QMap<QString, QByteArray> getGeometry();
    int getCollisionOpacity();
    int getMetatilesZoom();
    bool getShowPlayerView();
    bool getShowCursorTile();
    QSize getRegionMapDimensions();
protected:
    QString getConfigFilepath();
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
    void onNewConfigFileCreated() {}
private:
    QString recentProject;
    QString recentMap;
    QString stringFromByteArray(QByteArray);
    QByteArray bytesFromString(QString);
    MapSortOrder mapSortOrder;
    bool prettyCursors;
    QByteArray windowGeometry;
    QByteArray windowState;
    QByteArray mapSplitterState;
    QByteArray eventsSlpitterState;
    QByteArray mainSplitterState;
    int collisionOpacity;
    int metatilesZoom;
    bool showPlayerView;
    bool showCursorTile;
    QSize regionMapDimensions;
};

extern PorymapConfig porymapConfig;

enum BaseGameVersion {
    pokeruby,
    pokefirered,
    pokeemerald,
};

class ProjectConfig: public KeyValueConfigBase
{
public:
    ProjectConfig() {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
        this->useEncounterJson = true;
    }
    void setBaseGameVersion(BaseGameVersion baseGameVersion);
    BaseGameVersion getBaseGameVersion();
    void setEncounterJsonActive(bool active);
    bool getEncounterJsonActive();
    void setProjectDir(QString projectDir);
protected:
    QString getConfigFilepath();
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
    void onNewConfigFileCreated();
private:
    BaseGameVersion baseGameVersion;
    QString projectDir;
    bool useEncounterJson;
};

extern ProjectConfig projectConfig;

#endif // CONFIG_H
