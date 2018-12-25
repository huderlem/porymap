#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

enum MapSortOrder {
    Group   =  0,
    Area    =  1,
    Layout  =  2,
};

class KeyValueConfigBase
{
public:
    void save();
    void load();
    virtual ~KeyValueConfigBase();
protected:
    QString configFilename;
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig() {
        this->configFilename = "porymap.cfg";
        this->recentProject = "";
        this->recentMap = "";
        this->mapSortOrder = MapSortOrder::Group;
        this->prettyCursors = true;
    }
    void setRecentProject(QString project);
    void setRecentMap(QString map);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    QString getRecentProject();
    QString getRecentMap();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
protected:
    void parseConfigKeyValue(QString key, QString value);
    QMap<QString, QString> getKeyValueMap();
private:
    QString recentProject;
    QString recentMap;
    MapSortOrder mapSortOrder;
    bool prettyCursors;
};

extern PorymapConfig porymapConfig;

#endif // CONFIG_H
