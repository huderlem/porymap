#include "config.h"
#include "log.h"
#include "shortcut.h"
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
#include <QAction>
#include <QAbstractButton>

KeyValueConfigBase::~KeyValueConfigBase() {

}

void KeyValueConfigBase::load() {
    reset();
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
    QRegularExpression re("^(?<key>[^=]+)=(?<value>.*)$");
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
    this->setUnreadKeys();

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

void KeyValueConfigBase::setConfigBool(QString key, bool * field, QString value) {
    bool ok;
    *field = value.toInt(&ok);
    if (!ok) {
        logWarn(QString("Invalid config value for %1: '%2'. Must be 0 or 1.").arg(key).arg(value));
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
    } else if (key == "reopen_on_launch") {
        setConfigBool(key, &this->reopenOnLaunch, value);
    } else if (key == "pretty_cursors") {
        setConfigBool(key, &this->prettyCursors, value);
    } else if (key == "map_sort_order") {
        QString sortOrder = value.toLower();
        if (mapSortOrderReverseMap.contains(sortOrder)) {
            this->mapSortOrder = mapSortOrderReverseMap.value(sortOrder);
        } else {
            this->mapSortOrder = MapSortOrder::Group;
            logWarn(QString("Invalid config value for map_sort_order: '%1'. Must be 'group', 'area', or 'layout'.").arg(value));
        }
    } else if (key == "main_window_geometry") {
        this->mainWindowGeometry = bytesFromString(value);
    } else if (key == "main_window_state") {
        this->mainWindowState = bytesFromString(value);
    } else if (key == "map_splitter_state") {
        this->mapSplitterState = bytesFromString(value);
    } else if (key == "main_splitter_state") {
        this->mainSplitterState = bytesFromString(value);
    } else if (key == "collision_opacity") {
        bool ok;
        this->collisionOpacity = qMax(0, qMin(100, value.toInt(&ok)));
        if (!ok) {
            logWarn(QString("Invalid config value for collision_opacity: '%1'. Must be an integer.").arg(value));
            this->collisionOpacity = 50;
        }
    } else if (key == "tileset_editor_geometry") {
        this->tilesetEditorGeometry = bytesFromString(value);
    } else if (key == "tileset_editor_state") {
        this->tilesetEditorState = bytesFromString(value);
    } else if (key == "palette_editor_geometry") {
        this->paletteEditorGeometry = bytesFromString(value);
    } else if (key == "palette_editor_state") {
        this->paletteEditorState = bytesFromString(value);
    } else if (key == "region_map_editor_geometry") {
        this->regionMapEditorGeometry = bytesFromString(value);
    } else if (key == "region_map_editor_state") {
        this->regionMapEditorState = bytesFromString(value);
    } else if (key == "metatiles_zoom") {
        bool ok;
        this->metatilesZoom = qMax(10, qMin(100, value.toInt(&ok)));
        if (!ok) {
            logWarn(QString("Invalid config value for metatiles_zoom: '%1'. Must be an integer.").arg(value));
            this->metatilesZoom = 30;
        }
    } else if (key == "show_player_view") {
        setConfigBool(key, &this->showPlayerView, value);
    } else if (key == "show_cursor_tile") {
        setConfigBool(key, &this->showCursorTile, value);
    } else if (key == "show_border") {
        setConfigBool(key, &this->showBorder, value);
    } else if (key == "show_grid") {
        setConfigBool(key, &this->showGrid, value);
    } else if (key == "monitor_files") {
        setConfigBool(key, &this->monitorFiles, value);
    } else if (key == "region_map_dimensions") {
        bool ok1, ok2;
        QStringList dims = value.split("x");
        int w = dims[0].toInt(&ok1);
        int h = dims[1].toInt(&ok2);
        if (!ok1 || !ok2) {
            logWarn("Cannot parse region map dimensions. Using default values instead.");
            this->regionMapDimensions = QSize(32, 20);
        } else {
            this->regionMapDimensions = QSize(w, h);
        }
    } else if (key == "theme") {
        this->theme = value;
    } else if (key == "text_editor_open_directory") {
        this->textEditorOpenFolder = value;
    } else if (key == "text_editor_goto_line") {
        this->textEditorGotoLine = value;
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
}

QMap<QString, QString> PorymapConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("recent_project", this->recentProject);
    map.insert("reopen_on_launch", this->reopenOnLaunch ? "1" : "0");
    map.insert("pretty_cursors", this->prettyCursors ? "1" : "0");
    map.insert("map_sort_order", mapSortOrderMap.value(this->mapSortOrder));
    map.insert("main_window_geometry", stringFromByteArray(this->mainWindowGeometry));
    map.insert("main_window_state", stringFromByteArray(this->mainWindowState));
    map.insert("map_splitter_state", stringFromByteArray(this->mapSplitterState));
    map.insert("main_splitter_state", stringFromByteArray(this->mainSplitterState));
    map.insert("tileset_editor_geometry", stringFromByteArray(this->tilesetEditorGeometry));
    map.insert("tileset_editor_state", stringFromByteArray(this->tilesetEditorState));
    map.insert("palette_editor_geometry", stringFromByteArray(this->paletteEditorGeometry));
    map.insert("palette_editor_state", stringFromByteArray(this->paletteEditorState));
    map.insert("region_map_editor_geometry", stringFromByteArray(this->regionMapEditorGeometry));
    map.insert("region_map_editor_state", stringFromByteArray(this->regionMapEditorState));
    map.insert("collision_opacity", QString("%1").arg(this->collisionOpacity));
    map.insert("metatiles_zoom", QString("%1").arg(this->metatilesZoom));
    map.insert("show_player_view", this->showPlayerView ? "1" : "0");
    map.insert("show_cursor_tile", this->showCursorTile ? "1" : "0");
    map.insert("show_border", this->showBorder ? "1" : "0");
    map.insert("show_grid", this->showGrid ? "1" : "0");
    map.insert("monitor_files", this->monitorFiles ? "1" : "0");
    map.insert("region_map_dimensions", QString("%1x%2").arg(this->regionMapDimensions.width())
                                                        .arg(this->regionMapDimensions.height()));
    map.insert("theme", this->theme);
    map.insert("text_editor_open_directory", this->textEditorOpenFolder);
    map.insert("text_editor_goto_line", this->textEditorGotoLine);
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

void PorymapConfig::setReopenOnLaunch(bool enabled) {
    this->reopenOnLaunch = enabled;
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

void PorymapConfig::setMonitorFiles(bool monitor) {
    this->monitorFiles = monitor;
    this->save();
}

void PorymapConfig::setMainGeometry(QByteArray mainWindowGeometry_, QByteArray mainWindowState_,
                                QByteArray mapSplitterState_, QByteArray mainSplitterState_) {
    this->mainWindowGeometry = mainWindowGeometry_;
    this->mainWindowState = mainWindowState_;
    this->mapSplitterState = mapSplitterState_;
    this->mainSplitterState = mainSplitterState_;
    this->save();
}

void PorymapConfig::setTilesetEditorGeometry(QByteArray tilesetEditorGeometry_, QByteArray tilesetEditorState_) {
    this->tilesetEditorGeometry = tilesetEditorGeometry_;
    this->tilesetEditorState = tilesetEditorState_;
    this->save();
}

void PorymapConfig::setPaletteEditorGeometry(QByteArray paletteEditorGeometry_, QByteArray paletteEditorState_) {
    this->paletteEditorGeometry = paletteEditorGeometry_;
    this->paletteEditorState = paletteEditorState_;
    this->save();
}

void PorymapConfig::setRegionMapEditorGeometry(QByteArray regionMapEditorGeometry_, QByteArray regionMapEditorState_) {
    this->regionMapEditorGeometry = regionMapEditorGeometry_;
    this->regionMapEditorState = regionMapEditorState_;
    this->save();
}

void PorymapConfig::setCollisionOpacity(int opacity) {
    this->collisionOpacity = opacity;
    // don't auto-save here because this can be called very frequently.
}

void PorymapConfig::setMetatilesZoom(int zoom) {
    this->metatilesZoom = zoom;
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

void PorymapConfig::setShowBorder(bool enabled) {
    this->showBorder = enabled;
    this->save();
}

void PorymapConfig::setShowGrid(bool enabled) {
    this->showGrid = enabled;
    this->save();
}

void PorymapConfig::setRegionMapDimensions(int width, int height) {
    this->regionMapDimensions = QSize(width, height);
}

void PorymapConfig::setTheme(QString theme) {
    this->theme = theme;
}

void PorymapConfig::setTextEditorOpenFolder(const QString &command) {
    this->textEditorOpenFolder = command;
    this->save();
}

void PorymapConfig::setTextEditorGotoLine(const QString &command) {
    this->textEditorGotoLine = command;
    this->save();
}

QString PorymapConfig::getRecentProject() {
    return this->recentProject;
}

bool PorymapConfig::getReopenOnLaunch() {
    return this->reopenOnLaunch;
}

MapSortOrder PorymapConfig::getMapSortOrder() {
    return this->mapSortOrder;
}

bool PorymapConfig::getPrettyCursors() {
    return this->prettyCursors;
}

QMap<QString, QByteArray> PorymapConfig::getMainGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("main_window_geometry", this->mainWindowGeometry);
    geometry.insert("main_window_state", this->mainWindowState);
    geometry.insert("map_splitter_state", this->mapSplitterState);
    geometry.insert("main_splitter_state", this->mainSplitterState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getTilesetEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("tileset_editor_geometry", this->tilesetEditorGeometry);
    geometry.insert("tileset_editor_state", this->tilesetEditorState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getPaletteEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("palette_editor_geometry", this->paletteEditorGeometry);
    geometry.insert("palette_editor_state", this->paletteEditorState);

    return geometry;
}

QMap<QString, QByteArray> PorymapConfig::getRegionMapEditorGeometry() {
    QMap<QString, QByteArray> geometry;

    geometry.insert("region_map_editor_geometry", this->regionMapEditorGeometry);
    geometry.insert("region_map_editor_state", this->regionMapEditorState);

    return geometry;
}

int PorymapConfig::getCollisionOpacity() {
    return this->collisionOpacity;
}

int PorymapConfig::getMetatilesZoom() {
    return this->metatilesZoom;
}

bool PorymapConfig::getShowPlayerView() {
    return this->showPlayerView;
}

bool PorymapConfig::getShowCursorTile() {
    return this->showCursorTile;
}

bool PorymapConfig::getShowBorder() {
    return this->showBorder;
}

bool PorymapConfig::getShowGrid() {
    return this->showGrid;
}

bool PorymapConfig::getMonitorFiles() {
    return this->monitorFiles;
}

QSize PorymapConfig::getRegionMapDimensions() {
    return this->regionMapDimensions;
}

QString PorymapConfig::getTheme() {
    return this->theme;
}

QString PorymapConfig::getTextEditorOpenFolder() {
    return this->textEditorOpenFolder;
}

QString PorymapConfig::getTextEditorGotoLine() {
    return this->textEditorGotoLine;
}

const QMap<BaseGameVersion, QString> baseGameVersionMap = {
    {BaseGameVersion::pokeruby, "pokeruby"},
    {BaseGameVersion::pokefirered, "pokefirered"},
    {BaseGameVersion::pokeemerald, "pokeemerald"},
};

const QMap<QString, BaseGameVersion> baseGameVersionReverseMap = {
    {"pokeruby", BaseGameVersion::pokeruby},
    {"pokefirered", BaseGameVersion::pokefirered},
    {"pokeemerald", BaseGameVersion::pokeemerald},
};

ProjectConfig projectConfig;

QString ProjectConfig::getConfigFilepath() {
    // porymap config file is in the same directory as porymap itself.
    return QDir(this->projectDir).filePath("porymap.project.cfg");
}

void ProjectConfig::parseConfigKeyValue(QString key, QString value) {
    if (key == "base_game_version") {
        QString baseGameVersion = value.toLower();
        if (baseGameVersionReverseMap.contains(baseGameVersion)) {
            this->baseGameVersion = baseGameVersionReverseMap.value(baseGameVersion);
        } else {
            this->baseGameVersion = BaseGameVersion::pokeemerald;
            logWarn(QString("Invalid config value for base_game_version: '%1'. Must be 'pokeruby', 'pokefirered' or 'pokeemerald'.").arg(value));
        }
    } else if (key == "recent_map") {
        this->recentMap = value;
    } else if (key == "use_encounter_json") {
        setConfigBool(key, &this->useEncounterJson, value);
    } else if (key == "use_poryscript") {
        setConfigBool(key, &this->usePoryScript, value);
    } else if (key == "use_custom_border_size") {
        setConfigBool(key, &this->useCustomBorderSize, value);
    } else if (key == "enable_event_weather_trigger") {
        setConfigBool(key, &this->enableEventWeatherTrigger, value);
    } else if (key == "enable_event_secret_base") {
        setConfigBool(key, &this->enableEventSecretBase, value);
    } else if (key == "enable_hidden_item_quantity") {
        setConfigBool(key, &this->enableHiddenItemQuantity, value);
    } else if (key == "enable_hidden_item_requires_itemfinder") {
        setConfigBool(key, &this->enableHiddenItemRequiresItemfinder, value);
    } else if (key == "enable_heal_location_respawn_data") {
        setConfigBool(key, &this->enableHealLocationRespawnData, value);
    } else if (key == "enable_event_clone_object") {
        setConfigBool(key, &this->enableEventCloneObject, value);
    } else if (key == "enable_floor_number") {
        setConfigBool(key, &this->enableFloorNumber, value);
    } else if (key == "create_map_text_file") {
        setConfigBool(key, &this->createMapTextFile, value);
    } else if (key == "enable_triple_layer_metatiles") {
        setConfigBool(key, &this->enableTripleLayerMetatiles, value);
    } else if (key == "custom_scripts") {
        this->customScripts.clear();
        QList<QString> paths = value.split(",");
        paths.removeDuplicates();
        for (QString script : paths) {
            if (!script.isEmpty()) {
                this->customScripts.append(script);
            }
        }
    } else {
        logWarn(QString("Invalid config key found in config file %1: '%2'").arg(this->getConfigFilepath()).arg(key));
    }
    readKeys.append(key);
}

void ProjectConfig::setUnreadKeys() {
    // Set game-version specific defaults for any config field that wasn't read
    bool isPokefirered = this->baseGameVersion == BaseGameVersion::pokefirered;
    if (!readKeys.contains("use_custom_border_size")) this->useCustomBorderSize = isPokefirered;
    if (!readKeys.contains("enable_event_weather_trigger")) this->enableEventWeatherTrigger = !isPokefirered;
    if (!readKeys.contains("enable_event_secret_base")) this->enableEventSecretBase = !isPokefirered;
    if (!readKeys.contains("enable_hidden_item_quantity")) this->enableHiddenItemQuantity = isPokefirered;
    if (!readKeys.contains("enable_hidden_item_requires_itemfinder")) this->enableHiddenItemRequiresItemfinder = isPokefirered;
    if (!readKeys.contains("enable_heal_location_respawn_data")) this->enableHealLocationRespawnData = isPokefirered;
    if (!readKeys.contains("enable_event_clone_object")) this->enableEventCloneObject = isPokefirered;
    if (!readKeys.contains("enable_floor_number")) this->enableFloorNumber = isPokefirered;
    if (!readKeys.contains("create_map_text_file")) this->createMapTextFile = (this->baseGameVersion != BaseGameVersion::pokeemerald);
}

QMap<QString, QString> ProjectConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    map.insert("base_game_version", baseGameVersionMap.value(this->baseGameVersion));
    map.insert("recent_map", this->recentMap);
    map.insert("use_encounter_json", QString::number(this->useEncounterJson));
    map.insert("use_poryscript", QString::number(this->usePoryScript));
    map.insert("use_custom_border_size", QString::number(this->useCustomBorderSize));
    map.insert("enable_event_weather_trigger", QString::number(this->enableEventWeatherTrigger));
    map.insert("enable_event_secret_base", QString::number(this->enableEventSecretBase));
    map.insert("enable_hidden_item_quantity", QString::number(this->enableHiddenItemQuantity));
    map.insert("enable_hidden_item_requires_itemfinder", QString::number(this->enableHiddenItemRequiresItemfinder));
    map.insert("enable_heal_location_respawn_data", QString::number(this->enableHealLocationRespawnData));
    map.insert("enable_event_clone_object", QString::number(this->enableEventCloneObject));
    map.insert("enable_floor_number", QString::number(this->enableFloorNumber));
    map.insert("create_map_text_file", QString::number(this->createMapTextFile));
    map.insert("enable_triple_layer_metatiles", QString::number(this->enableTripleLayerMetatiles));
    map.insert("custom_scripts", this->customScripts.join(","));
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
        baseGameVersionComboBox->addItem("pokefirered", BaseGameVersion::pokefirered);
        baseGameVersionComboBox->addItem("pokeemerald", BaseGameVersion::pokeemerald);
        form.addRow(new QLabel("Game Version"), baseGameVersionComboBox);

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        form.addRow(&buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            this->baseGameVersion = static_cast<BaseGameVersion>(baseGameVersionComboBox->currentData().toInt());
        }
    }
    bool isPokefirered = this->baseGameVersion == BaseGameVersion::pokefirered;
    this->useCustomBorderSize = isPokefirered;
    this->enableEventWeatherTrigger = !isPokefirered;
    this->enableEventSecretBase = !isPokefirered;
    this->enableHiddenItemQuantity = isPokefirered;
    this->enableHiddenItemRequiresItemfinder = isPokefirered;
    this->enableHealLocationRespawnData = isPokefirered;
    this->enableEventCloneObject = isPokefirered;
    this->enableFloorNumber = isPokefirered;
    this->createMapTextFile = (this->baseGameVersion != BaseGameVersion::pokeemerald);
    this->useEncounterJson = true;
    this->usePoryScript = false;
    this->enableTripleLayerMetatiles = false;
    this->customScripts.clear();
}

void ProjectConfig::setProjectDir(QString projectDir) {
    this->projectDir = projectDir;
}

QString ProjectConfig::getProjectDir() {
    return this->projectDir;
}

void ProjectConfig::setBaseGameVersion(BaseGameVersion baseGameVersion) {
    this->baseGameVersion = baseGameVersion;
    this->save();
}

BaseGameVersion ProjectConfig::getBaseGameVersion() {
    return this->baseGameVersion;
}

QString ProjectConfig::getBaseGameVersionString() {
    return baseGameVersionMap.value(this->baseGameVersion);
}

void ProjectConfig::setRecentMap(const QString &map) {
    this->recentMap = map;
    this->save();
}

QString ProjectConfig::getRecentMap() {
    return this->recentMap;
}

void ProjectConfig::setEncounterJsonActive(bool active) {
    this->useEncounterJson = active;
    this->save();
}

bool ProjectConfig::getEncounterJsonActive() {
    return this->useEncounterJson;
}

void ProjectConfig::setUsePoryScript(bool usePoryScript) {
    this->usePoryScript = usePoryScript;
    this->save();
}

bool ProjectConfig::getUsePoryScript() {
    return this->usePoryScript;
}

void ProjectConfig::setUseCustomBorderSize(bool enable) {
    this->useCustomBorderSize = enable;
    this->save();
}

bool ProjectConfig::getUseCustomBorderSize() {
    return this->useCustomBorderSize;
}

void ProjectConfig::setEventWeatherTriggerEnabled(bool enable) {
    this->enableEventWeatherTrigger = enable;
    this->save();
}

bool ProjectConfig::getEventWeatherTriggerEnabled() {
    return this->enableEventWeatherTrigger;
}

void ProjectConfig::setEventSecretBaseEnabled(bool enable) {
    this->enableEventSecretBase = enable;
    this->save();
}

bool ProjectConfig::getEventSecretBaseEnabled() {
    return this->enableEventSecretBase;
}

void ProjectConfig::setHiddenItemQuantityEnabled(bool enable) {
    this->enableHiddenItemQuantity = enable;
    this->save();
}

bool ProjectConfig::getHiddenItemQuantityEnabled() {
    return this->enableHiddenItemQuantity;
}

void ProjectConfig::setHiddenItemRequiresItemfinderEnabled(bool enable) {
    this->enableHiddenItemRequiresItemfinder = enable;
    this->save();
}

bool ProjectConfig::getHiddenItemRequiresItemfinderEnabled() {
    return this->enableHiddenItemRequiresItemfinder;
}

void ProjectConfig::setHealLocationRespawnDataEnabled(bool enable) {
    this->enableHealLocationRespawnData = enable;
    this->save();
}

bool ProjectConfig::getHealLocationRespawnDataEnabled() {
    return this->enableHealLocationRespawnData;
}

void ProjectConfig::setEventCloneObjectEnabled(bool enable) {
    this->enableEventCloneObject = enable;
    this->save();
}

bool ProjectConfig::getEventCloneObjectEnabled() {
    return this->enableEventCloneObject;
}

void ProjectConfig::setFloorNumberEnabled(bool enable) {
    this->enableFloorNumber = enable;
    this->save();
}

bool ProjectConfig::getFloorNumberEnabled() {
    return this->enableFloorNumber;
}

void ProjectConfig::setCreateMapTextFileEnabled(bool enable) {
    this->createMapTextFile = enable;
    this->save();
}

bool ProjectConfig::getCreateMapTextFileEnabled() {
    return this->createMapTextFile;
}

void ProjectConfig::setTripleLayerMetatilesEnabled(bool enable) {
    this->enableTripleLayerMetatiles = enable;
    this->save();
}

bool ProjectConfig::getTripleLayerMetatilesEnabled() {
    return this->enableTripleLayerMetatiles;
}

void ProjectConfig::setCustomScripts(QList<QString> scripts) {
    this->customScripts = scripts;
    this->save();
}

QList<QString> ProjectConfig::getCustomScripts() {
    return this->customScripts;
}

ShortcutsConfig shortcutsConfig;

QString ShortcutsConfig::getConfigFilepath() {
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);

    QString configPath = dir.absoluteFilePath("porymap.shortcuts.cfg");

    return configPath;
}

void ShortcutsConfig::parseConfigKeyValue(QString key, QString value) {
    QStringList keySequences = value.split(' ');
    for (auto keySequence : keySequences)
        user_shortcuts.insert(key, keySequence);
}

QMap<QString, QString> ShortcutsConfig::getKeyValueMap() {
    QMap<QString, QString> map;
    for (auto cfg_key : user_shortcuts.uniqueKeys()) {
        auto keySequences = user_shortcuts.values(cfg_key);
        QStringList keySequenceStrings;
        for (auto keySequence : keySequences)
            keySequenceStrings.append(keySequence.toString());
        map.insert(cfg_key, keySequenceStrings.join(' '));
    }
    return map;
}

void ShortcutsConfig::setDefaultShortcuts(const QObjectList &objects) {
    storeShortcutsFromList(StoreType::Default, objects);
    save();
}

QList<QKeySequence> ShortcutsConfig::defaultShortcuts(const QObject *object) const {
    return default_shortcuts.values(cfgKey(object));
}

void ShortcutsConfig::setUserShortcuts(const QObjectList &objects) {
    storeShortcutsFromList(StoreType::User, objects);
    save();
}

void ShortcutsConfig::setUserShortcuts(const QMultiMap<const QObject *, QKeySequence> &objects_keySequences) {
    for (auto *object : objects_keySequences.uniqueKeys())
        if (!object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            storeShortcuts(StoreType::User, cfgKey(object), objects_keySequences.values(object));
    save();
}

QList<QKeySequence> ShortcutsConfig::userShortcuts(const QObject *object) const {
    return user_shortcuts.values(cfgKey(object));
}

void ShortcutsConfig::storeShortcutsFromList(StoreType storeType, const QObjectList &objects) {
    for (const auto *object : objects)
        if (!object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            storeShortcuts(storeType, cfgKey(object), currentShortcuts(object));
}

void ShortcutsConfig::storeShortcuts(
        StoreType storeType,
        const QString &cfgKey,
        const QList<QKeySequence> &keySequences)
{
    bool storeUser = (storeType == User) || !user_shortcuts.contains(cfgKey);

    if (storeType == Default)
        default_shortcuts.remove(cfgKey);
    if (storeUser)
        user_shortcuts.remove(cfgKey);

    if (keySequences.isEmpty()) {
        if (storeType == Default)
            default_shortcuts.insert(cfgKey, QKeySequence());
        if (storeUser)
            user_shortcuts.insert(cfgKey, QKeySequence());
    } else {
        for (auto keySequence : keySequences) {
            if (storeType == Default)
                default_shortcuts.insert(cfgKey, keySequence);
            if (storeUser)
                user_shortcuts.insert(cfgKey, keySequence);
        }
    }
}

/* Creates a config key from the object's name prepended with the parent 
 * window's object name, and converts camelCase to snake_case. */
QString ShortcutsConfig::cfgKey(const QObject *object) const {
    auto cfg_key = QString();
    auto *parentWidget = static_cast<QWidget *>(object->parent());
    if (parentWidget)
        cfg_key = parentWidget->window()->objectName() + '_';
    cfg_key += object->objectName();

    QRegularExpression re("[A-Z]");
    int i = cfg_key.indexOf(re, 1);
    while (i != -1) {
        if (cfg_key.at(i - 1) != '_')
            cfg_key.insert(i++, '_');
        i = cfg_key.indexOf(re, i + 1);
    }
    return cfg_key.toLower();
}

QList<QKeySequence> ShortcutsConfig::currentShortcuts(const QObject *object) const {
    if (object->inherits("QAction")) {
        const auto *action = qobject_cast<const QAction *>(object);
        return action->shortcuts();
    } else if (object->inherits("Shortcut")) {
        const auto *shortcut = qobject_cast<const Shortcut *>(object);
        return shortcut->keys();
    } else if (object->inherits("QShortcut")) {
        const auto *qshortcut = qobject_cast<const QShortcut *>(object);
        return { qshortcut->key() };
    } else if (object->property("shortcut").isValid()) {
        return { object->property("shortcut").value<QKeySequence>() };
    } else {
        return { };
    }
}
