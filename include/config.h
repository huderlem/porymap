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
    virtual void reset() = 0;
protected:
    virtual QString getConfigFilepath() = 0;
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
    virtual void onNewConfigFileCreated() = 0;
    virtual void setUnreadKeys() = 0;
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig() {
        reset();
    }
    virtual void reset() override {
        this->recentProject = "";
        this->recentMap = "";
        this->mapSortOrder = MapSortOrder::Group;
        this->prettyCursors = true;
        this->collisionOpacity = 50;
        this->metatilesZoom = 30;
        this->showPlayerView = false;
        this->showCursorTile = true;
        this->monitorFiles = true;
        this->regionMapDimensions = QSize(32, 20);
        this->theme = "default";
    }
    void setRecentProject(QString project);
    void setRecentMap(QString map);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    void setGeometry(QByteArray, QByteArray, QByteArray, QByteArray);
    void setCollisionOpacity(int opacity);
    void setMetatilesZoom(int zoom);
    void setShowPlayerView(bool enabled);
    void setShowCursorTile(bool enabled);
    void setMonitorFiles(bool monitor);
    void setRegionMapDimensions(int width, int height);
    void setTheme(QString theme);
    QString getRecentProject();
    QString getRecentMap();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
    QMap<QString, QByteArray> getGeometry();
    int getCollisionOpacity();
    int getMetatilesZoom();
    bool getShowPlayerView();
    bool getShowCursorTile();
    bool getMonitorFiles();
    QSize getRegionMapDimensions();
    QString getTheme();
protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override {};
    virtual void setUnreadKeys() override {};
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
    bool monitorFiles;
    QSize regionMapDimensions;
    QString theme;
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
        reset();
    }
    virtual void reset() override {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
        this->useEncounterJson = true;
        this->useCustomBorderSize = false;
        this->enableEventWeatherTrigger = true;
        this->enableEventSecretBase = true;
        this->enableHiddenItemQuantity = false;
        this->enableHiddenItemRequiresItemfinder = false;
        this->enableHealLocationRespawnData = false;
        this->enableObjectEventInConnection = false;
        this->enableFloorNumber = false;
        this->enableTripleLayerMetatiles = false;
        this->customScripts.clear();
        this->readKeys.clear();
    }
    void setBaseGameVersion(BaseGameVersion baseGameVersion);
    BaseGameVersion getBaseGameVersion();
    void setEncounterJsonActive(bool active);
    bool getEncounterJsonActive();
    void setUsePoryScript(bool usePoryScript);
    bool getUsePoryScript();
    void setProjectDir(QString projectDir);
    QString getProjectDir();
    void setUseCustomBorderSize(bool enable);
    bool getUseCustomBorderSize();
    void setEventWeatherTriggerEnabled(bool enable);
    bool getEventWeatherTriggerEnabled();
    void setEventSecretBaseEnabled(bool enable);
    bool getEventSecretBaseEnabled();
    void setHiddenItemQuantityEnabled(bool enable);
    bool getHiddenItemQuantityEnabled();
    void setHiddenItemRequiresItemfinderEnabled(bool enable);
    bool getHiddenItemRequiresItemfinderEnabled();
    void setHealLocationRespawnDataEnabled(bool enable);
    bool getHealLocationRespawnDataEnabled();
    void setObjectEventInConnectionEnabled(bool enable);
    bool getObjectEventInConnectionEnabled();
    void setFloorNumberEnabled(bool enable);
    bool getFloorNumberEnabled();
    void setTripleLayerMetatilesEnabled(bool enable);
    bool getTripleLayerMetatilesEnabled();
    void setCustomScripts(QList<QString> scripts);
    QList<QString> getCustomScripts();
protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override;
    virtual void setUnreadKeys() override;
private:
    BaseGameVersion baseGameVersion;
    QString projectDir;
    bool useEncounterJson;
    bool usePoryScript;
    bool useCustomBorderSize;
    bool enableEventWeatherTrigger;
    bool enableEventSecretBase;
    bool enableHiddenItemQuantity;
    bool enableHiddenItemRequiresItemfinder;
    bool enableHealLocationRespawnData;
    bool enableObjectEventInConnection;
    bool enableFloorNumber;
    bool enableTripleLayerMetatiles;
    QList<QString> customScripts;
    QStringList readKeys;
};

extern ProjectConfig projectConfig;

#endif // CONFIG_H
