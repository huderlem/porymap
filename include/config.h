#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

enum MapSortOrder {
    Group   =  0,
    Area    =  1,
    Layout  =  2,
};

class Config
{
public:
    static void save();
    static void load();
    static void setRecentProject(QString project);
    static void setRecentMap(QString map);
    static void setMapSortOrder(MapSortOrder order);
    static void setPrettyCursors(bool enabled);
    static QString getRecentProject();
    static QString getRecentMap();
    static MapSortOrder getMapSortOrder();
    static bool getPrettyCursors();
private:
    static void parseConfigKeyValue(QString key, QString value);
    static QString recentProject;
    static QString recentMap;
    static MapSortOrder mapSortOrder;
    static bool prettyCursors;
};

#endif // CONFIG_H
