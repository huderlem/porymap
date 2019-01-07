#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QObject>
#include <QByteArrayList>

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
    }
    void setRecentProject(QString project);
    void setRecentMap(QString map);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    void setGeometry(QByteArray, QByteArray, QByteArray, QByteArray, QByteArray);
    void setCollisionOpacity(int opacity);
    QString getRecentProject();
    QString getRecentMap();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
    QMap<QString, QByteArray> getGeometry();
    int getCollisionOpacity();
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
};

extern PorymapConfig porymapConfig;

enum BaseGameVersion {
    pokeruby,
    pokeemerald,
};

class ProjectConfig: public KeyValueConfigBase
{
public:
    ProjectConfig() {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
    }
    void setBaseGameVersion(BaseGameVersion baseGameVersion);
    BaseGameVersion getBaseGameVersion();
    void setProjectDir(QString projectDir);
protected:
    QString getConfigFilepath();
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
    void onNewConfigFileCreated();
private:
    BaseGameVersion baseGameVersion;
    QString projectDir;
};

extern ProjectConfig projectConfig;

#endif // CONFIG_H
