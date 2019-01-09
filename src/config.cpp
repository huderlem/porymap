#include "config.h"
#include "log.h"
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QComboBox>
#include <QLabel>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>

KeyValueConfigBase::~KeyValueConfigBase() {

}

void KeyValueConfigBase::load() {
    QFile file(this->getConfigFilepath());
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly)) {
            logError(QString("Could not create config file '%1'").arg(this->getConfigFilepath()));
        } else {
            file.close();
            this->onNewConfigFileCreated();
            this->save();
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open config file '%1': ").arg(this->getConfigFilepath()) + file.errorString());
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
            logWarn(QString("Invalid config line in %1: '%2'").arg(this->getConfigFilepath()).arg(line));
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

    QFile file(this->getConfigFilepath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
        file.close();
    } else {
        logError(QString("Could not open config file '%1' for writing: ").arg(this->getConfigFilepath()) + file.errorString());
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

QString PorymapConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);

    QString configPath = dir.absoluteFilePath("porymap.cfg");

    return configPath;
}

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
            logWarn(QString("Invalid config value for map_sort_order: '%1'. Must be 'group', 'area', or 'layout'.").arg(value));
        }
    } else if (key == "window_geometry") {
        this->windowGeometry = bytesFromString(value);
    } else if (key == "window_state") {
        this->windowState = bytesFromString(value);
    } else if (key == "map_splitter_state") {
        this->mapSplitterState = bytesFromString(value);
    } else if (key == "events_splitter_state") {
        this->eventsSlpitterState = bytesFromString(value);
    } else if (key == "main_splitter_state") {
        this->mainSplitterState = bytesFromString(value);
    } else if (key == "collision_opacity") {
        bool ok;
        this->collisionOpacity = qMax(0, qMin(100, value.toInt(&ok)));
        if (!ok) {
            logWarn(QString("Invalid config value for collision_opacity: '%1'. Must be an integer.").arg(value));
            this->collisionOpacity = 50;
        }
    } else if (key == "show_player_view") {
        bool ok;
        this->showPlayerView = value.toInt(&ok);
        if (!ok) {
            logWarn(QString("Invalid config value for show_player_view: '%1'. Must be 0 or 1.").arg(value));
        }
    } else if (key == "show_cursor_tile") {
        bool ok;
        this->showCursorTile = value.toInt(&ok);
        if (!ok) {
            logWarn(QString("Invalid config value for show_cursor_tile: '%1'. Must be 0 or 1.").arg(value));
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProject);
    map.insert("recent_map", this->recentMap);
    map.insert("pretty_cursors", this->prettyCursors ? "1" : "0");
    map.insert("map_sort_order", mapSortOrderMap.value(this->mapSortOrder));
    map.insert("window_geometry", stringFromByteArray(this->windowGeometry));
    map.insert("window_state", stringFromByteArray(this->windowState));
    map.insert("map_splitter_state", stringFromByteArray(this->mapSplitterState));
    map.insert("events_splitter_state", stringFromByteArray(this->eventsSlpitterState));
    map.insert("main_splitter_state", stringFromByteArray(this->mainSplitterState));
    map.insert("collision_opacity", QString("%1").arg(this->collisionOpacity));
    map.insert("show_player_view", this->showPlayerView ? "1" : "0");
    map.insert("show_cursor_tile", this->showCursorTile ? "1" : "0");
    return map;
}

QString PorymapConfig::stringFromByteArray(QByteArray bytearray) {
    QString ret;
    for (auto ch : bytearray) {
        ret += QString::number(static_cast<int>(ch)) + ":";
    }
    return ret;
}

QByteArray PorymapConfig::bytesFromString(QString in) {
    QByteArray ba;
    QStringList split = in.split(":");
    for (auto ch : split) {
        ba.append(static_cast<char>(ch.toInt()));
    }
    return ba;
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

void PorymapConfig::setGeometry(QByteArray windowGeometry_, QByteArray windowState_, QByteArray mapSplitterState_, 
                                QByteArray eventsSlpitterState_, QByteArray mainSplitterState_) {
    this->windowGeometry = windowGeometry_;
    this->windowState = windowState_;
    this->mapSplitterState = mapSplitterState_;
    this->eventsSlpitterState = eventsSlpitterState_;
    this->mainSplitterState = mainSplitterState_;
    this->save();
}

void PorymapConfig::setCollisionOpacity(int opacity) {
    this->collisionOpacity = opacity;
    // don't auto-save here because this can be called very frequently.
}

void PorymapConfig::setShowPlayerView(bool enabled) {
    this->showPlayerView = enabled;
    this->save();
}

void PorymapConfig::setShowCursorTile(bool enabled) {
    this->showCursorTile = enabled;
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

QMap<QString, QByteArray> PorymapConfig::getGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("window_geometry", this->windowGeometry);
    geometry.insert("window_state", this->windowState);
    geometry.insert("map_splitter_state", this->mapSplitterState);
    geometry.insert("events_splitter_state", this->eventsSlpitterState);
    geometry.insert("main_splitter_state", this->mainSplitterState);

    return geometry;
}

int PorymapConfig::getCollisionOpacity() {
    return this->collisionOpacity;
}

bool PorymapConfig::getShowPlayerView() {
    return this->showPlayerView;
}

bool PorymapConfig::getShowCursorTile() {
    return this->showCursorTile;
}

const QMap<BaseGameVersion, QString> baseGameVersionMap = {
    {BaseGameVersion::pokeruby, "pokeruby"},
    {BaseGameVersion::pokeemerald, "pokeemerald"},
};

const QMap<QString, BaseGameVersion> baseGameVersionReverseMap = {
    {"pokeruby", BaseGameVersion::pokeruby},
    {"pokeemerald", BaseGameVersion::pokeemerald},
};

ProjectConfig projectConfig;

QString ProjectConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    return QDir(this->projectDir).filePath("porymap.project.cfg");;
}

void ProjectConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "base_game_version") {
        QString baseGameVersion = value.toLower();
        if (baseGameVersionReverseMap.contains(baseGameVersion)) {
            this->baseGameVersion = baseGameVersionReverseMap.value(baseGameVersion);
        } else {
            this->baseGameVersion = BaseGameVersion::pokeemerald;
            logWarn(QString("Invalid config value for base_game_version: '%1'. Must be 'pokeruby' or 'pokeemerald'.").arg(value));
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
}

QMap<QString, QString> ProjectConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("base_game_version", baseGameVersionMap.value(this->baseGameVersion));
    return map;
}

void ProjectConfig::onNewConfigFileCreated() {
    QString dirName = QDir(this->projectDir).dirName().toLower();
    if (baseGameVersionReverseMap.contains(dirName)) {
        this->baseGameVersion = baseGameVersionReverseMap.value(dirName);
        logInfo(QString("Auto-detected base_game_version as '%1'").arg(dirName));
    } else {
        QDialog dialog(nullptr, Qt::WindowTitleHint);
        dialog.setWindowTitle("Project Configuration");
        dialog.setWindowModality(Qt::NonModal);

        QFormLayout form(&dialog);

        QComboBox *baseGameVersionComboBox = new QComboBox();
        baseGameVersionComboBox->addItem("pokeruby", BaseGameVersion::pokeruby);
        baseGameVersionComboBox->addItem("pokeemerald", BaseGameVersion::pokeemerald);
        form.addRow(new QLabel("Game Version"), baseGameVersionComboBox);

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
        connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        form.addRow(&buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            this->baseGameVersion = static_cast<BaseGameVersion>(baseGameVersionComboBox->currentData().toInt());
        }
    }
}

void ProjectConfig::setProjectDir(QString projectDir) {
    this->projectDir = projectDir;
}

void ProjectConfig::setBaseGameVersion(BaseGameVersion baseGameVersion) {
    this->baseGameVersion = baseGameVersion;
    this->save();
}

BaseGameVersion ProjectConfig::getBaseGameVersion() {
    return this->baseGameVersion;
}
