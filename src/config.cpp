#include "config.h"
#include "log.h"
#include <QDir>
#include <QFile>
#include <QList>
#include <QTextStream>
#include <QRegularExpression>

KeyValueConfigBase::~KeyValueConfigBase() {

}

void KeyValueConfigBase::load() {
    QFile file(this->configFilename);
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly)) {
            logError(QString("Could not create config file '%1'").arg(this->configFilename));
        } else {
            file.close();
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open config file '%1': ").arg(this->configFilename) + file.errorString());
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
            logWarn(QString("Invalid config line in %1: '%2'").arg(this->configFilename).arg(line));
            continue;
        }

        this->parseConfigKeyValue(match.captured("key").toLower(), match.captured("value"));
    }

    file.close();
}

void KeyValueConfigBase::save() {
    QString text = "";
    QMap<QString, QString> map = this->getKeyValueMap();
    for (QMap<QString, QString>::iterator it = map.begin(); it != map.end(); it++) {
        text += QString("%1=%2\n").arg(it.key()).arg(it.value());
    }

    QFile file(this->configFilename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
        file.close();
    } else {
        logError(QString("Could not open config file '%1' for writing: ").arg(this->configFilename) + file.errorString());
    }
}

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

PorymapConfig porymapConfig;

void PorymapConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "recent_project") {
        this->recentProject = value;
    } else if (key == "recent_map") {
        this->recentMap = value;
    } else if (key == "pretty_cursors") {
        bool ok;
        this->prettyCursors = value.toInt(&ok);
        if (!ok) {
            logWarn(QString("Invalid config value for pretty_cursors: '%1'. Must be 0 or 1.").arg(value));
        }
    } else if (key == "map_sort_order") {
        QString sortOrder = value.toLower();
        if (mapSortOrderReverseMap.contains(sortOrder)) {
            this->mapSortOrder = mapSortOrderReverseMap.value(sortOrder);
        } else {
            this->mapSortOrder = MapSortOrder::Group;
            logWarn(QString("Invalid config value for map_sort_order: '%1'. Must be 'group', 'area', or 'layout'."));
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->configFilename).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProject);
    map.insert("recent_map", this->recentMap);
    map.insert("pretty_cursors", this->prettyCursors ? "1" : "0");
    map.insert("map_sort_order", mapSortOrderMap.value(this->mapSortOrder));
    return map;
}

void PorymapConfig::setRecentProject(QString project) {
    this->recentProject = project;
    this->save();
}

void PorymapConfig::setRecentMap(QString map) {
    this->recentMap = map;
    this->save();
}

void PorymapConfig::setMapSortOrder(MapSortOrder order) {
    this->mapSortOrder = order;
    this->save();
}

void PorymapConfig::setPrettyCursors(bool enabled) {
    this->prettyCursors = enabled;
    this->save();
}

QString PorymapConfig::getRecentProject() {
    return this->recentProject;
}

QString PorymapConfig::getRecentMap() {
    return this->recentMap;
}

MapSortOrder PorymapConfig::getMapSortOrder() {
    return this->mapSortOrder;
}

bool PorymapConfig::getPrettyCursors() {
    return this->prettyCursors;
}
