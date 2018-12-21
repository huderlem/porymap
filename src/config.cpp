#include "config.h"
#include "log.h"
#include <QDir>
#include <QFile>
#include <QList>
#include <QTextStream>
#include <QRegularExpression>

const QString configFilename = "porymap.cfg";

QString Config::recentProject = "";
QString Config::recentMap = "";
MapSortOrder Config::mapSortOrder = MapSortOrder::Group;
bool Config::prettyCursors = true;

const QMap<MapSortOrder, QString> mapSortOrderMap = {
    {MapSortOrder::Group, "group"},
    {MapSortOrder::Layout, "layout"},
    {MapSortOrder::Area, "area"},
};

const QMap<QString, MapSortOrder> mapSortOrderReverseMap = {
    {"group", MapSortOrder::Group},
    {"layout", MapSortOrder::Layout},
    {"area", MapSortOrder::Area},
};

void Config::load() {
    QFile file(configFilename);
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly)) {
            logError(QString("Could not create config file '%1'").arg(configFilename));
        } else {
            file.close();
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open config file '%1': ").arg(configFilename) + file.errorString());
    }

    QTextStream in(&file);
    QList<QString> configLines;
    QRegularExpression re("^(?<key>.+)=(?<value>.+)$");
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        int commentIndex = line.indexOf("#");
        if (commentIndex >= 0) {
            line = line.left(commentIndex).trimmed();
        }

        if (line.length() == 0) {
            continue;
        }

        QRegularExpressionMatch match = re.match(line);
        if (!match.hasMatch()) {
            logWarn(QString("Invalid config line in %1: '%2'").arg(configFilename).arg(line));
            continue;
        }

        parseConfigKeyValue(match.captured("key").toLower(), match.captured("value"));
    }

    file.close();
}

void Config::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_project") {
        Config::recentProject = value;
    } else if (key == "recent_map") {
        Config::recentMap = value;
    } else if (key == "pretty_cursors") {
        bool ok;
        Config::prettyCursors = value.toInt(&ok);
        if (!ok) {
            logWarn(QString("Invalid config value for pretty_cursors: '%1'. Must be 0 or 1.").arg(value));
        }
    } else if (key == "map_sort_order") {
        QString sortOrder = value.toLower();
        if (mapSortOrderReverseMap.contains(sortOrder)) {
            Config::mapSortOrder = mapSortOrderReverseMap.value(sortOrder);
        } else {
            Config::mapSortOrder = MapSortOrder::Group;
            logWarn(QString("Invalid config value for map_sort_order: '%1'. Must be 'group', 'area', or 'layout'."));
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(configFilename).arg(key));
    }
}

void Config::save() {
    QString text = "";
    text += QString("recent_project=%1\n").arg(Config::recentProject);
    text += QString("recent_map=%1\n").arg(Config::recentMap);
    text += QString("pretty_cursors=%1\n").arg(Config::prettyCursors ? "1" : "0");
    text += QString("map_sort_order=%1\n").arg(mapSortOrderMap.value(Config::mapSortOrder));

    QFile file(configFilename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
        file.close();
    } else {
        logError(QString("Could not open config file '%1' for writing: ").arg(configFilename) + file.errorString());
    }
}

void Config::setRecentProject(QString project) {
    Config::recentProject = project;
    Config::save();
}

void Config::setRecentMap(QString map) {
    Config::recentMap = map;
    Config::save();
}

void Config::setMapSortOrder(MapSortOrder order) {
    Config::mapSortOrder = order;
    Config::save();
}

void Config::setPrettyCursors(bool enabled) {
    Config::prettyCursors = enabled;
    Config::save();
}

QString Config::getRecentProject() {
    return Config::recentProject;
}

QString Config::getRecentMap() {
    return Config::recentMap;
}

MapSortOrder Config::getMapSortOrder() {
    return Config::mapSortOrder;
}

bool Config::getPrettyCursors() {
    return Config::prettyCursors;
}
